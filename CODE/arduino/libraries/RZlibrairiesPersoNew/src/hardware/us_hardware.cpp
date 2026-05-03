/************************************************************
 * FICHIER : src/hardware/us_hardware.cpp
 * ROLE    : Implémentation bas-niveau pour les sonars
 * Chemin  : src/hardware/us_hardware.cpp
 *
 * Méthode : trigger 10µs + pulseIn(echo). Timeout géré par pulseIn.
 ************************************************************/
#include "us_hardware.h"

void us_initHardware()
{
    for (uint8_t i = 0; i < US_NUM_SENSORS; i++) {
        pinMode(usTrigPins[i], OUTPUT);
        digitalWrite(usTrigPins[i], LOW);

        pinMode(usEchoPins[i], INPUT);
    }
}

unsigned int us_measureOnce(uint8_t idx)
{
    if (idx >= US_NUM_SENSORS) return 0;

    uint8_t trig = usTrigPins[idx];
    uint8_t echo = usEchoPins[idx];

    digitalWrite(trig,LOW);
    delayMicroseconds(2);
    digitalWrite(trig,HIGH);
    delayMicroseconds(10);
    digitalWrite(trig,LOW);

    unsigned long dur = pulseIn(echo, HIGH, 30000UL);
    if (dur == 0) return 0;

    unsigned int dist = dur / 58UL;
    if (dist > US_MAX_DISTANCE_CM) dist = US_MAX_DISTANCE_CM;
    return dist;
}