/************************************************************
 * FICHIER : src/hardware/srv_hardware.cpp
 * ROLE    : Pilotage bas-niveau des servomoteurs
 * Chemin  : src/hardware/srv_hardware.cpp
 *
 * Remarques :
 *  - NE DÉCIDE AUCUNE VALEUR PAR DÉFAUT
 *  - Utilise EXCLUSIVEMENT les valeurs centralisées :
 *        → cfg_srv_angles[]   (position par servo)
 *        → cfg_srv_speed[]    (vitesse)
 *        → cfg_srv_active[]   (activation)
 *  - Les "zéro mécaniques" sont définis dans config.h :
 *        SERVO_TGD_ZERO, SERVO_THB_ZERO, SERVO_BASE_ZERO
 *    et déjà copiés dans cfg_srv_angles[] au boot dans config.cpp
 *
 *  - Aucune dépendance à Servo.h (timing précis 50 Hz maison)
 ************************************************************/

#include "srv_hardware.h"
#include "../config/config.h"
#include <Arduino.h>

/************************************************************
 * Pins des servos (définis dans config.h)
 ************************************************************/
static const uint8_t srvPins[SERVO_COUNT] = {
    SERVO_TGD_PIN,  // index 0
    SERVO_THB_PIN,  // index 1
    SERVO_BASE_PIN  // index 2
};

/************************************************************
 * Variables internes du module
 *
 * targetPulse[]  : objectif µs (imposé par cfg_srv_angles[])
 * currentPulse[] : valeur actuelle envoyée (µs)
 * lastUpdate[]   : dernière impulsion (µs)
 ************************************************************/
static unsigned int targetPulse[SERVO_COUNT];
static unsigned int currentPulse[SERVO_COUNT];
static unsigned long lastUpdate[SERVO_COUNT];

/************************************************************
 * Conversion angle → impulsion en microsecondes
 * Note : ce mapping reste standard, mais peut être ajusté
 ************************************************************/
static unsigned int angleToPulse(int angle)
{
    angle = constrain(angle, 0, 180);
    return map(angle, 0, 180, 500, 2500);
}

/************************************************************
 * Initialisation hardware
 *
 * → Lit directement cfg_srv_angles[] définis dans config.cpp :
 *      { SERVO_TGD_ZERO, SERVO_THB_ZERO, SERVO_BASE_ZERO }
 *
 ************************************************************/
void srv_hw_init()
{
    for (int i = 0; i < SERVO_COUNT; i++) {

        pinMode(srvPins[i], OUTPUT);
        digitalWrite(srvPins[i], LOW);

        // Position initiale = valeur définie côté config
        int initAngle = cfg_srv_angles[i];
        targetPulse[i]  = angleToPulse(initAngle);
        currentPulse[i] = targetPulse[i];

        lastUpdate[i] = micros();
    }
}

/************************************************************
 * Mise à jour d’un servo (appelée par module Srv)
 ************************************************************/
void srv_hw_setTarget(int index, int angle)
{
    if (index < 0 || index >= SERVO_COUNT) return;

    targetPulse[index] = angleToPulse(angle);
}

/************************************************************
 * Boucle d’entretien à 50 Hz — très léger
 ************************************************************/
void srv_hw_update()
{
    unsigned long now = micros();

    for (int i = 0; i < SERVO_COUNT; i++) {

        // servo désactivé → pas d’impulsion envoyée
        if (!cfg_srv_active[i]) continue;

        // période 20 ms (50 Hz)
        if (now - lastUpdate[i] < 20000UL) continue;
        lastUpdate[i] = now;

        // interpolation = vitesse douce (contrôlée par cfg_srv_speed[])
        int speed = max(1, cfg_srv_speed[i]);
        currentPulse[i] += (targetPulse[i] - currentPulse[i]) / speed;

        // génération de l’impulsion
        digitalWrite(srvPins[i], HIGH);
        delayMicroseconds(currentPulse[i]);
        digitalWrite(srvPins[i], LOW);
    }
}
