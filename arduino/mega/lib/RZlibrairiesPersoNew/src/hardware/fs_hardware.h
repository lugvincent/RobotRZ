/************************************************************
 * FICHIER : src/hardware/fs_hardware.h
 * ROLE    : Interface matérielle pour le capteur HX711
 *           (définitions pins, fonctions bas-niveau)
 *
 * Notes :
 *  - Par défaut les pins sont HX_DOUT / HX_CLK (si définis dans config.h)
 *    sinon fallback A4 / A5.
 *  - Implémentation basique du protocole HX711 (bit-bang).
 ************************************************************/
#ifndef FS_HARDWARE_H
#define FS_HARDWARE_H

#include <Arduino.h>
#include "config/config.h"

// init HX711 hardware
void fs_hw_init();

// read raw 24 bits signed integer (non-blocking)
bool fs_hw_readRaw(long &outValue);

// tare offset
void fs_hw_tare();

// calibration
void fs_hw_setCal(long cal);

// convert raw -> force integer
long fs_hw_readForce();

extern long cfg_fs_calibration;
extern long cfg_fs_offset;

#endif
