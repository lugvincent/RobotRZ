/*
US - Objectif limiter envois, gestion seuils, centraliser sonarDanger[]
On utilise un prototype local pour sendSonarData pour compil. Permet def. de la fonction avant utilisation.
 Pas dans le .h car c’est interne à ce fichier, static pour les mêmes raisons cf notes
*/
#include "ultrasonic.h"
#include "communication.h"
#include "config.h"
#include <NewPing.h>

// Prototype local 
static void sendSonarData(uint8_t sonarIndex, unsigned int distance);

// Création des objets sonar avec les pins de config.h
NewPing sonar[SONAR_NUM] = {
    NewPing(SONAR_PINS[0][0], SONAR_PINS[0][1], MAX_DISTANCE),
    NewPing(SONAR_PINS[1][0], SONAR_PINS[1][1], MAX_DISTANCE),
    NewPing(SONAR_PINS[2][0], SONAR_PINS[2][1], MAX_DISTANCE),
    NewPing(SONAR_PINS[3][0], SONAR_PINS[3][1], MAX_DISTANCE),
    NewPing(SONAR_PINS[4][0], SONAR_PINS[4][1], MAX_DISTANCE),
    NewPing(SONAR_PINS[5][0], SONAR_PINS[5][1], MAX_DISTANCE),
    NewPing(SONAR_PINS[6][0], SONAR_PINS[6][1], MAX_DISTANCE),
    NewPing(SONAR_PINS[7][0], SONAR_PINS[7][1], MAX_DISTANCE),
    NewPing(SONAR_PINS[8][0], SONAR_PINS[8][1], MAX_DISTANCE)
};

// Variables de gestion
unsigned int transfertSonarCode[SONAR_NUM] = {0};
unsigned int transfertSonarCodeOld[SONAR_NUM] = {0};
unsigned long pingTimer[SONAR_NUM] = {0};

// Initialisation des ultrasoniques
void initUltrasonic() {
    for (uint8_t i = 0; i < SONAR_NUM; i++) {
        pingTimer[i] = millis() + i * PING_INTERVAL;
    }
}

// Mesure et envoi des données ultrasoniques
void fSonarSe() {
    for (uint8_t i = 0; i < SONAR_NUM; i++) {
        if (millis() >= pingTimer[i]) {
            pingTimer[i] += PING_INTERVAL * SONAR_NUM;
            unsigned int cm = sonar[i].ping_cm();
            sendSonarData(i, cm);
        }
    }
}

// Envoi des données ultrasoniques au format VPIV
void sendSonarData(uint8_t sonarIndex, unsigned int distance) {
    Serial.print("$V:US:U,");
    Serial.print(sonarIndex);
    Serial.print(",D,");
    Serial.print(distance);
    Serial.println("#");
}

// Traitement des messages VPIV pour les ultrasoniques
void processUltrasonicMessage(const MessageVPIV &msg) {
    if (msg.variable != 'U') return;

    for (int propIndex = 0; propIndex < msg.nbProperties; propIndex++) {
        char prop = msg.properties[propIndex];
        if (prop == 'D') {  // Seuil de danger
            if (msg.allInstances) {
                int seuil = atoi(msg.values[propIndex][0]);
                for (uint8_t i = 0; i < SONAR_NUM; i++) {
                    sonarDanger[i] = seuil;
                }
            } else {
                for (int instIndex = 0; instIndex < msg.nbInstances; instIndex++) {
                    uint8_t sonarIndex = msg.instances[instIndex];
                    if (sonarIndex < SONAR_NUM) {
                        sonarDanger[sonarIndex] = atoi(msg.values[propIndex][instIndex]);
                    }
                }
            }
        }
    }
}
