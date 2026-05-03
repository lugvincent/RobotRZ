/************************************************************
 * FICHIER : src/communication/dispatch_odom.cpp
 * ROLE    : Dispatcher VPIV pour le module Odométrie (Odo)
 *
 * Propriétés supportées (propAbbr) :
 *   - "read"   : read type (pos|vel|raw) -> $V:Odo:*:read:pos#
 *   - "reset"  : reset pose -> value "1"
 *   - "act"    : 1/0 activer ou désactiver odom
 *   - "freq"   : période calcul ODOM_period_ms (ms)
 *   - "report" : période d'envoi (ms) ou "1" pour activer default
 *   - "coeff"  : coeffGLong,coeffDLong,gAngl,dAngl  (valeurs séparées par ,)
 *   - "gyro"   : "<deg/s>,<timestamp_ms>"  (valeur fournie par SE via Tasker)
 *   - "compass": "<deg>,<quality>"  (SE boussole)
 *   - "mode"   : idle|normal|surv (mapping interne)
 *
 * Exemples :
 *   $V:Odo:*:read:pos#
 *   $V:Odo:*:reset:1#
 *   $V:Odo:*:coeff:0.0712,0.0712,0.08035,0.08035#
 *   $V:Odo:*:gyro:12.4,170932#
 * vérif hypothèse à faire : coefficients sont en unités m/tick 
 * et rad/tick (vérifier et adapte si tes coefficients sont en mm/tick
 * 
 ************************************************************/
/************************************************************
 * FICHIER : src/sensors/odom.cpp
 * ROLE    : Implémentation ODOM v2 (unités : mètres / radians)
 * maintenant la conversion en degré ds dispatch
 ************************************************************/

#include "odom.h"
#include "../hardware/odom_hardware.h"
#include "../configuration/config.h"
#include "../communication/communication.h"
#include <Arduino.h>
#include <math.h>

// état interne (pose en mètres, orientation en radians)
static double xR = 0.0, yR = 0.0, orientation = 0.0; // orientation en rad
static unsigned long lastComputeTs = 0;
static unsigned long lastReportTs = 0;

// vitesses estimées
static double last_v = 0.0;    // m/s
static double last_omega = 0.0; // rad/s

// coefficients locaux (copie depuis config mais recalculables)
static double coeffGLong = 0.0;
static double coeffDLong = 0.0;
static double coeffGAngl = 0.0;
static double coeffDAngl = 0.0;

// Fonction utilitaire : (re)calcule les coeffs à partir des paramètres physiques
static void _recompute_coeffs_from_physical() {
    // lecture des paramètres configurables
    double D = cfg_odo_wheel_diameter_m;
    double track = cfg_odo_track_m;
    int cpr = cfg_odo_encoder_cpr;

    if (D <= 0.0 || track <= 0.0 || cpr <= 0) {
        // garde les coefficients déjà présents (sécurité), ne crash pas
        coeffGLong = cfg_odo_coeff_gLong;
        coeffDLong = cfg_odo_coeff_dLong;
        coeffGAngl = cfg_odo_coeff_gAngl;
        coeffDAngl = cfg_odo_coeff_dAngl;
        return;
    }

    double circ = M_PI * D; // m
    double dist_per_tick = circ / (double)cpr; // m / tick

    coeffGLong = dist_per_tick;
    coeffDLong = dist_per_tick;

    // angular contribution per meter = 1/track (rad per meter) => per tick multiply by dist_per_tick
    coeffDAngl = 1.0 / track;
    coeffGAngl = 1.0 / track;

    // mirror back to global config so dispatch/read sees updated values
    cfg_odo_coeff_gLong = coeffGLong;
    cfg_odo_coeff_dLong = coeffDLong;
    cfg_odo_coeff_gAngl = coeffGAngl;
    cfg_odo_coeff_dAngl = coeffDAngl;
}

// Initialisation
void odom_init() {
    odom_hw_init();

    // recalculer coefficients à partir des paramètres physiques (config variables)
    _recompute_coeffs_from_physical();

    lastComputeTs = millis();
    lastReportTs = millis();
    xR = 0.0; yR = 0.0; orientation = 0.0;
    last_v = 0.0; last_omega = 0.0;
}

// Reset pose
void odom_resetPose() {
    xR = 0.0; yR = 0.0; orientation = 0.0;
    odom_hw_forceClear();
    lastComputeTs = millis();
    lastReportTs = millis();
    cfg_odom_pendingReport = true;
}

void odom_processPeriodic() {
    if (!cfg_odom_active) return;

    unsigned long now = millis();

    // gestion du report indépendant du calcul
    if (cfg_odom_report_period_ms > 0 && now - lastReportTs >= cfg_odom_report_period_ms) {
        cfg_odom_pendingReport = true;
    }

    if (now - lastComputeTs < cfg_odom_period_ms) return;

    // récupérer delta ticks depuis dernier getAndClear
    long dG = odom_hw_getAndClearLeftTicks();
    long dD = odom_hw_getAndClearRightTicks();

    // si aucun tick, on met à jour timestamps et quitte (mais on peut toujours envoyer report)
    if (dG == 0 && dD == 0) {
        lastComputeTs = now;
        // report possible géré plus bas
    } else {
        // conversion en distance (m)
        double distG = (double)dG * coeffGLong; // m
        double distD = (double)dD * coeffDLong; // m

        // delta distance et delta angle (rad)
        double dDist = (distG + distD) / 2.0;
        double dAng = (distD * coeffDAngl) - (distG * coeffGAngl); // rad

        // mise à jour de la pose (approximation odom diff)
        double midOrient = orientation + dAng * 0.5;
        xR += dDist * cos(midOrient);
        yR += dDist * sin(midOrient);
        orientation += dAng;

        // normaliser orientation entre -PI..+PI (optionnel mais utile)
        if (orientation > M_PI) orientation -= 2.0 * M_PI;
        if (orientation <= -M_PI) orientation += 2.0 * M_PI;

        // vitesse estimée
        double dt = (now - lastComputeTs) / 1000.0; // s
        if (dt <= 0.0) dt = 1e-6;
        last_v = dDist / dt;      // m/s
        last_omega = dAng / dt;   // rad/s

        lastComputeTs = now;
    }

    // Trigger report si demandé
    if (cfg_odom_pendingReport || (cfg_odom_report_period_ms > 0 && now - lastReportTs >= cfg_odom_report_period_ms)) {
        char buf[256];
        // orientation en degrés pour affichage ; pos en mètres
        snprintf(buf, sizeof(buf), "pos:%.3f,%.3f,%.2f;vel:%.3f,%.3f",
                 xR, yR, orientation * 180.0 / M_PI,
                 last_v, last_omega);
        sendInfo("Odo", "report", "*", buf);
        cfg_odom_pendingReport = false;
        lastReportTs = now;
    }
}

// Accès pose
void odom_getPose(double* out_x, double* out_y, double* out_theta) {
    if (out_x) *out_x = xR;
    if (out_y) *out_y = yR;
    if (out_theta) *out_theta = orientation;
}

// Activation
void odom_setActive(bool on) {
    cfg_odom_active = on;
}

// Demande de report ponctuel
void odom_requestReport() {
    cfg_odom_pendingReport = true;
}

// Forcer coefficients (manuel) — si appelé, on garde ces valeurs et on met à jour config
void odom_setCoefficients(double gLong_m_per_tick, double dLong_m_per_tick,
                          double gAngl_rad_per_tick, double dAngl_rad_per_tick) {
    coeffGLong = gLong_m_per_tick;
    coeffDLong = dLong_m_per_tick;
    coeffGAngl = gAngl_rad_per_tick;
    coeffDAngl = dAngl_rad_per_tick;

    cfg_odo_coeff_gLong = coeffGLong;
    cfg_odo_coeff_dLong = coeffDLong;
    cfg_odo_coeff_gAngl = coeffGAngl;
    cfg_odo_coeff_dAngl = coeffDAngl;
}

// Réception données externes
void odom_receiveGyro(float gyro_z_dps, unsigned long ts_ms) {
    cfg_odo_lastGyro_dps = gyro_z_dps;
    cfg_odo_lastGyro_ts_ms = ts_ms;
    // futur : intégrer pour fusion
}
void odom_receiveCompass(float heading_deg, int quality) {
    cfg_odo_lastCompass_deg = heading_deg;
    cfg_odo_lastCompass_quality = quality;
    // futur : corriger orientation si qualité élevée
}
