// sound_tracker.cpp
#include "sound_tracker.h"
#include <Arduino.h>

// Variables globales
int lastSoundDirection = -1;  // -1: aucune direction détectée

// Initialisation du sound tracker
void initSoundTracker() {
    lastSoundDirection = -1;
}

// Mise à jour de la direction du son
void updateSoundDirection(int av, int gche, int drt) {
    int delta = 10;  // Seuil de différence pour détecter une direction

    if (gche > av + delta && gche > drt + delta) {
        lastSoundDirection = 1;  // Gauche
    } else if (drt > av + delta && drt > gche + delta) {
        lastSoundDirection = 2;  // Droit
    } else {
        lastSoundDirection = 0;  // Avant
    }

    // Envoi de la direction à Node-RED
    Serial.print("$V:ST:");
    Serial.print("S,0,D,");
    Serial.print(lastSoundDirection);
    Serial.println("#");
}

// Traitement des messages VPIV pour le sound tracker
void processSoundTrackerMessage(const MessageVPIV &msg) {
    if (msg.variable != 'S') return;  // 'S' pour Sound Tracker

    if (msg.properties[0] == 'A') {  // Activation
        // À implémenter si nécessaire
    }
}
