/************************************************************
 * FICHIER : src/system/urg.h
 * ROLE    : Gestion centralisée des urgences (URG)
 * Chemin  : src/system/urg.h
 ************************************************************/

#ifndef SYSTEM_URG_H
#define SYSTEM_URG_H

#include <Arduino.h>
#include <stdint.h> // uint8_t explicite
/************************************************************
 *  ENUM interne des causes d'urgence disponibles spécifiques
 * à l'arduino et à usage interne *
 ************************************************************/
enum UrgReason : uint8_t
{
    URG_NONE = 0,
    URG_LOW_BAT,
    URG_MOTOR_STALL,
    URG_SENSOR_FAIL,
    URG_BUFFER_OVERFLOW,
    URG_PARSING_ERROR,
    URG_LOOP_TOO_SLOW
};

/************************************************************
 * API PUBLIQUE — à utiliser dans tout le firmware
 ************************************************************/

// Initialise le module URG
void urg_init();

// Déclenche une urgence
void urg_handle(uint8_t code);

// Efface / réarme le système d’urgence
void urg_clear();

// État de l’urgence
bool urg_isActive();

// Envoi d’une notification VPIV
void urg_sendAlert(uint8_t code);

/************************************************************
 * HOOK MOTEURS — fourni par moteur (mtr_hardware.cpp)
 * Cette fonction est appelée lors d’une urgence.
 ************************************************************/
extern void urg_stopAllMotors();

#endif // SYSTEM_URG_H
