/************************************************************
 * FICHIER : src/actuators/mtr.cpp
 * ROLE    : Logique haut-niveau moteurs (Mega) - modern + legacy
 *
 * - Convertit les commandes anciennes (acc,spd,dir) vers L,R,A
 * - Fournit l'API moderne (mtr_setTargetsSigned) qui calcule L/R
 * - Fournit fonctions de sécurité : overrideStop / scaleTargets
 * - Périodique : mtr_processPeriodic() envoi vers hardware
 *
 * REMARQUE importante :
 *  - Ce fichier suppose l'existence d'une couche hardware qui expose
 *    des fonctions d'initialisation et d'envoi. Selon la version de ton
 *    projet, ces fonctions peuvent porter des noms différents.
 *    Voir les wrappers ci-dessous (/* HARDWARE WRAPPERS */) pour
 *    adapter si nécessaire.
 ************************************************************/

#include "mtr.h"
#include "../hardware/mtr_hardware.h"
#include "../configuration/config.h"
#include "../communication/communication.h"

#include <Arduino.h>
#include <math.h>

// ------------------------------------------------------------------
// NOTES SUR LES NOMS HARDWARE
//
// Dans différentes versions de ton repo on trouve plusieurs conventions:
//  - anciennes : mtr_init(), mtr_updateTrameFromConfig(), mtr_sendUno(), mtr_forceUrgStop()
//  - plus récentes : mtr_hw_init(), mtr_hw_updateTrameFromConfigLegacy(), mtr_hw_sendLegacy(),
//                   mtr_hw_updateTrameFromConfigModern(L,R,A), mtr_hw_sendModern()
//
// Ici on appelle des *wrappers* qui doivent être mappés vers les fonctions
// réelles de ton mtr_hardware. Si tu as une version différente, adapte les
// wrappers ci-dessous pour appeler tes fonctions matérielles réelles.
// ------------------------------------------------------------------

// ------------------ Wrappers (adapter ici si noms différents) ---------------

// Initialise le hardware moteur (wrapper)
static inline void _hw_init() {
    // si ton hardware propose mtr_hw_init(), utilise-la ; sinon mtr_init()
    // Exemple : mtr_hw_init();    OR    mtr_init();
    // /!\ Adapter ici si compilation échoue.
    #ifdef MTR_HW_INIT
        mtr_hw_init();
    #else
        // tentative : appeler mtr_init() si disponible
        mtr_init();
    #endif
}

// Met à jour la trame legacy (acc,spd,dir) depuis cfg et la prépare pour envoi
static inline void _hw_updateLegacyFromCfg() {
    // rename accordingly in hardware if needed
    #ifdef MTR_HW_UPDATE_LEGACY
        mtr_hw_updateTrameFromConfigLegacy();
    #else
        // fallback : generic update (old naming)
        mtr_updateTrameFromConfig();
    #endif
}

// Envoie la trame legacy au Uno (wrapper)
static inline void _hw_sendLegacy() {
    #ifdef MTR_HW_SEND_LEGACY
        mtr_hw_sendLegacy();
    #else
        mtr_sendUno();
    #endif
}

// Met à jour la trame moderne (L,R,A) - wrapper
static inline void _hw_updateModern(int L, int R, int A) {
    #ifdef MTR_HW_UPDATE_MODERN
        mtr_hw_updateTrameFromConfigModern(L, R, A);
    #else
        // Certaines implémentations n'ont pas de "modern" natif.
        // Si ton hardware n'a qu'une trame legacy, il faut ajouter un adaptateur
        // dans mtr_hardware pour convertir modern -> legacy ou envoyer directement
        // les caractères. Ici on essaye d'appeler une hypothétique fonction.
        // Si elle n'existe pas, commente cet appel et implemente l'adaptation.
        #warning "Adapter _hw_updateModern : ajouter mtr_hw_updateTrameFromConfigModern dans mtr_hardware"
    #endif
}

// Envoie la trame moderne (L,R,A) - wrapper
static inline void _hw_sendModern() {
    #ifdef MTR_HW_SEND_MODERN
        mtr_hw_sendModern();
    #else
        #warning "Adapter _hw_sendModern : ajouter mtr_hw_sendModern dans mtr_hardware"
    #endif
}

// Force arrêt matériel immédiat (wrapper)
static inline void _hw_forceUrgStop() {
    #ifdef MTR_HW_FORCE_URG
        mtr_forceUrgStop();
    #else
        // fallback : si hardware ne propose pas de wrapper, appeler urg_stopAllMotors si défini
        urg_stopAllMotors();
    #endif
}

// ------------------------------------------------------------------
// STATE (internals)
// ------------------------------------------------------------------

static int currentL = 0;   // last sent effective L (raw output units, ex -400..400)
static int currentR = 0;

static unsigned long lastSendTs = 0;
static unsigned long sendIntervalMs = 100UL; // resend périodique pour "keepalive"

// Safety / scaling (pour mvtsafe)
static float mtr_globalScale = 1.0f; // 0.0..1.0 ; multiplie targets
static bool  mtr_forcedStop  = false; // override stop (true => envoi 0,0)

// helpers
static inline int clampInt(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// computeDiffDrive:
// vin, omegaIn : sur l'échelle [-cfg_mtr_inputScale .. cfg_mtr_inputScale]
// outL/outR     : en unités de sortie ([-cfg_mtr_outputScale .. cfg_mtr_outputScale])
static void computeDiffDrive(int vin, int omegaIn, int *outL, int *outR) {
    float vNorm = (float)vin / (float)cfg_mtr_inputScale;
    float wNorm = (float)omegaIn / (float)cfg_mtr_inputScale;

    float scale = (float)cfg_mtr_outputScale;
    float k = cfg_mtr_kturn;

    float Lf = (vNorm - wNorm * k) * scale;
    float Rf = (vNorm + wNorm * k) * scale;

    int Li = (int)round(Lf);
    int Ri = (int)round(Rf);

    int maxOut = cfg_mtr_outputScale;
    Li = clampInt(Li, -maxOut, maxOut);
    Ri = clampInt(Ri, -maxOut, maxOut);

    *outL = Li;
    *outR = Ri;
}

// ------------------------------------------------------------------
// PUBLIC API IMPLEMENTATION
// ------------------------------------------------------------------

// Initialise le module MTR (doit être appelé depuis setup())
void mtr_init() {
    // init hardware via wrapper (adapter si noms différents)
    _hw_init();

    // synchroniser trames legacy depuis cfg (si existant)
    _hw_updateLegacyFromCfg();

    // initialisation runtime
    currentL = 0; currentR = 0;
    lastSendTs = millis();

    // init default modern targets si présents dans config
    cfg_mtr_targetL = 0;
    cfg_mtr_targetR = 0;
    cfg_mtr_targetA = 1;
}

// ---------------- Legacy API (compatibilité) ----------------

void mtr_setAcceleration(int a) {
    if (a < 0) a = 0; if (a > 9) a = 9;
    cfg_mtr_acc = char('0' + a);
    _hw_updateLegacyFromCfg();
    if (cfg_mtr_active) _hw_sendLegacy();
    sendInfo("Mtr","acc","*", String(a).c_str());
}

void mtr_setSpeed(int s) {
    if (s < 0) s = 0; if (s > 9) s = 9;
    cfg_mtr_spd = char('0' + s);
    _hw_updateLegacyFromCfg();
    if (cfg_mtr_active) _hw_sendLegacy();
    sendInfo("Mtr","spd","*", String(s).c_str());
}

void mtr_setDirection(int d) {
    if (d < 0) d = 0; if (d > 9) d = 9;
    cfg_mtr_dir = char('0' + d);
    _hw_updateLegacyFromCfg();
    if (cfg_mtr_active) _hw_sendLegacy();
    sendInfo("Mtr","dir","*", String(d).c_str());
}

void mtr_setTriplet(int a,int s,int d) {
    mtr_setAcceleration(a);
    mtr_setSpeed(s);
    mtr_setDirection(d);
}

void mtr_stopAll() {
    // legacy fields
    cfg_mtr_acc = '0';
    cfg_mtr_spd = '0';
    cfg_mtr_dir = '0';
    cfg_mtr_targetL = 0;
    cfg_mtr_targetR = 0;
    _hw_updateLegacyFromCfg();
    _hw_sendLegacy();

    // modern stop also (if hardware supports it)
    _hw_updateModern(0,0,0);
    _hw_sendModern();

    // notify
    sendInfo("Mtr","stop","*","OK");
}

// ---------------- Modern API ----------------

// Définit des cibles signées : v, omega sur l'échelle [-inputScale..inputScale], accelLevel 0..4
void mtr_setTargetsSigned(int v, int omega, int accelLevel) {
    // clamp inputs
    v = clampInt(v, -cfg_mtr_inputScale, cfg_mtr_inputScale);
    omega = clampInt(omega, -cfg_mtr_inputScale, cfg_mtr_inputScale);
    accelLevel = clampInt(accelLevel, 0, 4);

    // compute left/right in output units
    int L,R;
    computeDiffDrive(v, omega, &L, &R);

    // store runtime targets
    cfg_mtr_targetL = L;
    cfg_mtr_targetR = R;
    cfg_mtr_targetA = accelLevel;

    // send immediately if active and in modern mode
    if (cfg_mtr_active && cfg_mtr_mode_modern) {
        // apply scale if any (safety)
        int sendL = (int)round(L * mtr_globalScale);
        int sendR = (int)round(R * mtr_globalScale);

        _hw_updateModern(sendL, sendR, accelLevel);
        _hw_sendModern();

        currentL = sendL;
        currentR = sendR;
        lastSendTs = millis();
    }

    // ack
    char buf[64];
    snprintf(buf, sizeof(buf), "%d,%d,%d", L, R, accelLevel);
    sendInfo("Mtr","targets","*", buf);
}

// bascule modern/legacy
void mtr_setModeModern(bool modern) {
    cfg_mtr_mode_modern = modern;
    sendInfo("Mtr","mode","*", modern ? "modern" : "legacy");
}

bool mtr_isModeModern() { return cfg_mtr_mode_modern; }

// ---------------- Safety API (pour mvt_safe) ----------------

// Force un arrêt immédiat (override). Doit être appelé par safety en cas d'urgence.
void mtr_overrideStop() {
    mtr_forcedStop = true;
    // envoyer 0 immédiatement
    _hw_updateModern(0,0,0);
    _hw_sendModern();
    currentL = 0; currentR = 0;
    sendInfo("Mtr","override","*", "stop");
}

// Annule l'override stop (clear)
void mtr_clearOverride() {
    mtr_forcedStop = false;
    sendInfo("Mtr","override","*", "cleared");
}

// Applique un facteur de réduction (0.0..1.0) sur toutes les cibles (soft limit)
void mtr_scaleTargets(float s) {
    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    mtr_globalScale = s;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", mtr_globalScale);
    sendInfo("Mtr","scale","*", buf);
}

// ------------------------------------------------------------------
// mtr_processPeriodic()
//    - appelé depuis loop()
//    - gère l'envoi périodique en mode modern et applique scaling/override
// ------------------------------------------------------------------
void mtr_processPeriodic() {
    unsigned long now = millis();

    if (!cfg_mtr_active) return;

    // si override stop actif -> maintenir 0
    if (mtr_forcedStop) {
        // on renvoie régulièrement le 0 aux UNO au cas où
        if (now - lastSendTs >= sendIntervalMs) {
            _hw_updateModern(0,0,0);
            _hw_sendModern();
            lastSendTs = now;
        }
        return;
    }

    if (cfg_mtr_mode_modern) {
        // si cibles changées -> envoyer immédiatement (copie)
        if (cfg_mtr_targetL != currentL || cfg_mtr_targetR != currentR) {
            int scaledL = (int)round(cfg_mtr_targetL * mtr_globalScale);
            int scaledR = (int)round(cfg_mtr_targetR * mtr_globalScale);

            _hw_updateModern(scaledL, scaledR, cfg_mtr_targetA);
            _hw_sendModern();

            currentL = scaledL; currentR = scaledR;
            lastSendTs = now;
            return;
        }

        // resend périodique (keepalive)
        if (now - lastSendTs >= sendIntervalMs) {
            int scaledL = (int)round(currentL * mtr_globalScale);
            int scaledR = (int)round(currentR * mtr_globalScale);
            _hw_updateModern(scaledL, scaledR, cfg_mtr_targetA);
            _hw_sendModern();
            lastSendTs = now;
            return;
        }
    } else {
        // legacy mode : rien à faire périodiquement (hardware envoie sur changement)
        // on pourrait renvoyer la trame legacy périodiquement si souhaité :
        // _hw_updateLegacyFromCfg(); _hw_sendLegacy();
    }
}
