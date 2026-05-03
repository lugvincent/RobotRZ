// odometry.cpp
#include "odometry.h"
#include <Arduino.h>

// Variables globales
Encoder knobLeft(2, 19);  // Broches pour l'encodeur gauche
Encoder knobRight(18, 3); // Broches pour l'encodeur droit
long comptG = 0, comptD = 0, oldComptG = 0, oldComptD = 0;
double xR = 0.0, yR = 0.0, orientation = 0.0;
boolean odometrie = false;

// Coefficients pour la conversion ticks → mm/degrees
double coeffGLong = 0.0712, coeffDLong = 0.0712;
double coeffGAngl = 0.08035, coeffDAngl = 0.08035;

// Initialisation de l'odométrie
void initOdometry() {
    odometrie = false;
    comptG = comptD = 0;
    oldComptG = oldComptD = 0;
    xR = yR = orientation = 0.0;
}

// Mise à jour de l'odométrie
void updateOdometry() {
    if (!odometrie) return;

    long newLeft = knobLeft.read();
    long newRight = knobRight.read();

    if (newLeft != comptG || newRight != comptD) {
        long deltaG = newLeft - comptG;
        long deltaD = newRight - comptD;

        // Calcul des variations en distance et angle
        double dDist = (coeffGLong * deltaG + coeffDLong * deltaD) / 2.0;
        double dAngl = coeffDAngl * deltaD - coeffGAngl * deltaG;

        // Mise à jour de la position
        xR += dDist * cos(orientation);
        yR += dDist * sin(orientation);
        orientation += dAngl;

        // Envoi des données à Node-RED
        Serial.print("$V:ODO:");
        Serial.print("O,0,P,");
        Serial.print(xR);
        Serial.print(",");
        Serial.print(yR);
        Serial.print(",");
        Serial.print(orientation);
        Serial.println("#");

        comptG = newLeft;
        comptD = newRight;
    }
}

// Traitement des messages VPIV pour l'odométrie
void processOdometryMessage(const MessageVPIV &msg) {
    if (msg.variable != 'O') return;  // 'O' pour Odometry

    if (msg.properties[0] == 'A') {  // Activation
        odometrie = (String(msg.values[0][0]) == "1");
    } else if (msg.properties[0] == 'R') {  // Réinitialisation
        comptG = comptD = 0;
        xR = yR = orientation = 0.0;
    }
}
