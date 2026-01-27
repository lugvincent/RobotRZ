/************************************************************
 * FICHIER : src/hardware/srv_hardware.h
 * ROLE    : Interface bas-niveau servomoteurs (PWM 50 Hz maison)
 * NOTE    : Correspond EXACTEMENT à srv_hardware.cpp
 ************************************************************/

#ifndef SRV_HARDWARE_H
#define SRV_HARDWARE_H

#include <Arduino.h>
#include "config/config.h"

// ----------------------------------------------------------
// Initialisation du module hardware
// Lit cfg_srv_angles[] et initialise pulses
// ----------------------------------------------------------
void srv_hw_init();

// ----------------------------------------------------------
// Définit la nouvelle position cible du servo (µs) selon angle
// Appelé par srv_setAngle()
// ----------------------------------------------------------
void srv_hw_setTarget(int index, int angle);

// ----------------------------------------------------------
// Mise à jour périodique 50 Hz — doit être appelée dans loop()
// Gère interpolation + génération du PWM
// ----------------------------------------------------------
void srv_hw_update();

#endif
