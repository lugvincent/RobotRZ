// microphone_array.h
#ifndef MICROPHONE_ARRAY_H
#define MICROPHONE_ARRAY_H

#include "config.h"
#include "communication.h"

// Broches des microphones
#define MICRO_AV_PIN A1
#define MICRO_GCHE_PIN A2
#define MICRO_DRT_PIN A3

// Variables globales
extern int oldIntMSonAv, oldIntMSonGche, oldIntMSonDrt;  // Valeurs précédentes des micros
extern boolean activeMicro;  // État d'activation des micros

// Prototypes des fonctions
void initMicrophoneArray();
void microTraitement();
void processMicrophoneMessage(const MessageVPIV &msg);

#endif
