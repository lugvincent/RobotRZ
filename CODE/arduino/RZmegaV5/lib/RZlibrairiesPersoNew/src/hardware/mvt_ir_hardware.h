/************************************************************
 * FICHIER : src/hardware/mvt_ir_hardware.h
 * ROLE    : Interface matérielle du capteur IR de mouvement
 ************************************************************/
#ifndef MVT_IR_HARDWARE_H
#define MVT_IR_HARDWARE_H

#include <Arduino.h>
#include "../configuration/config.h"

// Initialise la pin hardware
void mvt_ir_hw_init();

// Lecture brute : retourne 0 ou 1
int  mvt_ir_hw_readRaw();

#endif // MVT_IR_HARDWARE_H