// ultrasonic.h
#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <NewPing.h>
#include "config.h"// Inclure config.h pour accéder à sonarDanger
#include "communication.h"

//Prototype
void processUltrasonicMessage(const MessageVPIV &msg);

// Constantes pour les ultrasoniques
#define SONAR_NUM 9
#define MAX_DISTANCE 200
#define PING_INTERVAL 40

// Variables globales
extern NewPing sonar[SONAR_NUM];
extern const char* nomSonar[SONAR_NUM];
extern unsigned int transfertSonarCode[SONAR_NUM];
extern unsigned int transfertSonarCodeOld[SONAR_NUM];
extern unsigned long pingTimer[SONAR_NUM];

// Prototypes des fonctions
void initUltrasonic();
void fSonarSe();
void sendSonarData(uint8_t sonarIndex, unsigned int distance);
void processUltrasonicMessage(const MessageVPIV &msg);

#endif
