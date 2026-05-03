// microphone_array.cpp
#include "microphone_array.h"
#include <Arduino.h>

// Variables globales
int oldIntMSonAv = 0, oldIntMSonGche = 0, oldIntMSonDrt = 0;
boolean activeMicro = false;

// Initialisation des microphones
void initMicrophoneArray() {
    pinMode(MICRO_AV_PIN, INPUT);
    pinMode(MICRO_GCHE_PIN, INPUT);
    pinMode(MICRO_DRT_PIN, INPUT);
}

// Traitement des données des microphones
void microTraitement() {
    if (!activeMicro) return;

    int deltaIntSignificative = 5;  // Seuil de variation significative
    int minMAv = 1024, maxMAv = 0;
    int minMGche = 1024, maxMGche = 0;
    int minMDrt = 1024, maxMDrt = 0;

    // Lecture des valeurs
    for (int i = 0; i < 5; i++) {
        int valMAv = analogRead(MICRO_AV_PIN);
        minMAv = min(minMAv, valMAv);
        maxMAv = max(maxMAv, valMAv);

        int valMGche = analogRead(MICRO_GCHE_PIN);
        minMGche = min(minMGche, valMGche);
        maxMGche = max(maxMGche, valMGche);

        int valMDrt = analogRead(MICRO_DRT_PIN);
        minMDrt = min(minMDrt, valMDrt);
        maxMDrt = max(maxMDrt, valMDrt);
    }

    // Calcul des intensités en %
    int intSonMAv = map(maxMAv - minMAv, 0, 1023, 0, 100);
    int intSonMGche = map(maxMGche - minMGche, 0, 1023, 0, 100);
    int intSonMDrt = map(maxMDrt - minMDrt, 0, 1023, 0, 100);

    // Envoi des données si variation significative
    if (abs(intSonMAv - oldIntMSonAv) > deltaIntSignificative ||
        abs(intSonMGche - oldIntMSonGche) > deltaIntSignificative ||
        abs(intSonMDrt - oldIntMSonDrt) > deltaIntSignificative) {

        oldIntMSonAv = intSonMAv;
        oldIntMSonGche = intSonMGche;
        oldIntMSonDrt = intSonMDrt;

        // Envoi des données à Node-RED
        Serial.print("$V:MIC:");
        Serial.print("M,");
        Serial.print("0");  // Instance 0 (micro avant)
        Serial.print(",I,");
        Serial.print(intSonMAv);
        Serial.println("#");

        Serial.print("$V:MIC:");
        Serial.print("M,");
        Serial.print("1");  // Instance 1 (micro gauche)
        Serial.print(",I,");
        Serial.print(intSonMGche);
        Serial.println("#");

        Serial.print("$V:MIC:");
        Serial.print("M,");
        Serial.print("2");  // Instance 2 (micro droit)
        Serial.print(",I,");
        Serial.print(intSonMDrt);
        Serial.println("#");
    }
}

// Traitement des messages VPIV pour les microphones
void processMicrophoneMessage(const MessageVPIV &msg) {
    if (msg.variable != 'M') return;  // 'M' pour Microphone

    if (msg.properties[0] == 'A') {  // Activation
        activeMicro = (msg.values[0][0] == "1");
    }
}
