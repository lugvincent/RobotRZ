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
 *    Voir les wrappers ci-dessous  HARDWARE WRAPPERS  pour
 *    adapter si nécessaire.
 ************************************************************/

#include "mtr.h"
#include "hardware/mtr_hardware.h"
#include "config/config.h"
#include "communication/communication.h"

#include <Arduino.h>
#include <math.h>

// ------------------------------------------------------------------
// STATE (internals)
// ------------------------------------------------------------------

static int currentL = 0; // last sent effective L (raw output units)
static int currentR = 0;

static unsigned long lastSendTs = 0;
static unsigned long sendIntervalMs = 100UL; // resend périodique pour "keepalive"

// Safety / scaling (pour mvtsafe)
static float mtr_globalScale = 1.0f; // 0.0..1.0 ; multiplie targets
static bool mtr_forcedStop = false;  // override stop (true => envoi 0,0)

// helpers
static inline int clampInt(int v, int lo, int hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

// computeDiffDrive:
// vin, omegaIn : sur l'échelle [-cfg_mtr_inputScale .. cfg_mtr_inputScale]
// outL/outR     : en unités de sortie ([-cfg_mtr_outputScale .. cfg_mtr_outputScale])
static void computeDiffDrive(int vin, int omegaIn, int *outL, int *outR)
{
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
void mtr_init()
{
    // init hardware directement
    mtr_hw_init();

    // synchroniser trames legacy depuis cfg
    mtr_hw_updateTrameFromConfigLegacy();

    // initialisation runtime
    currentL = 0;
    currentR = 0;
    lastSendTs = millis();

    // init default modern targets
    cfg_mtr_targetL = 0;
    cfg_mtr_targetR = 0;
    cfg_mtr_targetA = 1;
}

// ---------------- Legacy API ----------------

void mtr_setAcceleration(int a)
{
    if (a < 0)
        a = 0;
    if (a > 9)
        a = 9;
    cfg_mtr_acc = char('0' + a);
    mtr_hw_updateTrameFromConfigLegacy();
    if (cfg_mtr_active)
        mtr_hw_sendLegacy();
    sendInfo("Mtr", "acc", "*", String(a).c_str());
}

void mtr_setSpeed(int s)
{
    if (s < 0)
        s = 0;
    if (s > 9)
        s = 9;
    cfg_mtr_spd = char('0' + s);
    mtr_hw_updateTrameFromConfigLegacy();
    if (cfg_mtr_active)
        mtr_hw_sendLegacy();
    sendInfo("Mtr", "spd", "*", String(s).c_str());
}

void mtr_setDirection(int d)
{
    if (d < 0)
        d = 0;
    if (d > 9)
        d = 9;
    cfg_mtr_dir = char('0' + d);
    mtr_hw_updateTrameFromConfigLegacy();
    if (cfg_mtr_active)
        mtr_hw_sendLegacy();
    sendInfo("Mtr", "dir", "*", String(d).c_str());
}

void mtr_setTriplet(int a, int s, int d)
{
    mtr_setAcceleration(a);
    mtr_setSpeed(s);
    mtr_setDirection(d);
}

void mtr_stopAll()
{
    // legacy fields
    cfg_mtr_acc = '0';
    cfg_mtr_spd = '0';
    cfg_mtr_dir = '0';
    cfg_mtr_targetL = 0;
    cfg_mtr_targetR = 0;
    mtr_hw_updateTrameFromConfigLegacy();
    mtr_hw_sendLegacy();

    // modern stop also
    mtr_hw_updateTrameFromConfigModern(0, 0, 0);
    mtr_hw_sendModern();

    sendInfo("Mtr", "stop", "*", "OK");
}

// ---------------- Modern API ----------------

void mtr_setTargetsSigned(int v, int omega, int accelLevel)
{
    // clamp inputs
    v = clampInt(v, -cfg_mtr_inputScale, cfg_mtr_inputScale);
    omega = clampInt(omega, -cfg_mtr_inputScale, cfg_mtr_inputScale);
    accelLevel = clampInt(accelLevel, 0, 4);

    // compute left/right in output units
    int L, R;
    computeDiffDrive(v, omega, &L, &R);

    // store runtime targets
    cfg_mtr_targetL = L;
    cfg_mtr_targetR = R;
    cfg_mtr_targetA = accelLevel;

    // send immediately if active and in modern mode
    if (cfg_mtr_active && cfg_mtr_mode_modern)
    {
        int sendL = (int)round(L * mtr_globalScale);
        int sendR = (int)round(R * mtr_globalScale);

        mtr_hw_updateTrameFromConfigModern(sendL, sendR, accelLevel);
        mtr_hw_sendModern();

        currentL = sendL;
        currentR = sendR;
        lastSendTs = millis();
    }

    // ack
    char buf[64];
    snprintf(buf, sizeof(buf), "%d,%d,%d", L, R, accelLevel);
    sendInfo("Mtr", "targets", "*", buf);
}

void mtr_setModeModern(bool modern)
{
    cfg_mtr_mode_modern = modern;
    sendInfo("Mtr", "mode", "*", modern ? "modern" : "legacy");
}

bool mtr_isModeModern() { return cfg_mtr_mode_modern; }

// ---------------- Safety API ----------------

void mtr_overrideStop()
{
    mtr_forcedStop = true;
    mtr_hw_updateTrameFromConfigModern(0, 0, 0);
    mtr_hw_sendModern();
    currentL = 0;
    currentR = 0;
    sendInfo("Mtr", "override", "*", "stop");
}

void mtr_clearOverride()
{
    mtr_forcedStop = false;
    sendInfo("Mtr", "override", "*", "cleared");
}

void mtr_scaleTargets(float s)
{
    if (s < 0.0f)
        s = 0.0f;
    if (s > 1.0f)
        s = 1.0f;
    mtr_globalScale = s;
    char buf[16];
    dtostrf(mtr_globalScale, 5, 2, buf);
    sendInfo("Mtr", "scale", "*", buf);
}

// ------------------------------------------------------------------
// mtr_processPeriodic()
//    - appelé depuis loop()
//    - gère l'envoi périodique en mode modern et applique scaling/override
// ------------------------------------------------------------------
void mtr_processPeriodic()
{
    unsigned long now = millis();

    if (!cfg_mtr_active)
        return;

    if (mtr_forcedStop)
    {
        if (now - lastSendTs >= sendIntervalMs)
        {
            mtr_hw_updateTrameFromConfigModern(0, 0, 0);
            mtr_hw_sendModern();
            lastSendTs = now;
        }
        return;
    }

    if (cfg_mtr_mode_modern)
    {
        if (cfg_mtr_targetL != currentL || cfg_mtr_targetR != currentR)
        {
            int scaledL = (int)round(cfg_mtr_targetL * mtr_globalScale);
            int scaledR = (int)round(cfg_mtr_targetR * mtr_globalScale);

            mtr_hw_updateTrameFromConfigModern(scaledL, scaledR, cfg_mtr_targetA);
            mtr_hw_sendModern();

            currentL = scaledL;
            currentR = scaledR;
            lastSendTs = now;
            return;
        }

        if (now - lastSendTs >= sendIntervalMs)
        {
            int scaledL = (int)round(currentL * mtr_globalScale);
            int scaledR = (int)round(currentR * mtr_globalScale);
            mtr_hw_updateTrameFromConfigModern(scaledL, scaledR, cfg_mtr_targetA);
            mtr_hw_sendModern();
            lastSendTs = now;
            return;
        }
    }
    else
    {
        // legacy mode : rien à faire périodiquement
    }
}