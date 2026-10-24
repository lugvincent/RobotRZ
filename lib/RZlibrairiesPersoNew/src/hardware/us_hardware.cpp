/************************************************************
 * FICHIER : src/hardware/us_hardware.cpp
 * ROLE    : Implémentation bas-niveau pour les sonars
 * Chemin  : src/hardware/us_hardware.cpp
 *
 * Méthode : trigger 10µs + pulseIn(echo). Timeout géré par pulseIn.
 *
 * ⚠ PRECAUTION — pulseIn() est BLOQUANT. Il attend son délai complet quand
 *   l'écho ne revient pas, ce qui est le cas normal d'un capteur qui ne voit
 *   rien dans sa portée. Appeler cette fonction neuf fois de suite coûte donc
 *   neuf fois le délai. Depuis le 24/10/2026 le délai est un paramètre, et
 *   us_processPeriodic() n'interroge plus les neuf capteurs dans la même
 *   passe. Voir docs/decision_US_detection_panne.md
 ************************************************************/
#include <Arduino.h>
#include "config/config.h"
#include "us_hardware.h"

void us_initHardware()
{
    for (uint8_t i = 0; i < US_NUM_SENSORS; i++)
    {
        pinMode(usTrigPins[i], OUTPUT);
        digitalWrite(usTrigPins[i], LOW);
        pinMode(usEchoPins[i], INPUT);
    }
}

unsigned int us_measureOnce(uint8_t idx, unsigned long timeout_us)
{
    if (idx >= US_NUM_SENSORS)
        return 0;

    digitalWrite(usTrigPins[idx], LOW);
    delayMicroseconds(2);
    digitalWrite(usTrigPins[idx], HIGH);
    delayMicroseconds(10);
    digitalWrite(usTrigPins[idx], LOW);

    // Le délai décide de la portée ET du coût : voir l'entête de us_hardware.h
    unsigned long dur = pulseIn(usEchoPins[idx], HIGH, timeout_us);
    if (dur == 0)
        return 0;

    unsigned int dist = dur / 58UL;
    if (dist > US_MAX_DISTANCE_CM)
        dist = US_MAX_DISTANCE_CM;

    return dist;
}
