/************************************************************
 * FICHIER  : src/control/ctrl_L.h
 * CHEMIN   : arduino/mega/src/control/ctrl_L.h
 * VERSION  : 1.1  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Interface publique du module de contrôle par laisse (ctrl_L).
 *   À inclure dans main.cpp et dispatch_Ctrl_L.cpp.
 *
 * RÉSUMÉ DU FONCTIONNEMENT
 * ------------------------
 *   Le robot suit l'utilisateur en maintenant une distance cible :
 *     - DISTANCE  → régulée par les capteurs US arrière
 *       Si l'utilisateur s'éloigne : robot accélère
 *       Si l'utilisateur se rapproche : robot ralentit ou recule
 *       Zone morte : pas de correction dans ±dead_zone_mm
 *
 *     - ORIENTATION → régulée par le capteur de force (FS / laisse physique)
 *       La traction sur la laisse oriente le robot dans la direction de l'utilisateur
 *       Zone morte ±5 : évite les micro-rotations
 *
 * CONDITIONS D'ACTIVATION
 * -----------------------
 *   cfg_typePtge == 3 (laisse seul)
 *   cfg_typePtge == 4 (laisse + vocal)
 *   → Activation automatique depuis dispatch_CfgS.cpp
 *
 * INTERFACE VPIV PUBLIÉE (A->SP)
 * --------------------------------
 *   $I:ctrl_L:status:*:OK#      — fonctionnement normal
 *   $I:ctrl_L:status:*:OFF#     — module désactivé
 *   $I:ctrl_L:status:*:FceKO#   — traction excessive sur la laisse
 *   $I:ctrl_L:status:*:VtKO#    — utilisateur trop rapide (non impl. pour l'instant)
 *   Publié uniquement si l'état change (anti-spam).
 *
 * USAGE DANS loop()
 * ------------------
 *   ctrl_L_update() DOIT être appelé à chaque tour de loop().
 *   Sans cet appel, le contrôle est inopérant.
 ************************************************************/

#pragma once
#include <stdint.h>
#include <stdbool.h>

/* ================================================================
 * INITIALISATION
 *   - Charge les paramètres depuis config.cpp
 *   - Initialise le buffer de moyenne glissante US
 *   - Appeler une seule fois depuis setup()
 * ================================================================ */
void ctrl_L_init();

/* ================================================================
 * BOUCLE DE CONTRÔLE LAISSE
 *   - Appeler à chaque tour de loop() (non-bloquant)
 *   - Cadence interne : 200 ms (PERIOD_MS)
 *   - Calcule distance + orientation → commande moteurs
 * ================================================================ */
void ctrl_L_update();

/* ================================================================
 * ACTIVATION / DÉSACTIVATION
 *   - Désactivation → arrêt immédiat des moteurs
 * ================================================================ */
void ctrl_L_setEnabled(bool on);
bool ctrl_L_isEnabled();

/* ================================================================
 * PARAMÈTRES DYNAMIQUES — modifiables via VPIV ($V:Ctrl_L:*:...:...#)
 * ================================================================ */

/* Distance cible utilisateur/robot (mm)
 * Valeur de référence pour la régulation — dist_mesurée doit rester ≈ target */
void ctrl_L_setTargetDistance(uint16_t mm);

/* Vitesse longitudinale maximale autorisée (avant/arrière)
 * Limite le gain de la régulation — adapter selon l'environnement */
void ctrl_L_setMaxSpeed(int16_t v);

/* Rotation maximale autorisée (différentiel moteurs gauche/droite) */
void ctrl_L_setMaxTurn(int16_t w);

/* Taille de la fenêtre de moyenne glissante US (1..10 mesures)
 * Plus grande → réponse plus douce, moins réactive aux variations brusques */
void ctrl_L_setUsWindow(uint8_t n);

/* Zone morte autour de la distance cible (mm)
 * Dans cette zone : speed = 0 → pas de correction
 * Évite oscillations et micro-corrections continues */
void ctrl_L_setDeadZone(uint16_t mm);

/* ================================================================
 * MODE TEST
 *   - Calcule les consignes vitesse/rotation
 *   - N'envoie PAS les consignes aux moteurs
 *   - Utile pour calibration sans mouvement du robot
 * ================================================================ */
void ctrl_L_setTestMode(bool on);

/* ================================================================
 * ÉTAT DU CONTRÔLE LAISSE
 *   "OFF"    : module inactif
 *   "OK"     : fonctionnement normal
 *   "FceKO"  : force excessive sur la laisse
 * ================================================================ */
const char *ctrl_L_getStatus();