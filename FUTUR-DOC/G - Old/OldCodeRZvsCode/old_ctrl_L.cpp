/***********************************************************************
 * FICHIER : src/control/ctrl_L.cpp
 *
 * MODULE  : ctrl_L  (contrôle par laisse)
 *
 * LOGIQUE GÉNÉRALE :
 *   - orientation → capteur de force (FS)
 *   - vitesse     → distance utilisateur (US)
 ***********************************************************************/
#include <Arduino.h>
#include "config/config.h"
#include "control/ctrl_L.h"
#include "system/urg.h"
#include "actuators/mtr.h"
#include "sensors/fs.h"
#include "sensors/us.h"
#include "communication/communication.h"

/* État interne */
static bool ctrlL_enabled = false;
static bool ctrlL_test = false;
static char last_status[8] = "OFF"; // Stockage du dernier état envoyé

/* Paramètres dynamiques */
static uint16_t target_dist_mm;
static int16_t max_speed;
static int16_t max_turn;
static uint8_t us_window;
static uint16_t dead_zone_mm;

/* Temporisation */
static uint32_t last_update_ms = 0;
static const uint16_t PERIOD_MS = 200;

/* Capteurs US utilisés */
static const uint8_t us_idx[4] = {5, 4, 3, 8};

/* Moyenne glissante */
static uint16_t us_buf[10];
static uint8_t us_pos = 0;

/* --------------------------------------------------------------------
 * Récupération de l'état actuel du contrôle laisse
 * - "OK" : fonctionnement normal
 * - "FceKO" : traction excessive sur la laisse
 * - "VtKO" : utilisateur trop rapide pour le robot
 * ------------------------------------------------------------------*/
const char *ctrl_L_getStatus()
{
    if (!ctrlL_enabled)
        return "OFF";

    // Vérifier d'abord la force excessive
    int16_t force = (int16_t)fs_lastForce();
    if (abs(force) > cfg_fs_threshold * 1.5)
    {
        return "FceKO";
    }

    return "OK";
}

/* --------------------------------------------------------------------
 * Initialisation
 * ------------------------------------------------------------------*/
void ctrl_L_init()
{
    ctrlL_enabled = false;
    ctrlL_test = false;
    strncpy(last_status, "OFF", sizeof(last_status));

    target_dist_mm = cfg_laisse_target_dist_mm;
    max_speed = cfg_laisse_max_speed;
    max_turn = cfg_laisse_max_turn;
    us_window = constrain(cfg_laisse_us_window, 1, 10);
    dead_zone_mm = cfg_laisse_dead_zone_mm;

    for (uint8_t i = 0; i < us_window; i++)
        us_buf[i] = target_dist_mm;
}

/* --------------------------------------------------------------------
 * Activation
 * ------------------------------------------------------------------*/
void ctrl_L_setEnabled(bool on)
{
    ctrlL_enabled = on;
    if (!on)
    {
        mtr_setTargetsSigned(0, 0, 1);
        strncpy(last_status, "OFF", sizeof(last_status));
    }
}

/* --------------------------------------------------------------------
 * Paramètres dynamiques
 * ------------------------------------------------------------------*/
void ctrl_L_setTargetDistance(uint16_t mm) { target_dist_mm = mm; }
void ctrl_L_setMaxSpeed(int16_t v) { max_speed = abs(v); }
void ctrl_L_setMaxTurn(int16_t w) { max_turn = abs(w); }
void ctrl_L_setUsWindow(uint8_t n) { us_window = constrain(n, 1, 10); }
void ctrl_L_setDeadZone(uint16_t mm) { dead_zone_mm = mm; }
void ctrl_L_setTestMode(bool on) { ctrlL_test = on; }

bool ctrl_L_isEnabled() { return ctrlL_enabled; }

/* --------------------------------------------------------------------
 * Boucle principale
 * ------------------------------------------------------------------*/
void ctrl_L_update()
{
    uint32_t now = millis();
    if (now - last_update_ms < PERIOD_MS)
        return;
    last_update_ms = now;

    if (!ctrlL_enabled || urg_isActive())
    {
        mtr_setTargetsSigned(0, 0, 1);
        return;
    }

    if (!(cfg_typePtge == 3 || cfg_typePtge == 4))
    {
        ctrl_L_setEnabled(false);
        return;
    }

    /* Calcul de distance */
    uint32_t sum = 0;
    uint8_t valid = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        int v = us_peekCurrValue(us_idx[i]);
        if (v > 0)
        {
            sum += (uint32_t)v * 10; // cm → mm
            valid++;
        }
    }

    if (valid == 0)
        return;

    uint16_t avg = sum / valid;
    us_buf[us_pos++] = avg;
    if (us_pos >= us_window)
        us_pos = 0;

    uint32_t fsum = 0;
    for (uint8_t i = 0; i < us_window; i++)
        fsum += us_buf[i];
    uint16_t dist_mm = fsum / us_window;

    /* Calcul de vitesse et rotation */
    int16_t speed = 0;
    int16_t delta = dist_mm - target_dist_mm;

    if (abs(delta) > dead_zone_mm)
        speed = constrain(delta / 10, -max_speed, max_speed);

    int16_t force = (int16_t)fs_lastForce();
    int8_t dir = 0;
    if (force > 5)
        dir = 1;
    else if (force < -5)
        dir = -1;

    int16_t turn = constrain(dir * abs(force), -max_turn, max_turn);

    if (!ctrlL_test)
        mtr_setTargetsSigned(speed, turn, 1);

    // Envoi de l'état
    const char *new_status = ctrl_L_getStatus();
    if (strcmp(last_status, new_status) != 0)
    {
        strncpy(last_status, new_status, sizeof(last_status) - 1);
        sendInfo("ctrl_L", "status", "*", new_status);
    }
}
