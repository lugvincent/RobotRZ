// emergency.cpp
#include "emergency.h"
#include <Arduino.h>

// Variables globales
bool emergencyStop = false;
uint8_t emergencyCode = EMERGENCY_NONE;

// Initialisation du système d'urgence
void initEmergency() {
    emergencyStop = false;
    emergencyCode = EMERGENCY_NONE;
}

// Vérification des conditions d'urgence
void checkEmergencyConditions() {
    // Exemple : Vérifier la tension de la batterie (à implémenter)
    // if (batteryVoltage < LOW_BAT_THRESHOLD) {
    //     handleEmergency(EMERGENCY_LOW_BAT);
    // }
}

// Gestion d'une urgence
void handleEmergency(uint8_t code) {
    emergencyCode = code;
    emergencyStop = true;
    sendEmergencyAlert(code);

    // Actions spécifiques selon le code d'urgence
    switch (code) {
        case EMERGENCY_OVERHEAT:
            // Arrêter les moteurs, etc.
            break;
        case EMERGENCY_LOW_BAT:
            // Passer en mode veille
            break;
        case EMERGENCY_MOTOR_STALL:
            // Réinitialiser les moteurs
            break;
        case EMERGENCY_SENSOR_FAIL:
            // Désactiver le capteur défaillant
            break;
    }
}

// Envoi d'une alerte d'urgence
void sendEmergencyAlert(uint8_t code) {
const char* alertMessages[] = {
    "Aucune urgence",
    "Surchauffe",
    "Batterie faible",
    "Moteur bloqué",
    "Capteur défaillant",
    "Débordement de buffer",      // EMERGENCY_BUFFER_OVERFLOW
    "Erreur de parsing"           // EMERGENCY_PARSING_ERROR
};

    sendError("URGENCE", "", alertMessages[code]);
}
