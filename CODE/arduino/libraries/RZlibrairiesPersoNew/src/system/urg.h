/************************************************************
 * FICHIER : src/system/urg.h
 * ROLE    : Gestion centralisée des urgences (URG)
 * Chemin  : src/system/urg.h
 ************************************************************/

#ifndef SYSTEM_URG_H
#define SYSTEM_URG_H

#include <Arduino.h>

/************************************************************
 * Codes d’urgence disponibles
 ************************************************************/
#define URG_NONE             0
#define URG_OVERHEAT         1
#define URG_LOW_BAT          2
#define URG_MOTOR_STALL      3
#define URG_SENSOR_FAIL      4
#define URG_BUFFER_OVERFLOW  5
#define URG_PARSING_ERROR    6

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
uint8_t urg_getCode();

// Envoi d’une notification VPIV
void urg_sendAlert(uint8_t code);

/************************************************************
 * HOOK MOTEURS — fourni par moteur (mtr_hardware.cpp)
 * Cette fonction est appelée lors d’une urgence.
 ************************************************************/
extern void urg_stopAllMotors();

#endif // SYSTEM_URG_H
