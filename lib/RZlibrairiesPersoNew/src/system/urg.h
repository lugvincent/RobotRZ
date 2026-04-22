/************************************************************
 * FICHIER : src/system/urg.h
 * ROLE    : Gestion centralisée des urgences (URG)
 * Chemin  : src/system/urg.h
 ************************************************************/
#ifndef SYSTEM_URG_H
#define SYSTEM_URG_H

#include <Arduino.h>
#include <stdint.h>

/************************************************************
 * ENUM interne des causes d'urgence
 * Aligné avec nomenclature globale URG_*
 ************************************************************/
enum UrgReason : uint8_t
{
    URG_NONE = 0,

    URG_LOW_BAT,
    URG_MOTOR_STALL,
    URG_SENSOR_FAIL,
    URG_BUFFER_OVERFLOW,
    URG_PARSING_ERROR,

    // Différenciation A vs SE
    URG_LOOP_TOO_SLOW_A,

    // Nouveaux cas critiques Arduino
    URG_US_DANGER,
    URG_MVT_DANGER,

    // fallback
    URG_UNKNOWN
};

/************************************************************
 * API PUBLIQUE
 ************************************************************/

/// @brief
void urg_init();
void urg_handle(uint8_t code);
void urg_clear();
bool urg_isActive();

/************************************************************
 * HOOK MOTEURS
 ************************************************************/
extern void urg_stopAllMotors();

#endif