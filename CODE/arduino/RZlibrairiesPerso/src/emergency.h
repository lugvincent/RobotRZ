// emergency.h
#ifndef EMERGENCY_H
#define EMERGENCY_H

#include "config.h"
#include "communication.h"

// ===== Codes d'urgence =====
#define EMERGENCY_NONE             0 // Aucune urgence
#define EMERGENCY_OVERHEAT         1 // Surchauffe"
#define EMERGENCY_LOW_BAT          2 // Batterie faible
#define EMERGENCY_MOTOR_STALL      3 // Moteur bloqué
#define EMERGENCY_SENSOR_FAIL      4 // Capteur défaillant
#define EMERGENCY_BUFFER_OVERFLOW  5 // Débordement de buffer ds communication
#define EMERGENCY_PARSING_ERROR    6 // Erreur de parsing ds communication

// ===== Variables globales =====
extern bool emergencyStop;  // Arrêt d'urgence activé
extern uint8_t emergencyCode;  // Code de l'urgence actuelle

// ===== Prototypes des fonctions =====
void initEmergency();
void checkEmergencyConditions();
void handleEmergency(uint8_t code);
void sendEmergencyAlert(uint8_t code);

#endif
