/***********************************************************************
 * FICHIER : src/control/ctrl_L.h
 *
 * MODULE  : ctrl_L  (contrôle par laisse)
 *
 * RÔLE :
 *   Pilotage local du robot par laisse physique.
 *
 *   - Orientation :
 *       assurée exclusivement par le capteur de force (SF)
 *       → déséquilibre gauche / droite des moteurs
 *
 *   - Distance utilisateur / robot :
 *       régulée par les capteurs ultrason (US)
 *       → adaptation progressive de la vitesse longitudinale
 *
 * CONDITIONS D’ACTIVATION :
 *   - cfg_typePtge == 3  (laisse)
 *   - cfg_typePtge == 4  (laisse + vocal)
 *
 * PRIORITÉS :
 *   - urgence (Urg) > ctrl_L > tout autre pilotage
 *
 * INTERFACE VPIV (retour info) :
 *   $I:ctrl_L:L:*:OK#     → fonctionnement normal (1 seule émission)
 *   $I:ctrl_L:L:*:FceKO#  → traction excessive sur la laisse
 *   $I:ctrl_L:L:*:VtKO#   → utilisateur trop rapide pour le robot
 *
 ***********************************************************************/

// pragma once, include stdint -> v moderne de ifndef ... define ... endif
#pragma once
#include <stdint.h>
#include <stdbool.h>

/* --------------------------------------------------------------------
 * Initialisation du module ctrl_L
 * - charge les paramètres depuis config
 * - initialise les buffers internes (US)
 * ------------------------------------------------------------------*/
void ctrl_L_init();

/* --------------------------------------------------------------------
 * Boucle de contrôle laisse
 * - appelée périodiquement dans loop()
 * - calcule vitesse + rotation
 * - applique les consignes moteurs
 * ------------------------------------------------------------------*/
void ctrl_L_update();

/* --------------------------------------------------------------------
 * Activation / désactivation explicite du contrôle laisse
 * - désactivation => arrêt moteurs
 * ------------------------------------------------------------------*/
void ctrl_L_setEnabled(bool on);
bool ctrl_L_isEnabled();

/* --------------------------------------------------------------------
 * Paramètres dynamiques (réglables via Node-RED)
 * ------------------------------------------------------------------*/

/* Distance cible utilisateur / robot (mm) */
void ctrl_L_setTargetDistance(uint16_t mm);

/* Vitesse maximale autorisée */
void ctrl_L_setMaxSpeed(int16_t v);

/* Rotation maximale autorisée (différentiel moteurs) */
void ctrl_L_setMaxTurn(int16_t w);

/* Taille de la fenêtre de moyenne glissante US */
void ctrl_L_setUsWindow(uint8_t n);

/* Zone morte autour de la distance cible (mm)
 * → évite oscillations et micro-corrections
 */
void ctrl_L_setDeadZone(uint16_t mm);

/* --------------------------------------------------------------------
 * Mode test :
 * - calcule les consignes
 * - n’applique pas les moteurs
 * - utilisé pour calibration
 * ------------------------------------------------------------------*/
void ctrl_L_setTestMode(bool on);

/* --------------------------------------------------------------------
 * Récupération de l’état actuel du contrôle laisse
 * - "OK" : fonctionnement normal
 * - "FceKO" : traction excessive sur la laisse
 * ------------------------------------------------------------------*/
const char *ctrl_L_getStatus(); // Nouvelle fonction et non void pour renvoyer l'état détaillé
