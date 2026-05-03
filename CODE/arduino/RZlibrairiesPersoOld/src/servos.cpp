// servos.cpp
#include "servos.h"
#include <Arduino.h>

// Initialisation des servos
Servo servoTGD;
Servo servoTHB;
Servo servoBase;

// Variables globales pour le contrôle manuel
bool contrManuelServoTGD = false;
bool contrManuelServoTHB = false;
bool contrManuelServoBase = false;

// Initialisation des servos
void initServos() {
    servoTGD.attach(SERVO_TGD_PIN);
    servoTHB.attach(SERVO_THB_PIN);
    servoBase.attach(SERVO_BASE_PIN);

    // Position initiale
    servoTGD.write(90 + CORRECTION_TGD);
    servoTHB.write(90 + CORRECTION_THB);
    servoBase.write(90 + CORRECTION_BASE);

    // Détacher les servos pour économiser l'énergie
    servoTGD.detach();
    servoTHB.detach();
    servoBase.detach();
}

// Traitement des messages VPIV pour les servos
void processServoMessage(const MessageVPIV &msg) {
    if (msg.variable != 'T') return;

    for (int propIndex = 0; propIndex < msg.nbProperties; propIndex++) {
        char prop = msg.properties[propIndex];
        for (int instIndex = 0; instIndex < msg.nbInstances; instIndex++) {
            String instance = msg.allInstances ? "*" : String(msg.instances[instIndex]);
            String value = msg.values[propIndex][msg.allInstances ? 0 : instIndex];

            Servo *servo = nullptr;
            int correction = 0;
            if (instance == "t" || instance == "*") {
                servo = &servoTGD;
                correction = CORRECTION_TGD;
            } else if (instance == "c" || instance == "*") {
                servo = &servoTHB;
                correction = CORRECTION_THB;
            } else if (instance == "r" || instance == "*") {
                servo = &servoBase;
                correction = CORRECTION_BASE;
            } else {
                continue;  // Instance inconnue
            }

            if (prop == 'A') {  // Angle
                int angle = value.toInt();
                servo->attach(servo == &servoTGD ? SERVO_TGD_PIN :
                             servo == &servoTHB ? SERVO_THB_PIN : SERVO_BASE_PIN);
                servo->write(angle + correction);
                delay(50);
                servo->detach();
            } else if (prop == 'M') {  // Mode (manuel/automatique)
                bool mode = (value == "1");
                if (servo == &servoTGD) contrManuelServoTGD = mode;
                else if (servo == &servoTHB) contrManuelServoTHB = mode;
                else if (servo == &servoBase) contrManuelServoBase = mode;
            }
        }
    }
}
