/************************************************************
 * MODULE : Mvt-Safe (Sécurité mouvement)
 * ROLE   : Surveille les distances US → limite ou coupe moteurs
 * FONCTION :
 *    - warn  : réduction vitesse progressive
 *    - danger : stop immédiat
 *    - dépend uniquement du module US + de la commande moteurs
 ************************************************************/

#include "mvt_safe.h"

#include "../sensors/us.h"
#include "../actuators/mtr.h"
#include "../communication/communication.h"
#include "../configuration/config.h"

// ----------------------------------------------------------
// CONFIG RUNTIME
// ----------------------------------------------------------
static bool active = true;
static float maxScale = 1.0f;      // limitation vitesse globale
static int warnThd = 20;           // seuil avertissement
static int dangerThd = 10;         // seuil danger
static MvtSafeState state = MVT_SAFE_OK;

// ----------------------------------------------------------
// MAPPAGE SELON TON TABLEAU FINAL
// ----------------------------------------------------------
// Avant  (AvG, AvC, AvD)
static const int FRONT_IDX[3] = {5, 4, 3};

// Arrière (ArG, ArC, ArD)
static const int REAR_IDX[3]  = {0, 1, 2};

// Tête (TtG, TtD, TtC)
static const int HEAD_IDX[3]  = {6, 7, 8};

// Seuils personnalisés ultra-fins
static const int customThd[9] = {
    20,  // 0 ArG
    15,  // 1 ArC
    20,  // 2 ArD
    15,  // 3 AvD
    10,  // 4 AvC
    15,  // 5 AvG
    20,  // 6 TtG
    20,  // 7 TtD
    20   // 8 TtC
};

// ----------------------------------------------------------
static int getMinOf(const int* idxs, int n)
{
    int best = 9999;
    for (int i = 0; i < n; i++) {
        int d = us_getLastValue(idxs[i]);    // → lecture buffer US
        if (d > 0 && d < best) best = d;
    }
    return best;
}

int mvtsafe_getMinFront() { return getMinOf(FRONT_IDX, 3); }
int mvtsafe_getMinRear()  { return getMinOf(REAR_IDX , 3); }

// ----------------------------------------------------------
void mvtsafe_init() {
    state = MVT_SAFE_OK;
}

// ----------------------------------------------------------
void mvtsafe_setActive(bool en) {
    active = en;
}

void mvtsafe_setMode(const char* m) {
    if (!m) return;
    if (strcmp(m,"idle")==0) {
        active = false;
        state = MVT_SAFE_OK;
    }
    else if (strcmp(m,"soft")==0) {
        active = true;
        warnThd   = 20;
        dangerThd = 10;
    }
    else if (strcmp(m,"hard")==0) {
        active = true;
        warnThd   = 30;
        dangerThd = 15;
    }
}

void mvtsafe_setThresholds(int warn, int danger) {
    warnThd = warn;
    dangerThd = danger;
}

void mvtsafe_setMaxScale(float s) {
    if (s < 0.05f) s = 0.05f;
    if (s > 1.0f)  s = 1.0f;
    maxScale = s;
}

int mvtsafe_getState() {
    return (int)state;
}

// ----------------------------------------------------------
// LOGIQUE PRINCIPALE
// ----------------------------------------------------------
void mvtsafe_process()
{
    if (!active) {
        state = MVT_SAFE_OK;
        return;
    }

    // lire distances
    int df = mvtsafe_getMinFront();
    int dr = mvtsafe_getMinRear();

    // appliquer seuils "par capteur"
    int dmin = min(df, dr);

    if (dmin <= 0 || dmin == US_SILENCE || dmin == US_DISABLED) {
        return;
    }

    // priorité danger
    // mais en tenant compte des seuils individuels
    for (int i = 0; i < 9; i++) {
        int v = us_getLastValue(i);
        if (v <= 0) continue;

        if (v <= customThd[i]) {
            // STOP IMMEDIAT
            mtr_overrideStop();
            if (state != MVT_SAFE_DANGER) {
                sendInfo("mvtSafe","event","*","danger");
            }
            state = MVT_SAFE_DANGER;
            return;
        }
    }

    // warn ?
    if (dmin <= warnThd) {
        // limiter vitesse
        float scale = 1.0f;
        if (warnThd > dangerThd)
            scale = (float)(dmin - dangerThd) / (float)(warnThd - dangerThd);
        else
            scale = 0.5f;

        if (scale < maxScale) scale = maxScale;

        mtr_scaleTargets(scale);

        if (state != MVT_SAFE_WARN) {
            sendInfo("mvtSafe","event","*","warn");
        }
        state = MVT_SAFE_WARN;
        return;
    }

    // sinon OK
    if (state != MVT_SAFE_OK) {
        sendInfo("mvtSafe","event","*","ok");
    }
    state = MVT_SAFE_OK;
}
