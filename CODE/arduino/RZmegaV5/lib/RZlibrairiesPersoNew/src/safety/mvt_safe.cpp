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

#include "../sensors/us.h"                 // us_peekCurrValue()
#include "../actuators/mtr.h"              // mtr_* API
#include "../system/urg.h"                 // urg_handle() / URG codes
#include "../communication/communication.h"// sendInfo/sendError
#include "../configuration/config.h"

#include <Arduino.h>
#include <string.h>
#include <math.h>

// Intervalle de check (ms)
static const unsigned long MVTSAFE_PERIOD_MS = 80UL;

// Hystérésis (cm) pour éviter oscillations
static const int HYSTERESIS_CM = 5;

// état / config
static bool mvtsafe_active = true;
static int mvtsafe_mode = MVTSAFE_MODE_SOFT; // default Soft
static int mvtsafe_warn_cm = 20;   // warning distance
static int mvtsafe_danger_cm = 10; // danger distance
static float mvtsafe_maxScale = 0.5f; // facteur de réduction en soft mode (50%)

static unsigned long mvtsafe_lastTs = 0;

// état courant renvoyé
static int mvtsafe_state = MVTSAFE_STATE_OK;

// utilitaires internes
static void _send_vpiv_state(const char* what, const char* val) {
    sendInfo("mvtSafe", what, "*", val);
}

// calcule réduction des consignes en utilisant cfg_mtr_targetL/R (si présent)
// -> on reconstruit v/omega approximatif puis renvoie scaled via mtr_setTargetsSigned
static void _apply_soft_reduction() {
    // On essaie de récupérer les targets existants (runtime) depuis config
    // cfg_mtr_targetL / cfg_mtr_targetR sont en units output (-outputScale..outputScale)
    // Si absent, on enverra un arrêt doux (0).
    int tgtL = cfg_mtr_targetL;
    int tgtR = cfg_mtr_targetR;
    // outputScale et inputScale disponibles dans config
    float outScale = (cfg_mtr_outputScale > 0) ? (float)cfg_mtr_outputScale : 400.0f;
    float inScale  = (cfg_mtr_inputScale > 0) ? (float)cfg_mtr_inputScale : 100.0f;
    float kturn    = cfg_mtr_kturn;

    // Reconstruire vNorm et wNorm approximatifs :
    // Lf = (vNorm - wNorm * k) * outScale
    // Rf = (vNorm + wNorm * k) * outScale
    // => vNorm = (Lf + Rf) / (2*outScale)
    // => wNorm = (Rf - Lf) / (2*outScale*k)
    float Lf = (float)tgtL;
    float Rf = (float)tgtR;

    float vNorm = (Lf + Rf) / (2.0f * outScale);
    float wNorm = 0.0f;
    if (kturn != 0.0f)
        wNorm = (Rf - Lf) / (2.0f * outScale * kturn);

    // revient aux unités inScale
    int vIn = (int)round(vNorm * inScale);
    int wIn = (int)round(wNorm * inScale);

    // Appliquer scale
    int vScaled = (int)round(vIn * mvtsafe_maxScale);
    int wScaled = (int)round(wIn * mvtsafe_maxScale);

    // envoyer consigne réduite en modern mode (mtr API)
    mtr_setTargetsSigned(vScaled, wScaled, cfg_mtr_targetA);
}

// action en cas de danger (hard)
static void _apply_hard_stop() {
    // stop moteurs immédiatement
    mtr_stopAll();

    // signaler urgence moteur stall (ou generic motor stop)
    urg_handle(URG_MOTOR_STALL);

    // envoi VPIV info
    _send_vpiv_state("alert", "danger");
}

// test un capteur individuel (renvoie true si danger)
static bool _check_sensor_for_state(int idx, int warn_cm, int danger_cm) {
    int v = us_peekCurrValue(idx); // valeur non bloquante
    if (v <= 0) return false; // 0 = silence / timeout ; on ignore ici
    if (v <= danger_cm) return true;
    return false;
}

// scan all sensors to decide the global state
static int _evaluate_global_state() {
    bool anyWarn = false;
    bool anyDanger = false;

    for (int i = 0; i < US_NUM_SENSORS; ++i) {
        int v = us_peekCurrValue(i);
        if (v <= 0) continue; // ignore silence
        if (v <= mvtsafe_danger_cm) { anyDanger = true; break; }
        if (v <= mvtsafe_warn_cm) anyWarn = true;
    }

    if (anyDanger) return MVTSAFE_STATE_DANGER;
    if (anyWarn) return MVTSAFE_STATE_WARN;
    return MVTSAFE_STATE_OK;
}

// ------------------------------------------------------------------
// PUBLIC API
// ------------------------------------------------------------------
void mvtsafe_init() {
    mvtsafe_lastTs = millis();
    mvtsafe_state = MVTSAFE_STATE_OK;
    mvtsafe_active = true;
    // annonce initial state
    _send_vpiv_state("init", "OK");
}

void mvtsafe_setActive(bool on) {
    mvtsafe_active = on;
    _send_vpiv_state("act", on ? "1" : "0");
}

bool mvtsafe_isActive() {
    return mvtsafe_active;
}

void mvtsafe_setMode(const char* mode) {
    if (!mode) return;
    if (strcmp(mode, "idle") == 0) {
        mvtsafe_mode = MVTSAFE_MODE_IDLE;
    } else if (strcmp(mode, "soft") == 0) {
        mvtsafe_mode = MVTSAFE_MODE_SOFT;
    } else if (strcmp(mode, "hard") == 0) {
        mvtsafe_mode = MVTSAFE_MODE_HARD;
    } else {
        // unknown -> ignore
        sendError("mvtSafe", "mode", "*", "unknown");
        return;
    }
    // ack
    char buf[16];
    snprintf(buf, sizeof(buf), "%s", mode);
    _send_vpiv_state("mode", buf);
}

void mvtsafe_setThresholds(int warn_cm, int danger_cm) {
    if (warn_cm < 1) warn_cm = 1;
    if (danger_cm < 0) danger_cm = 0;
    // ensure warn >= danger
    if (warn_cm < danger_cm) warn_cm = danger_cm;
    mvtsafe_warn_cm = warn_cm;
    mvtsafe_danger_cm = danger_cm;
    char buf[32];
    snprintf(buf, sizeof(buf), "%d,%d", mvtsafe_warn_cm, mvtsafe_danger_cm);
    _send_vpiv_state("thd", buf);
}

void mvtsafe_setMaxScale(float s) {
    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    mvtsafe_maxScale = s;
    char buf[16];
    dtostrf(mvtsafe_maxScale, 0, 2, buf);
    _send_vpiv_state("scale", buf);
}

int mvtsafe_getState() {
    return mvtsafe_state;
}

// Periodic non-blocking process
void mvtsafe_process() {
    if (!mvtsafe_active) return;
    unsigned long now = millis();
    if (now - mvtsafe_lastTs < MVTSAFE_PERIOD_MS) return;
    mvtsafe_lastTs = now;

    // Evaluate global state from sensors
    int newState = _evaluate_global_state();

    // Hysteresis on state transitions:
    // - To leave DANGER, require all sensors > danger + HYSTERESIS
    // - To leave WARN, require all sensors > warn + HYSTERESIS
    bool keepDanger = false;
    bool keepWarn = false;

    if (mvtsafe_state == MVTSAFE_STATE_DANGER) {
        // check if still danger
        for (int i = 0; i < US_NUM_SENSORS; ++i) {
            int v = us_peekCurrValue(i);
            if (v > 0 && v <= (mvtsafe_danger_cm + HYSTERESIS_CM)) {
                keepDanger = true;
                break;
            }
        }
        if (keepDanger) {
            newState = MVTSAFE_STATE_DANGER;
        } else {
            // demote to WARN or OK depending on values
            newState = _evaluate_global_state();
        }
    } else if (mvtsafe_state == MVTSAFE_STATE_WARN) {
        for (int i = 0; i < US_NUM_SENSORS; ++i) {
            int v = us_peekCurrValue(i);
            if (v > 0 && v <= (mvtsafe_warn_cm + HYSTERESIS_CM)) {
                keepWarn = true;
                break;
            }
        }
        if (keepWarn) newState = MVTSAFE_STATE_WARN;
        else newState = _evaluate_global_state();
    }

    // React to newState (edge-sensitive actions)
    if (newState != mvtsafe_state) {
        // state changed
        if (newState == MVTSAFE_STATE_OK) {
            _send_vpiv_state("state", "OK");
            // optionally restore normal operation (no action required)
        } else if (newState == MVTSAFE_STATE_WARN) {
            _send_vpiv_state("state", "WARN");
            // In soft mode, reduce speed proportionally
            if (mvtsafe_mode == MVTSAFE_MODE_SOFT) {
                _apply_soft_reduction();
            }
        } else if (newState == MVTSAFE_STATE_DANGER) {
            _send_vpiv_state("state", "DANGER");
            if (mvtsafe_mode == MVTSAFE_MODE_HARD) {
                _apply_hard_stop();
            } else if (mvtsafe_mode == MVTSAFE_MODE_SOFT) {
                // strong reduction and send alert
                mvtsafe_setMaxScale(mvtsafe_maxScale * 0.5f); // further reduce
                _apply_soft_reduction();
                sendInfo("mvtSafe", "alert", "*", "danger_soft");
            }
        }
        mvtsafe_state = newState;
    } else {
        // still same state -> periodic actions for persistent WARN in SOFT mode
        if (mvtsafe_state == MVTSAFE_STATE_WARN && mvtsafe_mode == MVTSAFE_MODE_SOFT) {
            // keep applying reduced targets to be safe
            _apply_soft_reduction();
        }
    }

    // always publish a lightweight status periodically
    static unsigned long lastPub = 0;
    if (now - lastPub >= 1000UL) {
        char buf[64];
        snprintf(buf, sizeof(buf), "s:%d,w:%d,d:%d,scale:%.2f", mvtsafe_state, mvtsafe_warn_cm, mvtsafe_danger_cm, mvtsafe_maxScale);
        sendInfo("mvtSafe", "status", "*", buf);
        lastPub = now;
    }
}
