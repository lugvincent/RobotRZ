/************************************************************
 * FICHIER : src/hardware/mtr_hardware.h
 * ROLE    : Interface bas-niveau moteurs (trames vers UNO)
 ************************************************************/
#ifndef MTR_HARDWARE_H
#define MTR_HARDWARE_H

#include <Arduino.h>
#include "../configuration/config.h"

// legacy trame literal (7 chars)
extern char transMoteurP[8];
extern char oldTransMoteurP[8];

// modern trame buffer
extern char transMoteurModernBuf[64];

// Fonctions hardware (envoi / init)
void mtr_hw_init();
void mtr_hw_updateTrameFromConfigLegacy();
void mtr_hw_sendLegacy();

void mtr_hw_updateTrameFromConfigModern(int L, int R, int A);
void mtr_hw_sendModern();

void mtr_hw_forceUrgStop(); // arrête matériel (envoi trame 0)
void urg_stopAllMotors();   // hook (déclaré extern)

#endif
