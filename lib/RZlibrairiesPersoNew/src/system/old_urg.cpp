/************************************************************
 * FICHIER : src/config/urg.cpp
 * ROLE    : Implémentation du module URG (Urgence)
 ************************************************************/
#include "config/config.h"
#include "../actuators/lring.h"
#include "../communication/communication.h"
// ----------------------------------------------------------
// Mapping local enum → raison normalisée (VPIV)
// ----------------------------------------------------------
static const char *urg_reason_str(uint8_t code)
{
    switch (code)
    {
    case URG_LOW_BAT:
        return "low_bat";
    case URG_MOTOR_STALL:
        return "motor_stall";
    case URG_SENSOR_FAIL:
        return "sensor_fail";
    case URG_BUFFER_OVERFLOW:
        return "buffer_overflow";
    case URG_PARSING_ERROR:
        return "parsing_error";
    case URG_LOOP_TOO_SLOW:
        return "loop_too_slow";
    default:
        return "unknown";
    }
}

// ----------------------------------------------------------
// Initialisation
// ----------------------------------------------------------
void urg_init()
{
    cfg_urg_active = false;
    cfg_urg_code = URG_NONE;
    cfg_urg_timestamp = 0;
}

// ----------------------------------------------------------
// États
// ----------------------------------------------------------
bool urg_isActive() { return cfg_urg_active; }

// ----------------------------------------------------------
// Déclenchement d’une urgence
// -indice internes dans cfg_urg_messages et pour messages vpiv
//  on utilise la fonction urg_reason_str pour convertir le code en string
// ----------------------------------------------------------
void urg_handle(uint8_t code)
{
    // Si déjà actif → on ignore (pas d’escalade automatique)
    if (cfg_urg_active)
        return;

    cfg_urg_active = true;
    cfg_urg_code = code;
    cfg_urg_timestamp = millis();

    // 1️⃣ Arrêt immédiat des moteurs (sécurité locale)
    urg_stopAllMotors();

    // 2️⃣ Signalisation locale (LED ring)
    lring_emergencyTrigger(code);

    // 3️⃣ Notification VPIV avec CAT=E (événement critique)
    sendInfo("Urg", "source", "*", cfg_urg_messages[code]); // CAT=E pour la source
    sendInfo("Urg", "etat", "*", "active");                 // CAT=E pour l'état
}

// ----------------------------------------------------------
// Effacement de l’urgence
// ----------------------------------------------------------
void urg_clear()
{
    if (!cfg_urg_active)
        return;

    cfg_urg_active = false;
    cfg_urg_code = URG_NONE;
    cfg_urg_timestamp = millis();

    // Retour état visuel neutre
    lring_applyExpression("neutre");

    // Notification VPIV
    sendInfo("Urg", "etat", "*", "cleared");
}