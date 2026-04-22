// =====================================================================
// FICHIER  : src/sensors/fs.cpp
// CHEMIN   : lib/RZlibrairiesPersoNew/src/sensors/fs.cpp
// VERSION  : 1.2  —  Mars 2026
// AUTEUR   : Vincent Philippe
//
// RÔLE
// ----
//   Module haut-niveau du capteur de force (Force Sensor, HX711).
//   Logique métier : initialisation, lecture, tare, calibration,
//   seuil d'alerte, publication VPIV périodique.
//
// VPIV PRODUITS — conformes Table A Arduino v3.1
// -----------------------------------------------
//   $F:FS:*:force:<val>#     — force calibrée (périodique CAT=F + réponse read)
//   $F:FS:*:raw:<val>#       — valeur brute HX711 (réponse read:raw, CAT=F)
//   $I:FS:*:alerte:<val>#    — valeur force au moment du dépassement du seuil (CAT=I)
//   $I:FS:*:status:alerte#   — changement d'état → alerte active (CAT=I)
//   $I:FS:*:status:actif#    — changement d'état → retour sous le seuil (CAT=I)
//   $I:FS:*:tare:OK#         — confirmation tare
//   $I:FS:*:cal:<val>#       — confirmation calibration
//   $I:FS:*:act:0|1#         — confirmation activation
//   $I:FS:*:freq:OK#         — confirmation fréquence
//   $I:FS:*:thd:OK#          — confirmation seuil
//
// ARTICULATION
// ------------
//   dispatch_FS.cpp → fs.cpp → fs_hardware.cpp (HX711 bas-niveau)
//   loop() → fs_processPeriodic()
//
// CORRECTIONS v1.2 (conformité Table A)
// ----------------------------------------
//   - prop "force"  (L46 table : prop=force, CAT=F)
//   - prop  "alerte" (L59 table : prop=alerte, valeur=force)
//   - sendInfo → sendFlux pour force et raw (CAT=F dans la table)
//   - alerte : valeur = force au dépassement (entier, même unité)
//   - alerte descente : status:actif + alerte:0 (retour sous seuil)
//   - FS:status émis lors de chaque changement d'état (L58 table)
//   - freq:0 → désactive le module (note table L54)
//
// PRÉCAUTIONS
// -----------
//   cfg_fs_active, cfg_fs_threshold, cfg_fs_freq_ms dans config.h/cpp
//   fs_processPeriodic() DOIT être appelé dans loop()
// =====================================================================

#include "fs.h"
#include "hardware/fs_hardware.h"
#include "communication/communication.h"
#include "config/config.h"
#include <Arduino.h>
#include <string.h>

// =====================================================================
// ÉTAT INTERNE
// =====================================================================
static unsigned long _lastPeriodic = 0;
static long _lastForce = 0;
static bool _alertActive = false; // true = seuil dépassé en cours

// =====================================================================
// fs_init()
// =====================================================================
void fs_init()
{
    fs_hw_init();
    _lastPeriodic = 0;
    _lastForce = 0;
    _alertActive = false;
}

// =====================================================================
// fs_doRead()
// Lecture immédiate sur demande SP ($V:FS:*:read:force# ou read:raw).
// type = "force" (défaut) | "raw"
// =====================================================================
void fs_doRead(const char *type)
{
    long raw = 0;
    if (!fs_hw_readRaw(raw))
    {
        sendError("FS", "read", "*", "not_ready");
        return;
    }

    if (type && strcmp(type, "raw") == 0)
    {
        // Valeur brute HX711 → CAT=F (Table A L47)
        char buf[20];
        snprintf(buf, sizeof(buf), "%ld", raw);
        sendFlux("FS", "raw", "*", buf); // $F:FS:*:raw:<val>#
    }
    else
    {
        // Force calibrée → CAT=F (Table A L46)
        _lastForce = (cfg_fs_calibration != 0)
                         ? (raw - cfg_fs_offset) / cfg_fs_calibration
                         : 0;
        char buf[20];
        snprintf(buf, sizeof(buf), "%ld", _lastForce);
        sendFlux("FS", "force", "*", buf); // $F:FS:*:force:<val>#
    }
}

// =====================================================================
// fs_doTare()
// =====================================================================
void fs_doTare()
{
    fs_hw_tare();
    sendInfo("FS", "tare", "*", "OK"); // $I:FS:*:tare:OK#
}

// =====================================================================
// fs_doCal()
// factor = valeur ×1000 (Table A L50 : CODAGE=×1000, 950=0.950)
// =====================================================================
void fs_doCal(long factor)
{
    if (factor == 0)
    {
        sendError("FS", "cal", "*", "zero_interdit");
        return;
    }
    fs_hw_setCal(factor);
    cfg_fs_calibration = factor;

    char buf[20];
    snprintf(buf, sizeof(buf), "%ld", factor);
    sendInfo("FS", "cal", "*", buf); // $I:FS:*:cal:<val>#
}

// =====================================================================
// fs_setActive()
// =====================================================================
void fs_setActive(bool on)
{
    cfg_fs_active = on;
    sendInfo("FS", "act", "*", on ? "1" : "0"); // $I:FS:*:act:0|1#
}

// =====================================================================
// fs_setFreq()
// ms=0 → désactive la publication périodique (Table A L54 note)
// reste active pour alerte
// =====================================================================
void fs_setFreq(unsigned long ms)
{
    cfg_fs_freq_ms = ms;               // 0 = mode "pour alerte", géré dans processPeriodic
    sendInfo("FS", "freq", "*", "OK"); // $I:FS:*:freq:OK#
}

// =====================================================================
// fs_setThreshold()
// =====================================================================
void fs_setThreshold(int th)
{
    cfg_fs_threshold = th;
    sendInfo("FS", "thd", "*", "OK"); // $I:FS:*:thd:OK#
}

// =====================================================================
// Accesseurs état interne
// =====================================================================
long fs_lastForce() { return _lastForce; }
int fs_lastAngle() { return 0; } // HX711 axe unique — angle non implémenté

// =====================================================================
// _handleAlert()
// Gère les transitions alerte et publie status + alerte.
// Appelé depuis fs_processPeriodic() après chaque lecture.
//
// Montée seuil :
//   $I:FS:*:status:alerte#          (Table A L58)
//   $I:FS:*:alerte:<val_force>#     (Table A L59 — valeur au dépassement)
//
// Descente seuil :
//   $I:FS:*:status:actif#           (Table A L58 — retour normal)
//   $I:FS:*:alerte:0#               (information : seuil levé)
// =====================================================================
static void _handleAlert(long force)
{
    bool over = (force > cfg_fs_threshold || force < -(long)cfg_fs_threshold);

    if (over && !_alertActive)
    {
        // Passage en alerte
        _alertActive = true;
        sendInfo("FS", "status", "*", "alerte"); // $I:FS:*:status:alerte#
        char buf[20];
        snprintf(buf, sizeof(buf), "%ld", force);
        sendInfo("FS", "alerte", "*", buf); // $I:FS:*:alerte:<val>#
    }
    else if (!over && _alertActive)
    {
        // Retour sous le seuil
        _alertActive = false;
        sendInfo("FS", "status", "*", "actif"); // $I:FS:*:status:actif#
        sendInfo("FS", "alerte", "*", "0");     // $I:FS:*:alerte:0# (seuil levé)
    }
}

// =====================================================================
// fs_processPeriodic()
// Appelé dans loop() à chaque tour — non bloquant.
// freq=0 : publication désactivée, mais alerte toujours surveillée.
// =====================================================================
void fs_processPeriodic()
{
    static unsigned long lastT = 0;

    if (!cfg_fs_active)
        return;

    unsigned long now = millis();

    // ================================
    // MODE freq = 0 → ALERT ONLY
    // ================================
    if (cfg_fs_freq_ms == 0)
    {
        long raw;

        if (fs_hw_readRaw(raw))
        {
            long force = (cfg_fs_calibration != 0)
                             ? (raw - cfg_fs_offset) / cfg_fs_calibration
                             : 0;
            _handleAlert(force);
        }

        return; // PAS de publication périodique
    }

    // ================================
    // MODE NORMAL périodique
    // ================================
    if (now - lastT < cfg_fs_freq_ms)
        return;

    lastT = now;

    long raw;

    if (!fs_hw_readRaw(raw))
        return;

    long force = (cfg_fs_calibration != 0)
                     ? (raw - cfg_fs_offset) / cfg_fs_calibration
                     : 0;

    // 1. publication valeur
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", force);
    sendFlux("FS", "force", "*", buf);

    // 2. gestion alerte
    _handleAlert(force);
}