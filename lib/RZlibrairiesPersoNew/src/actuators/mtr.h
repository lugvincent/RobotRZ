/************************************************************
 * FICHIER  : src/actuators/mtr.h
 * CHEMIN   : arduino/mega/src/actuators/mtr.h
 * VERSION  : 2.0  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Interface publique du module moteurs (Mtr).
 *   Commande les deux moteurs du robot en mode différentiel (modern).
 *   La couche matérielle envoie les consignes à l'Arduino UNO via
 *   liaison série sous la forme de trames <L,R,A>.
 *
 * PRINCIPE DU PILOTAGE DIFFÉRENTIEL
 * -----------------------------------
 *   Deux paramètres de commande signés :
 *     v     : vitesse de translation (avant/arrière)
 *               +100 = avance pleine vitesse
 *                  0 = arrêt en translation
 *               -100 = recule pleine vitesse
 *
 *     omega : vitesse de rotation (gauche/droite)
 *               +100 = rotation droite max
 *                  0 = tout droit
 *               -100 = rotation gauche max
 *
 *   Calcul roues gauche/droite :
 *     L = (v - omega × kturn) × outputScale / inputScale
 *     R = (v + omega × kturn) × outputScale / inputScale
 *
 *   kturn : coefficient d'amplification de la rotation
 *           (réglable via VPIV — défaut : MTR_DEFAULT_KTURN = 0.8)
 *
 *   Exemples :
 *     v=80, omega=0   → avance droit
 *     v=0,  omega=60  → tourne sur place vers la droite
 *     v=60, omega=30  → avance en virant à droite
 *     v=0,  omega=0   → arrêt
 *
 * PARAMÈTRE D'ACCÉLÉRATION (accelLevel)
 * ---------------------------------------
 *   Contrôle la douceur des transitions de vitesse (lissage côté UNO) :
 *     0 = changement direct, sans lissage
 *     1 = lissage léger
 *     2 = lissage moyen (recommandé)
 *     3 = lissage fort
 *     4 = très doux (transitions longues)
 *
 * SÉCURITÉS
 * ----------
 *   mtr_overrideStop() : arrêt forcé immédiat, ignore toute commande
 *                        jusqu'à mtr_clearOverride()
 *   mtr_scaleTargets() : réduit toutes les vitesses (0.0 = arrêt, 1.0 = normal)
 *                        utilisé par mvt_safe.cpp pour ralentir progressivement
 *   mtr_stopAll()      : arrêt propre (remet targets à 0)
 *
 * PROPRIÉTÉS VPIV PUBLIÉES (A->SP)
 * ----------------------------------
 *   targets (I) : confirmation L,R,A après chaque mtr_setTargetsSigned()
 *   stop    (I) : confirmation d'arrêt
 *   scale   (I) : confirmation du facteur de réduction
 *   override(I) : confirmation override "stop" ou "cleared"
 *   kturn   (I) : confirmation du nouveau coefficient kturn
 *   state   (I) : réponse à read (L,R,A courants)
 *
 * PROPRIÉTÉS VPIV REÇUES (SP->A)  — voir dispatch_Mtr.cpp
 * ---------------------------------------------------------
 *   cmd      (V) : "v,omega,accelLevel"  — commande principale
 *   stop     (V) : arrêt propre
 *   override (V) : "stop" ou "clear"
 *   scale    (V) : facteur 0.0..1.0
 *   kturn    (V) : coefficient de rotation
 *   read     (V) : demande l'état courant
 *
 * ARTICULATION
 * ------------
 *   dispatch_Mtr.cpp → mtr.h / mtr.cpp → mtr_hardware.h / mtr_hardware.cpp
 *   mvt_safe.cpp     → mtr_scaleTargets(), mtr_overrideStop(), mtr_stopAll()
 *   ctrl_L.cpp       → mtr_setTargetsSigned()
 *   urg.cpp          → urg_stopAllMotors() hook dans mtr_hardware.cpp
 *
 * PRÉCAUTIONS
 * -----------
 *   - mtr_processPeriodic() DOIT être appelé à chaque tour de loop()
 *     (keepalive 100ms vers l'UNO — le UNO stoppe si plus de trame > 1s)
 *   - cfg_mtr_active doit être true pour que les commandes soient envoyées
 *   - Ne jamais appeler mtr_hw_* directement depuis l'extérieur de mtr.cpp
 ************************************************************/

#ifndef MTR_ACTUATORS_H
#define MTR_ACTUATORS_H

#include <Arduino.h>

/* ================================================================
 * INITIALISATION — appeler depuis setup()
 * ================================================================ */
void mtr_init();

/* ================================================================
 * BOUCLE PÉRIODIQUE — appeler à CHAQUE tour de loop()
 *   Gère le keepalive (renvoi périodique des consignes toutes les
 *   100ms pour éviter le timeout safety côté UNO) et l'application
 *   du facteur de scaling.
 * ================================================================ */
void mtr_processPeriodic();

/* ================================================================
 * COMMANDE PRINCIPALE — pilotage différentiel
 *
 *   v          : translation  (-inputScale..+inputScale)  ex: -100..+100
 *   omega      : rotation     (-inputScale..+inputScale)
 *   accelLevel : lissage UNO  (0..4)  — 0=direct, 4=très doux
 *
 *   Publie $I:Mtr:targets:*:L,R,A# en confirmation.
 * ================================================================ */
void mtr_setTargetsSigned(int v, int omega, int accelLevel);

/* ================================================================
 * ARRÊT PROPRE
 *   Remet v=0, omega=0, envoie trame <0,0,0> à l'UNO.
 *   Publie $I:Mtr:stop:*:OK#
 * ================================================================ */
void mtr_stopAll();

/* ================================================================
 * SÉCURITÉS — utilisées par mvt_safe.cpp
 * ================================================================ */

/* Arrêt forcé immédiat.
 * Ignore toutes les commandes setTargetsSigned jusqu'à clearOverride.
 * Publie $I:Mtr:override:*:stop# */
void mtr_overrideStop();

/* Lève l'arrêt forcé — reprend les commandes normales.
 * Publie $I:Mtr:override:*:cleared# */
void mtr_clearOverride();

/* Facteur de réduction de vitesse (0.0..1.0).
 *   1.0 = vitesse normale
 *   0.5 = toutes les vitesses réduites de moitié
 *   0.0 = arrêt complet
 * Publie $I:Mtr:scale:*:<valeur># */
void mtr_scaleTargets(float s);

/* ================================================================
 * RÉGLAGE DU COEFFICIENT DE ROTATION
 *   kturn : amplification omega dans le calcul différentiel
 *   Publie $I:Mtr:kturn:*:<valeur>#
 * ================================================================ */
void mtr_setKturn(float k);

#endif // MTR_ACTUATORS_H