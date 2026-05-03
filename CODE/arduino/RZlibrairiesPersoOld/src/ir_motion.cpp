// ir_motion.cpp
#include "ir_motion.h"
#include <Arduino.h>

// Variables globales
boolean detecteurMvt = false;
int mvtIrVal = 0;

// Initialisation du détecteur de mouvement IR
void initIRMotion() {
    pinMode(MVT_IR_PIN, INPUT);
    detecteurMvt = false;
    mvtIrVal = digitalRead(MVT_IR_PIN);
}

// Vérification du détecteur de mouvement
void checkIRMotion() {
    if (!detecteurMvt) return;

    int currentVal = digitalRead(MVT_IR_PIN);
    if (currentVal != mvtIrVal) {
        mvtIrVal = currentVal;
        if (currentVal == HIGH) {
            Serial.println("$I:Mouvement détecté:IR,0,,#");
        }
    }
}

// Traitement des messages VPIV pour le détecteur IR
void processIRMotionMessage(const MessageVPIV &msg) {
    if (msg.variable != 'P') return;  // 'P' pour IR Motion (P comme Présence)

    if (msg.properties[0] == 'A') {  // Activation
        detecteurMvt = (msg.values[0][0] == "1");
    }
}
