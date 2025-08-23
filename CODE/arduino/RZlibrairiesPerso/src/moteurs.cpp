// moteurs.cpp
#include "moteurs.h"
#include <Arduino.h>

// Variables globales
char transMoteurP[8] = {'<', '0', ',', '0', ',', '0', '>'};
char oldTransMoteurP[8] = {'<', '0', ',', '0', ',', '0', '>'};
char transMoteurAuto[20];

// Initialisation des moteurs
void initMoteurs() {
    transMoteurP[1] = '0';  // Accélération
    transMoteurP[3] = '0';  // Vitesse
    transMoteurP[5] = '0';  // Direction
}

// Envoi des commandes aux moteurs (Uno)
void envoiUno() {
    for (int i = 0; i < 7; i++) {
        MOTEURS_SERIAL.print(transMoteurP[i]);
    }
}

// Traitement des messages VPIV pour les moteurs
void processMoteurMessage(const MessageVPIV &msg) {
    if (msg.variable != 'M') return;

    for (int propIndex = 0; propIndex < msg.nbProperties; propIndex++) {
        char prop = msg.properties[propIndex];
        String value = msg.values[propIndex][0];  // Une seule valeur pour tous les moteurs

        if (prop == 'V') {  // Vitesse
            transMoteurP[3] = value.charAt(0);
        } else if (prop == 'W') {  // Direction
            transMoteurP[5] = value.charAt(0);
        } else if (prop == 'A') {  // Accélération
            transMoteurP[1] = value.charAt(0);
        }
    }
    envoiUno();  // Envoyer les commandes mises à jour
}
