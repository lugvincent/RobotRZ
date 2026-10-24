/************************************************************
 * FICHIER : src/hardware/us_hardware.h
 * ROLE    : Interface matérielle pour les sonars ultrasoniques
 * Chemin  : src/hardware/us_hardware.h
 *
 * Fournit :
 *  - us_initHardware()
 *  - us_measureOnce(idx[, timeout_us]) -> distance en cm, 0 si timeout
 *
 *  LE DELAI, ET POURQUOI IL EST DESORMAIS PARAMETRABLE (24/10/2026)
 *  ----------------------------------------------------------------
 *    pulseIn() attend son delai COMPLET quand l'echo ne revient jamais.
 *    A 30 ms par capteur et 9 capteurs dans la meme passe, la boucle
 *    atteignait 270 ms pour un chien de garde regle a 80 ms : l'urgence
 *    se redeclenchait a chaque passe. Voir docs/decision_US_detection_panne.md
 *
 *    Le delai se choisit maintenant selon l'usage :
 *      securite    les seuils sont a 10 et 20 cm ; 1 m suffit largement,
 *                  soit 6 ms — cinq fois le seuil d'alerte
 *      exploration la portee complete, 4 m, soit 30 ms
 *
 *    Conversion : distance_m x 2 / 343 m/s. 1 m -> 5,83 ms ; 4 m -> 23,3 ms.
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
// Delais types, en microsecondes
#define US_TIMEOUT_SECURITE_US     6000UL   // ~1 m   — groupe prioritaire
#define US_TIMEOUT_EXPLORATION_US 30000UL   // ~5 m   — portee complete

unsigned int us_measureOnce(uint8_t idx,
                            unsigned long timeout_us = US_TIMEOUT_EXPLORATION_US);

#endif
