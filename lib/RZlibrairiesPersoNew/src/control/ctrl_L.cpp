/************************************************************
 * FICHIER  : src/control/ctrl_L.cpp
 * CHEMIN   : arduino/mega/src/control/ctrl_L.cpp
 * VERSION  : 1.1  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Contrôle du robot en mode "pilotage par laisse".
 *   Le robot suit l'utilisateur en maintenant une distance cible
 *   et en s'orientant dans sa direction.
 *
 * PRINCIPE GÉNÉRAL
 * ----------------
 *   DISTANCE (vitesse longitudinale) — régulée par les capteurs US arrière :
 *     dist_mesurée = moyenne glissante des 4 capteurs US arrière (mm)
 *     delta        = dist_mesurée - dist_cible
 *
 *     Si abs(delta) <= dead_zone → pas de correction (zone morte)
 *                                   évite les micro-corrections et oscillations
 *     Sinon :
 *       speed = constrain(delta / 10, -max_speed, max_speed)
 *       → proportionnel à l'écart : plus l'utilisateur s'éloigne, plus le robot accélère
 *       → si l'utilisateur se rapproche (delta négatif), le robot ralentit voire recule
 *       → la division par 10 est un facteur de proportionnalité arbitraire (à calibrer)
 *
 *   ORIENTATION (rotation) — régulée par le capteur de force (FS) :
 *     La laisse physique exerce une force sur le capteur.
 *     Si force > +5  → traction vers la droite → dir = +1
 *     Si force < -5  → traction vers la gauche → dir = -1
 *     Sinon          → pas de rotation (zone morte ±5)
 *     turn = constrain(dir × |force|, -max_turn, max_turn)
 *     → proportionnel à la force : plus la traction est forte, plus le robot tourne
 *
 *   COMMANDE MOTEURS :
 *     mtr_setTargetsSigned(speed, turn, 1)
 *     speed = vitesse longitudinale (avant/arrière)
 *     turn  = vitesse de rotation (différentiel gauche/droite)
 *
 * CONDITIONS D'ACTIVATION
 * -----------------------
 *   cfg_typePtge == 3 : mode laisse seul
 *   cfg_typePtge == 4 : mode laisse + vocal
 *   → ctrl_L_setEnabled(true) appelé automatiquement par dispatch_CfgS.cpp
 *
 * PROPRIÉTÉS VPIV REÇUES (SP->A, via dispatch_Ctrl_L.cpp)
 * ---------------------------------------------------------
 *   act     (V) : active/désactive le contrôle laisse
 *   dist    (V) : distance cible à maintenir avec l'utilisateur (mm) [CONFIG]
 *   vmax    (V) : vitesse longitudinale maximale autorisée
 *   dead    (V) : zone morte autour de la distance cible (mm)
 *   [optionnel] : une propriété distMes (F) A->SP pourrait publier la distance
 *                 mesurée en temps réel — à ajouter si SP en a besoin
 *
 * PROPRIÉTÉS VPIV PUBLIÉES (A->SP)
 * ----------------------------------
 *   status  (I) : état du contrôle — "OK", "OFF", "FceKO", "VtKO"
 *                 Envoyé uniquement si l'état change (anti-spam)
 *
 * ARTICULATION
 * ------------
 *   dispatch_Ctrl_L.cpp → ctrl_L.cpp
 *   ctrl_L.cpp consomme :
 *     us_peekCurrValue()    → distance mesurée (sensors/us.h)
 *     fs_lastForce()        → force laisse (sensors/fs.h)
 *     mtr_setTargetsSigned()→ commande moteurs (actuators/mtr.h)
 *     urg_isActive()        → vérification urgence (system/urg.h)
 *
 * PRÉCAUTIONS
 * -----------
 *   - ctrl_L_update() doit être appelé à chaque tour de loop().
 *   - Si une urgence est active, les moteurs sont stoppés et ctrl_L est suspendu.
 *   - Le mode test (ctrl_L_setTestMode) calcule sans envoyer aux moteurs.
 ************************************************************/

#include <Arduino.h>
#include "config/config.h"
#include "control/ctrl_L.h"
#include "system/urg.h"
#include "actuators/mtr.h"
#include "sensors/fs.h"
#include "sensors/us.h"
#include "communication/communication.h"

/* ----------------------------------------------------------------
 * État interne du module
 * ---------------------------------------------------------------- */
static bool ctrlL_enabled = false;  // module actif ou non
static bool ctrlL_test = false;     // mode test : calcule sans commander les moteurs
static char last_status[8] = "OFF"; // dernier état publié (anti-spam)

/* ----------------------------------------------------------------
 * Paramètres dynamiques — modifiables via VPIV ($V:Ctrl_L:*:dist:xxx#)
 * ---------------------------------------------------------------- */
static uint16_t target_dist_mm; // distance cible utilisateur/robot (mm)
static int16_t max_speed;       // vitesse longitudinale max (avant/arrière)
static int16_t max_turn;        // rotation max (différentiel moteurs)
static uint8_t us_window;       // taille fenêtre moyenne glissante US (1..10)
static uint16_t dead_zone_mm;   // zone morte en distance (mm) — pas de correction dedans

/* ----------------------------------------------------------------
 * Temporisation boucle de contrôle
 * ---------------------------------------------------------------- */
static uint32_t last_update_ms = 0;
static const uint16_t PERIOD_MS = 200; // période de contrôle : 200 ms = 5 Hz

/* ----------------------------------------------------------------
 * Capteurs US arrière utilisés pour mesurer la distance utilisateur
 * Indices dans le tableau us_peekCurrValue() — à adapter selon câblage
 * ---------------------------------------------------------------- */
static const uint8_t us_idx[4] = {5, 4, 3, 8};

/* ----------------------------------------------------------------
 * Buffer de moyenne glissante pour la distance US
 * Lisse les variations brusques et évite les fausses corrections
 * ---------------------------------------------------------------- */
static uint16_t us_buf[10];
static uint8_t us_pos = 0;

/* ----------------------------------------------------------------
 * ctrl_L_getStatus() — retourne l'état courant du contrôle laisse
 *
 * Valeurs possibles :
 *   "OFF"    : contrôle inactif
 *   "FceKO"  : traction trop forte sur la laisse (utilisateur tire fort)
 *              → seuil : cfg_fs_threshold × 1.5
 *   "OK"     : fonctionnement normal
 *
 * NOTE : "VtKO" (utilisateur trop rapide) serait à détecter si le robot
 * est à la vitesse max depuis un certain temps — non implémenté pour l'instant.
 * ---------------------------------------------------------------- */
const char *ctrl_L_getStatus()
{
    if (!ctrlL_enabled)
        return "OFF";

    int16_t force = (int16_t)fs_lastForce();
    if (abs(force) > cfg_fs_threshold * 1.5)
        return "FceKO"; // force excessive → alerte laisse

    return "OK";
}

/* ================================================================
 * INITIALISATION — appeler depuis setup() via ctrl_L_init()
 * ================================================================ */
void ctrl_L_init()
{
    ctrlL_enabled = false;
    ctrlL_test = false;
    strncpy(last_status, "OFF", sizeof(last_status));

    // Charge les valeurs depuis la config (définies dans config.cpp)
    target_dist_mm = cfg_laisse_target_dist_mm;
    max_speed = cfg_laisse_max_speed;
    max_turn = cfg_laisse_max_turn;
    us_window = constrain(cfg_laisse_us_window, 1, 10);
    dead_zone_mm = cfg_laisse_dead_zone_mm;

    // Initialise le buffer US à la distance cible (évite une correction au démarrage)
    for (uint8_t i = 0; i < us_window; i++)
        us_buf[i] = target_dist_mm;
}

/* ================================================================
 * ACTIVATION / DÉSACTIVATION
 * ================================================================ */
void ctrl_L_setEnabled(bool on)
{
    ctrlL_enabled = on;
    if (!on)
    {
        mtr_setTargetsSigned(0, 0, 1); // arrêt moteurs à la désactivation
        strncpy(last_status, "OFF", sizeof(last_status));
    }
}

/* ================================================================
 * SETTERS PARAMÈTRES DYNAMIQUES (appelés par dispatch_Ctrl_L.cpp)
 * ================================================================ */
void ctrl_L_setTargetDistance(uint16_t mm) { target_dist_mm = mm; }
void ctrl_L_setMaxSpeed(int16_t v) { max_speed = abs(v); }
void ctrl_L_setMaxTurn(int16_t w) { max_turn = abs(w); }
void ctrl_L_setUsWindow(uint8_t n) { us_window = constrain(n, 1, 10); }
void ctrl_L_setDeadZone(uint16_t mm) { dead_zone_mm = mm; }
void ctrl_L_setTestMode(bool on) { ctrlL_test = on; }
bool ctrl_L_isEnabled() { return ctrlL_enabled; }

/* ================================================================
 * BOUCLE PRINCIPALE — appeler depuis loop()
 *
 * Cadence : PERIOD_MS (200 ms)
 * 1. Lit les capteurs US arrière → distance mesurée (moyenne glissante)
 * 2. Calcule la vitesse proportionnelle à l'écart distance cible
 * 3. Lit le capteur de force → direction de traction
 * 4. Calcule la rotation proportionnelle à la force
 * 5. Commande les moteurs (si non mode test)
 * 6. Publie le status si changement d'état
 * ================================================================ */
void ctrl_L_update()
{
    uint32_t now = millis();
    if (now - last_update_ms < PERIOD_MS)
        return; // pas encore le moment d'agir
    last_update_ms = now;

    /* --- Vérifications de sécurité --- */
    if (!ctrlL_enabled || urg_isActive())
    {
        mtr_setTargetsSigned(0, 0, 1); // urgence ou désactivé → arrêt
        return;
    }

    /* --- Vérification mode de pilotage actif ---
     * Le contrôle laisse n'est actif qu'en typePtge 3 (laisse) ou 4 (laisse+vocal) */
    if (!(cfg_typePtge == 3 || cfg_typePtge == 4))
    {
        ctrl_L_setEnabled(false); // mode changé → désactiver
        return;
    }

    /* ============================================================
     * CALCUL DE DISTANCE — régulation vitesse longitudinale
     *
     * Lit les 4 capteurs US arrière (cm → conversion mm × 10).
     * Filtre les valeurs invalides (v <= 0 = pas de mesure).
     * Calcule la moyenne sur les mesures valides.
     * ============================================================ */
    uint32_t sum = 0;
    uint8_t valid = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        int v = us_peekCurrValue(us_idx[i]); // valeur US en cm
        if (v > 0)
        {
            sum += (uint32_t)v * 10; // cm → mm (× 10)
            valid++;
        }
    }

    if (valid == 0)
        return; // aucun capteur ne répond → pas de correction possible

    /* Moyenne instantanée des capteurs valides */
    uint16_t avg = sum / valid;

    /* Moyenne glissante sur 'us_window' mesures — lisse les variations */
    us_buf[us_pos++] = avg;
    if (us_pos >= us_window)
        us_pos = 0; // buffer circulaire

    uint32_t fsum = 0;
    for (uint8_t i = 0; i < us_window; i++)
        fsum += us_buf[i];
    uint16_t dist_mm = fsum / us_window; // distance lissée en mm

    /* ============================================================
     * RÉGULATION PROPORTIONNELLE EN VITESSE
     *
     * delta = dist_mesurée - dist_cible
     *   delta > 0 : utilisateur s'éloigne → robot accélère (speed > 0)
     *   delta < 0 : utilisateur se rapproche → robot ralentit/recule (speed < 0)
     *   |delta| <= dead_zone : dans la zone morte → speed = 0 (pas de correction)
     *
     * Le facteur / 10 est un gain proportionnel empirique.
     * Pour un delta de 100 mm → speed ≈ 10 unités.
     * À ajuster via max_speed pour limiter la vitesse maximale.
     * ============================================================ */
    int16_t speed = 0;
    int16_t delta = (int16_t)dist_mm - (int16_t)target_dist_mm;

    if (abs(delta) > dead_zone_mm)
        speed = constrain(delta / 10, -max_speed, max_speed);
    // else speed reste 0 : on est dans la zone morte, pas de correction

    /* ============================================================
     * CALCUL ORIENTATION — régulation rotation par capteur de force
     *
     * La force exercée sur la laisse indique la direction de l'utilisateur.
     * Zone morte ±5 : évite les micro-rotations dues au bruit capteur.
     * Turn = dir × |force| limité à max_turn
     *   dir = +1 → tourne droite
     *   dir = -1 → tourne gauche
     * ============================================================ */
    int16_t force = (int16_t)fs_lastForce();
    int8_t dir = 0;

    if (force > 5)
        dir = 1; // traction droite
    else if (force < -5)
        dir = -1; // traction gauche
    // sinon dir = 0 : laisse droite → pas de rotation

    int16_t turn = constrain(dir * abs(force), -max_turn, max_turn);

    /* ============================================================
     * COMMANDE MOTEURS
     * En mode test : calcule mais n'envoie pas (pour calibration/debug)
     * ============================================================ */
    if (!ctrlL_test)
        mtr_setTargetsSigned(speed, turn, 1);

    /* ============================================================
     * PUBLICATION STATUS (anti-spam : seulement si changement)
     * ============================================================ */
    const char *new_status = ctrl_L_getStatus();
    if (strcmp(last_status, new_status) != 0)
    {
        strncpy(last_status, new_status, sizeof(last_status) - 1);
        sendInfo("ctrl_L", "status", "*", new_status);
    }
}