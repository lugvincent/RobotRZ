/************************************************************
 * FICHIER  : src/hardware/mtr_hardware.cpp
 * CHEMIN   : arduino/mega/src/hardware/mtr_hardware.cpp
 * VERSION  : 2.0  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Couche matérielle bas-niveau du module moteurs.
 *   Construit et envoie les trames ASCII "<L,R,A>" vers l'Arduino UNO
 *   via la liaison série hardware MOTEURS_SERIAL.
 *
 * FORMAT DE TRAME
 * ----------------
 *   Trame ASCII : "<L,R,A>\n"
 *   L, R : entiers signés, en unités moteur [-400..+400]
 *   A    : niveau de lissage [0..4]
 *
 *   L'UNO lit chaque caractère entrant, accumule entre '<' et '>',
 *   puis parse le buffer. Le '\n' final sert de flush série.
 *
 *   Exemples envoyés sur la liaison série :
 *     "<320,320,2>\n"     → avance droit à ~80%, lissage moyen
 *     "<-160,160,2>\n"    → tourne droite sur place
 *     "<0,0,0>\n"         → arrêt immédiat (A=0 = changement direct)
 *
 * CLAMP DE SÉCURITÉ
 * ------------------
 *   mtr_hw_updateTrame() clamp L et R dans [-MTR_HW_MAX..+MTR_HW_MAX]
 *   et A dans [0..4] comme protection de dernière chance.
 *   mtr.cpp clamp déjà les valeurs — ce clamp est une sécurité supplémentaire.
 *
 * HOOK URGENCE
 * ------------
 *   urg_stopAllMotors() est appelé directement par urg.cpp.
 *   Il bypass mtr.cpp pour garantir l'arrêt dans tous les cas.
 *
 * ARTICULATION
 * ------------
 *   mtr.cpp → mtr_hardware.cpp → MOTEURS_SERIAL → UNO
 *
 * PRÉCAUTIONS
 * -----------
 *   - MOTEURS_SERIAL est défini dans config.h (ex: Serial3)
 *   - La vitesse de liaison (baud) doit correspondre à l'UNO (115200)
 ************************************************************/

#include "mtr_hardware.h"
#include "config/config.h"

/* ----------------------------------------------------------------
 * Clamp matériel : valeur absolue maximale envoyable à l'UNO.
 * Doit correspondre à MOTOR_MAX dans le firmware UNO (= 400).
 * ---------------------------------------------------------------- */
#define MTR_HW_MAX 400

/* Buffer de la trame courante — partagé avec mtr.h (extern) */
char mtr_trameBuf[32];

/* ================================================================
 * INITIALISATION
 *   Construit une trame neutre <0,0,2> et laisse le buffer prêt.
 *   MOTEURS_SERIAL.begin() est appelé dans setup() (main.cpp).
 * ================================================================ */
void mtr_hw_init()
{
    snprintf(mtr_trameBuf, sizeof(mtr_trameBuf), "<0,0,2>");
}

/* ================================================================
 * CONSTRUCTION DE LA TRAME
 *
 *   Remplit mtr_trameBuf avec le format "<L,R,A>".
 *   Clamp de sécurité sur L, R et A avant construction.
 *
 *   L, R : vitesses roues signées [-MTR_HW_MAX..+MTR_HW_MAX]
 *   A    : niveau lissage [0..4]
 * ================================================================ */
void mtr_hw_updateTrame(int L, int R, int A)
{
    // Clamp de sécurité — protection de dernière chance
    if (L > MTR_HW_MAX)
        L = MTR_HW_MAX;
    if (L < -MTR_HW_MAX)
        L = -MTR_HW_MAX;
    if (R > MTR_HW_MAX)
        R = MTR_HW_MAX;
    if (R < -MTR_HW_MAX)
        R = -MTR_HW_MAX;
    if (A < 0)
        A = 0;
    if (A > 4)
        A = 4;

    snprintf(mtr_trameBuf, sizeof(mtr_trameBuf), "<%d,%d,%d>", L, R, A);
}

/* ================================================================
 * ENVOI DE LA TRAME
 *   Envoie le contenu de mtr_trameBuf sur MOTEURS_SERIAL.
 *   Le '\n' final provoque le flush du buffer série côté UNO.
 *   Appeler mtr_hw_updateTrame() avant cet appel.
 * ================================================================ */
void mtr_hw_send()
{
    MOTEURS_SERIAL.print(mtr_trameBuf);
    MOTEURS_SERIAL.print('\n');
}

/* ================================================================
 * HOOK URGENCE — arrêt matériel immédiat
 *
 *   Construit et envoie directement <0,0,0> sans passer par mtr.cpp.
 *   A=0 = changement direct côté UNO (pas de lissage → arrêt net).
 *   Appelé par urg_stopAllMotors() depuis urg.cpp.
 * ================================================================ */
void mtr_hw_forceUrgStop()
{
    // Construction et envoi directs — bypass total de mtr.cpp
    MOTEURS_SERIAL.print("<0,0,0>");
    MOTEURS_SERIAL.print('\n');

    // Mise à jour du buffer pour cohérence (si relecture de mtr_trameBuf)
    snprintf(mtr_trameBuf, sizeof(mtr_trameBuf), "<0,0,0>");
}

/* ================================================================
 * urg_stopAllMotors() — hook appelé par urg.cpp
 *   Simple délégation vers mtr_hw_forceUrgStop().
 *   Déclaration dans mtr_hardware.h — implémentation ici
 *   pour rester dans la couche hardware.
 * ================================================================ */
void urg_stopAllMotors()
{
    mtr_hw_forceUrgStop();
}