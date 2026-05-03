/************************************************************
 * FICHIER : src/actuators/srv.h
 * ROLE    : Interface logique des servomoteurs (module Srv)
 * Chemin  : src/actuators/srv.h
 *
 * CE MODULE GÈRE :
 *   - angles des servos (cfg_srv_angles[])
 *   - vitesses des servos (cfg_srv_speed[])
 *   - activation/désactivation (cfg_srv_active[])
 *
 * ET FOURNIT :
 *   - srv_init()        → initialise le module + positions
 *   - srv_setAngle()    → applique un angle
 *   - srv_setSpeed()    → change la vitesse
 *   - srv_setActive()   → active/désactive
 *   - srv_process()     → tick métier (appelé dans loop)
 *
 * NOTE :
 *   Aucun pin, aucune valeur par défaut n’est stockée ici.
 *   Tout vient de :
 *        src/configuration/config.h / config.cpp
 *
 *   Le bas-niveau est dans :
 *        src/hardware/servos_hardware.cpp
 ************************************************************/

#ifndef SRV_H
#define SRV_H

#include <Arduino.h>
#include "../configuration/config.h"   // contient cfg_srv_* et SERVO_COUNT

// ----------------------------------------------------------
// Initialisation du module
// ----------------------------------------------------------
void srv_init();

// ----------------------------------------------------------
// Changement d’angle
// index = 0..SERVO_COUNT-1
// ----------------------------------------------------------
void srv_setAngle(int index, int angle);

// ----------------------------------------------------------
// Changement de vitesse
// vitesse = 1 = rapide ; >10 = très lent
// ----------------------------------------------------------
void srv_setSpeed(int index, int speed);

// ----------------------------------------------------------
// Activation ou désactivation d’un servo
// ----------------------------------------------------------
void srv_setActive(int index, bool on);

// ----------------------------------------------------------
// Tick métier (loop):
// appeler srv_process() à chaque frame
// ----------------------------------------------------------
void srv_process();

#endif
