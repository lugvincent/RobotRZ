/************************************************************
 * FICHIER : src/hardware/us_hardware.cpp
 * ROLE    : Implémentation bas-niveau pour les sonars
 * Chemin  : src/hardware/us_hardware.cpp
 *
 * Méthode : trigger 10µs + pulseIn(echo). Timeout géré par pulseIn.
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

unsigned int us_measureOnce(uint8_t idx)
{
    if (idx >= US_NUM_SENSORS)
        return 0;

    digitalWrite(usTrigPins[idx], LOW);
    delayMicroseconds(2);
    digitalWrite(usTrigPins[idx], HIGH);
    delayMicroseconds(10);
    digitalWrite(usTrigPins[idx], LOW);

    unsigned long dur = pulseIn(usEchoPins[idx], HIGH, 30000UL);
    if (dur == 0)
        return 0;

    unsigned int dist = dur / 58UL;
    if (dist > US_MAX_DISTANCE_CM)
        dist = US_MAX_DISTANCE_CM;

    return dist;
}
