/************************************************************
 * FICHIER : src/sensors/mvt_ir.cpp
 * ROLE    : Gestion haut-niveau du détecteur IR
 *
 * Fonctionnalités :
 *   - read          -> lecture immédiate
 *   - valeur périodique
 *   - alert si nombre de détections successives > seuil (thd)
 *   - act           -> activer/désactiver
 *   - freq          -> période
 *   - mode          -> idle (inactif)/ monitor / surveillance
 *                      preset de configuration (n'écrase pas modifications ultérieures)
 ************************************************************/

#include "mvt_ir.h"
#include "hardware/mvt_ir_hardware.h"
#include "config/config.h"
#include <Arduino.h>
#include <string.h>
#include "../communication/communication.h" // pour sendInfo / sendError

// Compteur interne (ne va pas dans config)
static int mvtir_counter = 0;

// Envoi VPIV : valeur capteur (instantané ou périodique)
static void _send_value_vpiv(int v)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", v);
    sendInfo("IRmvt", "value", "*", buf);
}

// Envoi alerte
static void _send_alert_vpiv()
{
    sendInfo("IRmvt", "alert", "*", "1");
}

// Initialisation
void mvt_ir_init()
{
    mvt_ir_hw_init();
    // charger valeurs depuis config variables existantes (déjà définies dans config.cpp)
    mvtir_counter = 0;
}

// Lecture instantanée et envoi VPIV
void mvt_ir_readInstant()
{
    int v = mvt_ir_hw_readRaw();

    // 1. ACK read
    sendInfo("IRmvt", "read", "*", "OK");

    // 2. valeur réelle
    _send_value_vpiv(v);
}

// Activation / désactivation
void mvt_ir_setActive(bool on)
{
    cfg_mvtir_active = on;
    // ack
    sendInfo("IRmvt", "act", "*", on ? "1" : "0");
}

// Fréquence (ms)
void mvt_ir_setFreqMs(unsigned long ms)
{
    if (ms < 10)
        ms = 10;
    cfg_mvtir_freq_ms = ms;
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", ms);
    sendInfo("IRmvt", "freq", "*", buf);
}

// Seuil : nombre de détections successives pour déclencher l'alerte
void mvt_ir_setThreshold(int n)
{
    if (n < 1)
        n = 1;
    cfg_mvtir_threshold = n;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", n);
    sendInfo("IRmvt", "thd", "*", buf);
}

// Mode : met en place provisoirement des valeurs spécifiques
// pour les modes "prédéfinis" (monitor, surveillance) afin de faciliter
// la configuration rapide. Ces valeurs ne sont pas figées,
// elles peuvent être modifiées ensuite via les fonctions de réglage individuelles (freq, thd).
void mvt_ir_setMode(const char *mode)
{
    if (!mode)
        return;
    if (strcmp(mode, "idle") == 0)
    {
        cfg_mvtir_active = false;
        cfg_mvtir_freq_ms = 0;
        strncpy(cfg_mvtir_mode, "idle", sizeof(cfg_mvtir_mode) - 1);
        cfg_mvtir_mode[sizeof(cfg_mvtir_mode) - 1] = '\0';
        sendInfo("IRmvt", "mode", "*", "idle");
        return;
    }
    if (strcmp(mode, "monitor") == 0)
    {
        cfg_mvtir_active = true;
        cfg_mvtir_freq_ms = 150;
        cfg_mvtir_threshold = 1;
        strncpy(cfg_mvtir_mode, "monitor", sizeof(cfg_mvtir_mode) - 1);
        cfg_mvtir_mode[sizeof(cfg_mvtir_mode) - 1] = '\0';
        sendInfo("IRmvt", "mode", "*", "monitor");
        return;
    }
    if (strcmp(mode, "surveillance") == 0)
    {
        cfg_mvtir_active = true;
        cfg_mvtir_freq_ms = 80;
        cfg_mvtir_threshold = 2;
        strncpy(cfg_mvtir_mode, "surveillance", sizeof(cfg_mvtir_mode) - 1);
        cfg_mvtir_mode[sizeof(cfg_mvtir_mode) - 1] = '\0';
        sendInfo("IRmvt", "mode", "*", "surveillance");
        return;
    }

    // si mode inconnu -> retourner erreur
    sendError("IRmvt", "mode", "*", "unknown");
}

// Process périodique (à appeler depuis loop())
void mvt_ir_processPeriodic()
{
    static unsigned long lastT = 0;

    if (!cfg_mvtir_active)
        return;

    if (cfg_mvtir_freq_ms == 0)
        return;

    unsigned long now = millis();

    if (now - lastT < cfg_mvtir_freq_ms)
        return;

    lastT = now;

    int v = mvt_ir_hw_readRaw();

    // ✅ AJOUT IMPORTANT → publication valeur
    _send_value_vpiv(v);

    // gestion détection
    if (v == 1)
    {
        mvtir_counter++;

        if (mvtir_counter >= cfg_mvtir_threshold)
        {
            _send_alert_vpiv();
            mvtir_counter = 0;
        }
    }
    else
    {
        mvtir_counter = 0;
    }
}