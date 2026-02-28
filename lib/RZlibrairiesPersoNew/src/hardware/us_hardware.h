/************************************************************
 * FICHIER : src/hardware/us_hardware.h
 * ROLE    : Interface matérielle pour les sonars ultrasoniques
 * Chemin  : src/hardware/us_hardware.h
 *
 * Fournit :
 *  - us_initHardware()
 *  - us_measureOnce(idx) -> retourne distance en cm (0 si timeout)
 *
 * Note : la logique de "suspect / -1" et d'agrégation est dans src/sensors/us.cpp
 ************************************************************/
#ifndef US_HARDWARE_H
#define US_HARDWARE_H

#include <Arduino.h>
#include "config/config.h"

// Init bas niveau
void us_initHardware();

// Mesure brute (cm), 0 si timeout
unsigned int us_measureOnce(uint8_t idx);

#endif
