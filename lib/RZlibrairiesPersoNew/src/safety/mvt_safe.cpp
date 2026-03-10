/************************************************************
 * FICHIER : src/safety/mvt_safe.cpp
 * ROLE    : Implémentation mvtSafe
 *
 * Comportement :
 *  - Lit périodiquement les valeurs US via us_peekCurrValue(idx)
 *  - Si valeur <= warn -> état WARN (envoie VPIV "warn")
 *  - Si valeur <= danger -> état DANGER (action selon mode)
 *      - mode SOFT : réduit les consignes moteurs en appliquant mvtsafe_maxScale
 *      - mode HARD : stop moteur (mtr_stopAll()) + optionnellement urg_handle()
 *  - Hystérésis simple : pour sortir d'un état on attend que la valeur monte
 *    au dessus du seuil + hysteresis_cm
 *
 * Non bloquant, adapté pour être appelé depuis loop()
 ************************************************************/

#include "mvt_safe.h"

#include "sensors/us.h"                  // us_peekCurrValue()
#include "actuators/mtr.h"               // mtr_* API
#include "system/urg.h"                  // urg_handle() / URG codes
#include "communication/communication.h" // sendInfo/sendError
#include "config/config.h"

#include <Arduino.h>
#include <string.h>
#include <math.h>

// Intervalle de check (ms)
static const unsigned long MVTSAFE_PERIOD_MS = 80UL;
static const int HYSTERESIS_CM = 5;

// état / config
static bool mvtsafe_active = true;
static int mvtsafe_mode = MVTSAFE_MODE_SOFT;
static int mvtsafe_warn_cm = 20;
static int mvtsafe_danger_cm = 10;
static float mvtsafe_maxScale = 0.5f;

static unsigned long mvtsafe_lastTs = 0;
static int mvtsafe_state = MVTSAFE_STATE_OK;

static void _send_vpiv_state(const char *what, const char *val)
{
    sendInfo("SecMvt", what, "*", val);
}

// ---------------------------------
// Apply soft reduction
// ---------------------------------
static void _apply_soft_reduction()
{
    int tgtL = cfg_mtr_targetL;
    int tgtR = cfg_mtr_targetR;
    float outScale = (cfg_mtr_outputScale > 0) ? (float)cfg_mtr_outputScale : 400.0f;
    float inScale = (cfg_mtr_inputScale > 0) ? (float)cfg_mtr_inputScale : 100.0f;
    float kturn = cfg_mtr_kturn;

    float Lf = (float)tgtL;
    float Rf = (float)tgtR;

    float vNorm = (Lf + Rf) / (2.0f * outScale);
    float wNorm = (kturn != 0.0f) ? (Rf - Lf) / (2.0f * outScale * kturn) : 0.0f;

    int vIn = (int)round(vNorm * inScale);
    int wIn = (int)round(wNorm * inScale);

    int vScaled = (int)round(vIn * mvtsafe_maxScale);
    int wScaled = (int)round(wIn * mvtsafe_maxScale);

    mtr_setTargetsSigned(vScaled, wScaled, cfg_mtr_targetA);
}

// ---------------------------------
// Apply hard stop
// ---------------------------------
static void _apply_hard_stop()
{
    mtr_stopAll();
    urg_handle(URG_MOTOR_STALL);
    _send_vpiv_state("alert", "danger");
}

// ---------------------------------
// Evaluate global state
// ---------------------------------
static int _evaluate_global_state()
{
    bool anyWarn = false;
    bool anyDanger = false;

    for (int i = 0; i < US_NUM_SENSORS; ++i)
    {
        int v = us_peekCurrValue(i);
        if (v <= 0)
            continue;
        if (v <= mvtsafe_danger_cm)
        {
            anyDanger = true;
            break;
        }
        if (v <= mvtsafe_warn_cm)
            anyWarn = true;
    }

    if (anyDanger)
        return MVTSAFE_STATE_DANGER;
    if (anyWarn)
        return MVTSAFE_STATE_WARN;
    return MVTSAFE_STATE_OK;
}

// -----------------------
// PUBLIC API
// -----------------------
void mvtsafe_init()
{
    mvtsafe_lastTs = millis();
    mvtsafe_state = MVTSAFE_STATE_OK;
    mvtsafe_active = true;
    _send_vpiv_state("init", "OK");
}

void mvtsafe_setActive(bool on)
{
    mvtsafe_active = on;
    _send_vpiv_state("act", on ? "1" : "0");
}

bool mvtsafe_isActive() { return mvtsafe_active; }

void mvtsafe_setMode(const char *mode)
{
    if (!mode)
        return;

    if (strcmp(mode, "idle") == 0)
        mvtsafe_mode = MVTSAFE_MODE_IDLE;
    else if (strcmp(mode, "soft") == 0)
        mvtsafe_mode = MVTSAFE_MODE_SOFT;
    else if (strcmp(mode, "hard") == 0)
        mvtsafe_mode = MVTSAFE_MODE_HARD;
    else
    {
        sendError("SecMvt", "mode", "*", "unknown");
        return;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%s", mode);
    _send_vpiv_state("mode", buf);
}

void mvtsafe_setThresholds(int warn_cm, int danger_cm)
{
    if (warn_cm < 1)
        warn_cm = 1;
    if (danger_cm < 0)
        danger_cm = 0;
    if (warn_cm < danger_cm)
        warn_cm = danger_cm;

    mvtsafe_warn_cm = warn_cm;
    mvtsafe_danger_cm = danger_cm;

    char buf[32];
    snprintf(buf, sizeof(buf), "%d,%d", mvtsafe_warn_cm, mvtsafe_danger_cm);
    _send_vpiv_state("thd", buf);
}

void mvtsafe_setMaxScale(float s)
{
    if (s < 0.0f)
        s = 0.0f;
    if (s > 1.0f)
        s = 1.0f;
    mvtsafe_maxScale = s;
    char buf[16];
    dtostrf(mvtsafe_maxScale, 0, 2, buf);
    _send_vpiv_state("scale", buf);
}

int mvtsafe_getState() { return mvtsafe_state; }

void mvtsafe_process()
{
    if (!mvtsafe_active)
        return;
    unsigned long now = millis();
    if (now - mvtsafe_lastTs < MVTSAFE_PERIOD_MS)
        return;
    mvtsafe_lastTs = now;

    int newState = _evaluate_global_state();

    if (newState != mvtsafe_state)
    {
        if (newState == MVTSAFE_STATE_OK)
            _send_vpiv_state("state", "OK");
        else if (newState == MVTSAFE_STATE_WARN)
        {
            _send_vpiv_state("state", "WARN");
            if (mvtsafe_mode == MVTSAFE_MODE_SOFT)
                _apply_soft_reduction();
        }
        else if (newState == MVTSAFE_STATE_DANGER)
        {
            _send_vpiv_state("state", "DANGER");
            if (mvtsafe_mode == MVTSAFE_MODE_HARD)
                _apply_hard_stop();
            else if (mvtsafe_mode == MVTSAFE_MODE_SOFT)
            {
                mvtsafe_setMaxScale(mvtsafe_maxScale * 0.5f);
                _apply_soft_reduction();
                sendInfo("SecMvt", "alert", "*", "danger_soft");
            }
        }
        mvtsafe_state = newState;
    }

    static unsigned long lastPub = 0;
    if (now - lastPub >= 1000UL)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "s:%d,w:%d,d:%d,scale:%.2f", mvtsafe_state, mvtsafe_warn_cm, mvtsafe_danger_cm, (double)mvtsafe_maxScale);
        sendInfo("SecMvt", "status", "*", buf);
        lastPub = now;
    }
}