// servos.h
#ifndef SERVOS_H
#define SERVOS_H

#include <Servo.h>
#include "config.h"
#include "communication.h"

// Broches des servos
#define SERVO_TGD_PIN 4   // Tête Gauche/Droite
#define SERVO_THB_PIN 5   // Tête Haut/Bas
#define SERVO_BASE_PIN 9  // Base (corps)

// Corrections des positions
#define CORRECTION_TGD 30
#define CORRECTION_THB 45
#define CORRECTION_BASE 20

// Variables globales
extern Servo servoTGD;
extern Servo servoTHB;
extern Servo servoBase;
extern bool contrManuelServoTGD;
extern bool contrManuelServoTHB;
extern bool contrManuelServoBase;

// Prototypes des fonctions
void initServos();
void processServoMessage(const MessageVPIV &msg);

#endif
