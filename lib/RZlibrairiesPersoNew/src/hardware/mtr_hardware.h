/************************************************************
 * FICHIER  : src/hardware/mtr_hardware.h
 * CHEMIN   : arduino/mega/src/hardware/mtr_hardware.h
 * VERSION  : 2.0  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Interface bas-niveau du module moteurs.
 *   Gère la construction et l'envoi des trames vers l'Arduino UNO
 *   via la liaison série (MOTEURS_SERIAL).
 *
 * FORMAT DE TRAME (vers UNO)
 * ---------------------------
 *   "<L,R,A>\n"
 *     L : vitesse roue gauche, signée, en unités moteur [-400..+400]
 *     R : vitesse roue droite, signée, en unités moteur [-400..+400]
 *     A : niveau de lissage accélération [0..4]
 *
 *   Exemples :
 *     <320,320,2>\n   → avance droit, lissage moyen
 *     <-160,160,2>\n  → tourne droite sur place
 *     <0,0,0>\n       → arrêt immédiat
 *
 * DÉTECTION CÔTÉ UNO
 * -------------------
 *   L'UNO détecte automatiquement si la trame est "modern" :
 *   si L ou R sont signés (contiennent '-') ou > 9, c'est du modern.
 *   Les trames modern sont toujours > 9 en valeur absolue ici.
 *
 * HOOK URGENCE
 * ------------
 *   urg_stopAllMotors() est déclaré ici et implémenté dans ce fichier.
 *   Il est appelé par urg.cpp lors d'un déclenchement d'urgence.
 *   Ce hook agit directement sur le hardware, sans passer par mtr.cpp,
 *   pour garantir l'arrêt même si mtr.cpp est dans un état incohérent.
 *
 * ARTICULATION
 * ------------
 *   mtr.cpp → mtr_hardware.h / mtr_hardware.cpp → MOTEURS_SERIAL → UNO
 *   urg.cpp → urg_stopAllMotors() → mtr_hardware.cpp
 *
 * PRÉCAUTIONS
 * -----------
 *   - Ne pas appeler ces fonctions directement depuis l'extérieur de mtr.cpp
 *     (sauf urg_stopAllMotors() qui est un hook réservé à urg.cpp)
 *   - MOTEURS_SERIAL doit être initialisé avant mtr_hw_init()
 ************************************************************/

#ifndef MTR_HARDWARE_H
#define MTR_HARDWARE_H

#include <Arduino.h>
#include "config/config.h"

/* Buffer de la trame courante — format ASCII "<L,R,A>" */
extern char mtr_trameBuf[32];

/* ================================================================
 * INITIALISATION — appeler depuis mtr_init() / setup()
 * ================================================================ */
void mtr_hw_init();

/* ================================================================
 * CONSTRUCTION DE LA TRAME
 *   Remplit mtr_trameBuf avec "<L,R,A>".
 *   L, R : [-outputScale..+outputScale]
 *   A    : [0..4]
 * ================================================================ */
void mtr_hw_updateTrame(int L, int R, int A);

/* ================================================================
 * ENVOI DE LA TRAME
 *   Envoie mtr_trameBuf sur MOTEURS_SERIAL suivi d'un '\n'.
 *   Appeler mtr_hw_updateTrame() avant.
 * ================================================================ */
void mtr_hw_send();

/* ================================================================
 * HOOK URGENCE — réservé à urg.cpp
 *   Arrêt matériel immédiat, sans passer par mtr.cpp.
 *   Envoie <0,0,0> directement sur la liaison série.
 * ================================================================ */
void mtr_hw_forceUrgStop();
void urg_stopAllMotors(); // appelé par urg.cpp sur déclenchement urgence

#endif // MTR_HARDWARE_H