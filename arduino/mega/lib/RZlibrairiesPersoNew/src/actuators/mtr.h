/************************************************************
 * FICHIER : src/actuators/mtr.h
 * ROLE    : API haut-niveau moteurs (Mega)
 *
 * Fournit :
 *  - mtr_init()         : initialisation matérielle / trame
 *  - mtr_sendDirect(L,R,A) : envoi direct de consignes L,R signés et A accélération
 *  - mtr_setTriplet(acc,spd,dir) : compatibilité ancienne (convertit + envoi)
 *
 ************************************************************/#ifndef MTR_ACTUATORS_H
#define MTR_ACTUATORS_H

#include <Arduino.h>

// initialisation module (appel dans setup)
void mtr_init();

// tick périodique (appel dans loop)
void mtr_processPeriodic();

// LEGACY API (compat)
void mtr_setAcceleration(int a); // 0..9 (legacy)
void mtr_setSpeed(int s);
void mtr_setDirection(int d);
void mtr_setTriplet(int a,int s,int d);
void mtr_stopAll();

// MODERN API (signed velocities)
void mtr_setTargetsSigned(int v, int omega, int accelLevel);
// v: -inputScale..+inputScale  (ex -100..100)
// omega: -inputScale..+inputScale
// accelLevel: 0..4

void mtr_setModeModern(bool modern); // bascule modern/legacy
bool mtr_isModeModern();

#endif