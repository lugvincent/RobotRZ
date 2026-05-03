/************************************************************
 * FICHIER : src/sensors/us.cpp
 * ROLE    : Logique haute-niveau pour les sonars (module Us)
 * Chemin  : src/sensors/us.cpp
 *
 * Principes :
 *  - lecture périodique cyclique (us_processPeriodic)
 *  - moyenne courte par cycle et comparaison avec valeur précédente
 *  - si |delta| >= US_DELTA_THRESHOLD -> envoi VPIV pour l'indice
 *  - si US_SEND_ALL==true -> envoi systématique de toutes les valeurs
 *  - si mesure == 0 répétée -> marquée suspect; si suspect dure > US_SUSPECT_TIMEOUT_MS alors
 *    considérée désactivée et on renvoie -1 pour cet index
 *  - alertes si distance <= cfg_us_danger_thresholds[idx] * alertMultiplier[idx]
 *
 ************************************************************/
/************************************************************
 * Logique haut-niveau du module US
 ************************************************************/
#include "us.h"
#include "../hardware/us_hardware.h"
#include "../system/urg.h"
#include "../communication/communication.h"
#include "../communication/vpiv_utils.h"

// valeurs internes
static int lastReported[US_NUM_SENSORS];
static int currValue[US_NUM_SENSORS];

static unsigned long suspectSince[US_NUM_SENSORS];
static bool sensorDisabled[US_NUM_SENSORS];

static unsigned long lastCycle = 0;

// ----------------------------------------------------------
// Envoi d’une valeur unique : $I:Us:one:<idx>:<val>
// ----------------------------------------------------------
static void send_one(int i, int v)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", v);

    char inst[4];
    snprintf(inst, sizeof(inst), "%d", i);

    sendInfo("Us", "one", inst, buf);
}

// ----------------------------------------------------------
// Envoi groupé : $I:Us:val:*:v1,v2,v3…
// ----------------------------------------------------------
static void send_all()
{
    char buf[128];
    buf[0] = 0;

    for (int i = 0; i < US_NUM_SENSORS; i++) {
        char t[8];
        snprintf(t, sizeof(t), "%d", sensorDisabled[i] ? -1 : currValue[i]);
        strcat(buf, t);
        if (i < US_NUM_SENSORS - 1) strcat(buf, ",");
    }

    sendInfo("Us", "val", "*", buf);
}

// ----------------------------------------------------------
// Init
// ----------------------------------------------------------
void us_init()
{
    us_initHardware();

    for (int i = 0; i < US_NUM_SENSORS; i++) {
        lastReported[i] = US_UNREPORTED;
        currValue[i]   = US_SILENCE;
        suspectSince[i] = 0;
        sensorDisabled[i] = false;
    }

    lastCycle = millis();
}

// ----------------------------------------------------------
// Boucle périodique sonar
// ----------------------------------------------------------
void us_processPeriodic()
{
    if (!cfg_us_active) return;

    // arrêt total en cas d’urgence
    if (urg_isActive()) return;

    unsigned long now = millis();
    if (now - lastCycle < cfg_us_ping_cycle_ms) return;
    lastCycle = now;

    for (int i = 0; i < US_NUM_SENSORS; i++) {

        if (sensorDisabled[i]) {
            currValue[i] = US_DISABLED;
            continue;
        }

        unsigned int d = us_measureOnce(i);

        // ----- gestion du mode suspect -----
        if (d == 0) {
            if (suspectSince[i] == 0)
                suspectSince[i] = now;

            if (now - suspectSince[i] >= cfg_us_suspect_timeout_ms) {
                sensorDisabled[i] = true;
                currValue[i] = US_DISABLED;
                send_one(i, US_DISABLED);
                lastReported[i] = US_DISABLED;
                continue;
            }

            currValue[i] = US_SILENCE;
            continue;
        }

        // mesure valide
        suspectSince[i] = 0;
        currValue[i] = d;

        // ----- système d’alerte danger -----
        // seuil danger = cfg_us_danger_thresholds[i]
        // distance d <= seuil × multiplicateur → danger
        if (d <= cfg_us_danger_thresholds[i] * cfg_us_alert_multiplier[i]) {

            // Niveau danger = intensité
            // Exemple : seuil 20cm ⇒ 20/5 = 4 si obstacle à 5cm
            int lvl = max(1, cfg_us_danger_thresholds[i] / max(1, (int)d));

            char buf[8];
            snprintf(buf, sizeof(buf), "%d", lvl);

            sendInfo("Us", "alert", String(i).c_str(), buf);
        }
    }

    // envoi groupé
    if (cfg_us_send_all) {
        send_all();
        for (int i = 0; i < US_NUM_SENSORS; i++)
            lastReported[i] = currValue[i];
        return;
    }

    // envoi des variations significatives
    for (int i = 0; i < US_NUM_SENSORS; i++) {

        int c = currValue[i];
        int p = lastReported[i];

        if (p == US_UNREPORTED) {
            send_one(i, c);
            lastReported[i] = c;
            continue;
        }

        if (c == US_DISABLED && p != US_DISABLED) {
            send_one(i, c);
            lastReported[i] = c;
            continue;
        }

        if (c == US_SILENCE) continue;

        if (abs(c - p) >= cfg_us_delta_threshold) {
            send_one(i, c);
            lastReported[i] = c;
        }
    }
}

// ----------------------------------------------------------
// Lecture forcée de certaines instances
// ----------------------------------------------------------
void us_handleReadList(const int* idxs, int n)
{
    // "*" -> tous
    if (n == 0) {
        for (int i = 0; i < US_NUM_SENSORS; i++) {
            int v = us_measureOnce(i);
            if (sensorDisabled[i]) v = US_DISABLED;
            else if (v == 0) v = US_SILENCE;
            send_one(i, v);
        }
        return;
    }

    // liste explicite
    for (int k = 0; k < n; k++) {
        int i = idxs[k];
        if (i < 0 || i >= US_NUM_SENSORS) continue;

        int v = us_measureOnce(i);
        if (sensorDisabled[i]) v = US_DISABLED;
        else if (v == 0) v = US_SILENCE;

        send_one(i, v);
    }
}
int us_peekCurrValue(int idx) {
    if (idx < 0 || idx >= US_NUM_SENSORS) return -1;
    // currValue[] existe dans us.cpp
    int v = currValue[idx];
    if (sensorDisabled[idx]) return -1;  // sécurité : si disabled → -1
    return v; // peut être 0 (silence) ou une valeur positive
}