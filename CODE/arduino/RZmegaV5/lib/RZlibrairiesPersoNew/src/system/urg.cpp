/************************************************************
 * FICHIER : src/config/urg.cpp
 * ROLE    : Implémentation du module URG (Urgence)
 ************************************************************/

#include "urg.h"
#include "../config/config.h"
#include "../communication/communication.h"
#include "../actuators/lring.h"      // <-- AJOUT INDISPENSABLE

// ----------------------------------------------------------
// Initialisation
// ----------------------------------------------------------
void urg_init() {
    cfg_urg_active    = false;
    cfg_urg_code      = URG_NONE;
    cfg_urg_timestamp = 0;
}

// ----------------------------------------------------------
// États
// ----------------------------------------------------------
bool urg_isActive() { return cfg_urg_active; }
uint8_t urg_getCode() { return cfg_urg_code; }

// ----------------------------------------------------------
// VPIV : envoi d’un message d’alerte
// Format : $I:Urg:code:*:<code>:<message>#
// ----------------------------------------------------------
void urg_sendAlert(uint8_t code) {

    char buf[32];
    snprintf(buf, sizeof(buf), "%d:%s", code, cfg_urg_messages[code]);

    sendInfo("Urg", "code", "*", buf);
}

// ----------------------------------------------------------
// Déclenchement d’une urgence
// ----------------------------------------------------------
void urg_handle(uint8_t code) {

    // Première activation
    if (!cfg_urg_active) {

        cfg_urg_active    = true;
        cfg_urg_code      = code;
        cfg_urg_timestamp = millis();

        // 1) Arrêt immédiat des moteurs
        urg_stopAllMotors();

        // 2) Expression alerte sur le ring LED
        lring_emergencyTrigger(code);      // <-- AJOUT MAJEUR

        // 3) Envoi VPIV
        urg_sendAlert(code);
    }
    else {
        // Urgence déjà active mais changement de code
        if (cfg_urg_code != code) {
            cfg_urg_code = code;

            // Mise à jour de l'affichage si gravité change
            lring_emergencyTrigger(code);

            urg_sendAlert(code);
        }
    }
}

// ----------------------------------------------------------
// Effacement de l’urgence
// ----------------------------------------------------------
void urg_clear() {

    cfg_urg_active    = false;
    cfg_urg_code      = URG_NONE;
    cfg_urg_timestamp = millis();

    // Retour à l’expression neutre
    lring_applyExpression("neutre");

    sendInfo("Urg", "clear", "*", "OK");
}
