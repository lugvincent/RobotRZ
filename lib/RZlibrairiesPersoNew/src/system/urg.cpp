/************************************************************
 * FICHIER  : src/system/urg.cpp
 * CHEMIN   : arduino/mega/src/system/urg.cpp
 * VERSION  : 1.1  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Gestion centralisée des urgences matérielles et logicielles
 *   du firmware Arduino Mega (module URG).
 *
 * PRINCIPE
 * --------
 *   Une urgence est un état critique qui nécessite un arrêt immédiat
 *   et une action de l'opérateur (ou de SP) pour être levée.
 *   → Elle est déclenchée en interne par le firmware (jamais depuis VPIV).
 *   → Elle est levée (effacée) uniquement sur commande explicite :
 *       $V:Urg:*:clear:1#  via dispatch_Urg.cpp
 *
 * CAUSES GÉRÉES (enum UrgReason dans urg.h)
 * ------------------------------------------
 *   URG_LOW_BAT          : batterie faible
 *   URG_MOTOR_STALL      : blocage moteur détecté
 *   URG_SENSOR_FAIL      : défaut capteur critique
 *   URG_BUFFER_OVERFLOW  : débordement buffer de communication
 *   URG_PARSING_ERROR    : erreur de parsing de trame VPIV
 *   URG_LOOP_TOO_SLOW    : boucle principale trop lente (>LOOP_MAX_MS)
 *
 * PROPRIÉTÉS VPIV PUBLIÉES (direction A->SP)
 * ------------------------------------------
 *   source (CAT=E) : cause de l'urgence (string normalisé, ex "low_bat")
 *                    Publié avec sendError() car nécessite réaction automatique de SP
 *   etat   (CAT=E) : état urgence — "active" ou "cleared"
 *                    Publié avec sendError() lors du déclenchement
 *   etat   (CAT=I) : confirmation "cleared" après effacement
 *                    Publié avec sendInfo() — simple information, pas d'action requise
 *
 * PROPRIÉTÉS VPIV REÇUES (direction SP->A)
 * -----------------------------------------
 *   clear (CAT=V) : commande d'effacement de l'urgence (voir dispatch_Urg.cpp)
 *
 * ARTICULATION
 * ------------
 *   → urg_handle() appelé depuis : mvt_safe.cpp, us.cpp, main.cpp (loop watchdog)
 *   → urg_clear()  appelé depuis : dispatch_Urg.cpp
 *   → urg_isActive() consulté par : ctrl_L.cpp, mtr.cpp, tout module critique
 *   → lring.cpp   : signalisation visuelle sur déclenchement urgence
 *   → urg_stopAllMotors() : hook fourni par mtr_hardware.cpp
 *
 * PRÉCAUTIONS
 * -----------
 *   - Une urgence déjà active est ignorée (pas d'escalade automatique).
 *   - urg_handle() arrête immédiatement les moteurs (sécurité locale).
 *   - sendError() utilisé pour les urgences : SP doit réagir automatiquement.
 *   - sendInfo() utilisé uniquement pour la confirmation d'effacement.
 ************************************************************/

#include "config/config.h"
#include "../actuators/lring.h"
#include "../communication/communication.h"

/* ----------------------------------------------------------------
 * Correspondance code interne → string VPIV normalisé
 * ---------------------------------------------------------------- */
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

/* ================================================================
 * INITIALISATION — appeler depuis setup() via RZ_initAll()
 * ================================================================ */
void urg_init()
{
    cfg_urg_active = false;
    cfg_urg_code = URG_NONE;
    cfg_urg_timestamp = 0;
}

/* ================================================================
 * ÉTAT — consulté par ctrl_L, mtr, et tous les modules critiques
 * ================================================================ */
bool urg_isActive() { return cfg_urg_active; }

/* ================================================================
 * DÉCLENCHEMENT D'UNE URGENCE
 *
 * Actions réalisées dans l'ordre :
 *   1. Arrêt immédiat des moteurs (sécurité locale, sans attendre SP)
 *   2. Signalisation visuelle (LED ring — appel lring_emergencyTrigger)
 *   3. Publication VPIV avec sendError() :
 *        $E:Urg:source:*:<cause>#   — cause de l'urgence
 *        $E:Urg:etat:*:active#      — état urgence actif
 *      CAT=E utilisé ici car SP doit réagir automatiquement (stop, alerte, ...)
 * ================================================================ */
void urg_handle(uint8_t code)
{
    // Si une urgence est déjà active, on l'ignore (pas d'escalade)
    if (cfg_urg_active)
        return;

    cfg_urg_active = true;
    cfg_urg_code = code;
    cfg_urg_timestamp = millis();

    // 1. Arrêt moteurs immédiat (hook défini dans mtr_hardware.cpp)
    urg_stopAllMotors();

    // 2. Signalisation visuelle sur le LED ring
    lring_emergencyTrigger(code);

    // 3. Notification VPIV — CAT=E : SP doit réagir automatiquement
    sendError("Urg", "source", "*", urg_reason_str(code)); // cause normalisée
    sendError("Urg", "etat", "*", "active");               // état urgence
}

/* ================================================================
 * EFFACEMENT DE L'URGENCE (sur commande $V:Urg:*:clear:1#)
 *
 * Remet le système en état opérationnel.
 * La confirmation est publiée en CAT=I (simple ACK, pas d'urgence).
 * ================================================================ */
void urg_clear()
{
    if (!cfg_urg_active)
        return; // déjà effacée, rien à faire

    cfg_urg_active = false;
    cfg_urg_code = URG_NONE;
    cfg_urg_timestamp = millis();

    // Retour visuel neutre (expression LED ring)
    lring_applyExpression("neutre");

    // Confirmation VPIV — CAT=I : simple information, aucune action requise
    sendInfo("Urg", "etat", "*", "cleared");
}