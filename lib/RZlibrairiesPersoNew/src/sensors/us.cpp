/************************************************************
 * FICHIER : src/sensors/us.cpp
 * ROLE    : Logique haute-niveau pour les sonars (module Us)
 ************************************************************/
#include "us.h"
#include "hardware/us_hardware.h"
#include "system/urg.h"
#include "communication/communication.h"
#include "communication/vpiv_utils.h"

// valeurs internes
static int lastReported[US_NUM_SENSORS];
static int currValue[US_NUM_SENSORS];

static unsigned long suspectSince[US_NUM_SENSORS];
static bool sensorDisabled[US_NUM_SENSORS];

static unsigned long lastCycle = 0;

// ----------------------------------------------------------
// Envoi d’une valeur unique : $I:Us:one:<idx>:<val>
// ----------------------------------------------------------
static void send_one(int i, int v)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", v);

    char inst[4];
    snprintf(inst, sizeof(inst), "%d", i);

    sendInfo("Us", "one", inst, buf);
}

// ----------------------------------------------------------
// Envoi groupé : $I:Us:val:*:v1,v2,v3…
// ----------------------------------------------------------
static void send_all()
{
    char buf[128];
    buf[0] = 0;

    for (int i = 0; i < US_NUM_SENSORS; i++)
    {
        char t[8];
        snprintf(t, sizeof(t), "%d", sensorDisabled[i] ? -1 : currValue[i]);
        strcat(buf, t);
        if (i < US_NUM_SENSORS - 1)
            strcat(buf, ",");
    }

    sendInfo("Us", "val", "*", buf);
}

// ----------------------------------------------------------
// Init
// ----------------------------------------------------------
void us_init()
{
    us_initHardware();

    for (int i = 0; i < US_NUM_SENSORS; i++)
    {
        lastReported[i] = US_UNREPORTED;
        currValue[i] = US_SILENCE;
        suspectSince[i] = 0;
        sensorDisabled[i] = false;
    }

    lastCycle = millis();
}

// ----------------------------------------------------------
// Boucle périodique sonar
// ----------------------------------------------------------
void us_processPeriodic()
{
    if (!cfg_us_active)
        return;

    if (urg_isActive())
        return;

    unsigned long now = millis();
    if (now - lastCycle < cfg_us_ping_cycle_ms)
        return;
    lastCycle = now;

    for (int i = 0; i < US_NUM_SENSORS; i++)
    {
        if (sensorDisabled[i])
        {
            currValue[i] = US_DISABLED;
            continue;
        }

        int d = us_measureOnce(i);

        // ----- gestion du mode suspect -----
        if (d < 0)
        {
            if (suspectSince[i] == 0)
                suspectSince[i] = now;

            if (now - suspectSince[i] >= cfg_us_suspect_timeout_ms)
            {
                sensorDisabled[i] = true;
                currValue[i] = US_DISABLED;
                send_one(i, US_DISABLED);
                lastReported[i] = US_DISABLED;
                continue;
            }

            currValue[i] = US_SILENCE;
            continue;
        }

        // mesure valide
        suspectSince[i] = 0;
        currValue[i] = d;

        // ----- système d’alerte danger -----
        // Conversion complète en uint32_t pour éviter warning signed/unsigned
        uint32_t d_u = static_cast<uint32_t>(d);
        uint32_t dangerThreshold = static_cast<uint32_t>(cfg_us_danger_thresholds[i]) *
                                   static_cast<uint32_t>(cfg_us_alert_multiplier[i]);

        if (d_u <= dangerThreshold)
        {
            int lvl = max(1, cfg_us_danger_thresholds[i] / max(1, d));
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", lvl);
            sendInfo("Us", "alert", String(i).c_str(), buf);
        }
    }

    // envoi groupé
    if (cfg_us_send_all)
    {
        send_all();
        for (int i = 0; i < US_NUM_SENSORS; i++)
            lastReported[i] = currValue[i];
        return;
    }

    // envoi des variations significatives
    for (int i = 0; i < US_NUM_SENSORS; i++)
    {
        int c = currValue[i];
        int p = lastReported[i];

        if (p == US_UNREPORTED)
        {
            send_one(i, c);
            lastReported[i] = c;
            continue;
        }

        if (c == US_DISABLED && p != US_DISABLED)
        {
            send_one(i, c);
            lastReported[i] = c;
            continue;
        }

        if (c == US_SILENCE)
            continue;

        if (abs(c - p) >= cfg_us_delta_threshold)
        {
            send_one(i, c);
            lastReported[i] = c;
        }
    }
}

// ----------------------------------------------------------
// Lecture forcée de certaines instances
// ----------------------------------------------------------
void us_handleReadList(const int *idxs, int n)
{
    if (n == 0) // "*" -> tous
    {
        for (int i = 0; i < US_NUM_SENSORS; i++)
        {
            int v = us_measureOnce(i);
            if (sensorDisabled[i])
                v = US_DISABLED;
            else if (v == 0)
                v = US_SILENCE;
            send_one(i, v);
        }
        return;
    }

    // liste explicite
    for (int k = 0; k < n; k++)
    {
        int i = idxs[k];
        if (i < 0 || static_cast<uint32_t>(i) >= US_NUM_SENSORS)
            continue;

        int v = us_measureOnce(i);
        if (sensorDisabled[i])
            v = US_DISABLED;
        else if (v == 0)
            v = US_SILENCE;

        send_one(i, v);
    }
}

// ----------------------------------------------------------
// Lecture d’une valeur courante
// ----------------------------------------------------------
int us_peekCurrValue(int idx)
{
    if (idx < 0 || static_cast<uint32_t>(idx) >= US_NUM_SENSORS)
        return -1;

    int v = currValue[idx];
    if (sensorDisabled[idx])
        return -1;

    return v;
}