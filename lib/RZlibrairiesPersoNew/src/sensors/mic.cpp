/************************************************************
 * FICHIER : src/sensors/mic.cpp
 * ROLE    : Module micro non-bloquant (entiers)
 *
 * - Utilise mic_hardware sampler non bloquant
 * - Mic_processPeriodic() appelle mic_sampler_pollAll()
 * - Envoie VPIV (alert/orient/values) en entiers
 ************************************************************/

#include "mic.h"
#include "hardware/mic_hardware.h"
#include "config/config.h"
#include "communication/communication.h"
#include <Arduino.h>
#include <string.h>

// internal state for orientation smoothing
static int lastOrientation = 0;

// ----------------- VPIV helpers -----------------
static void send_val(const char *prop, int idx, int value)
{
    char inst[4];
    snprintf(inst, sizeof(inst), "%d", idx);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", value);
    sendInfo("Mic", prop, inst, buf);
}

static void send_orient(int angDeg)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", angDeg);
    sendInfo("Mic", "orient", "*", buf);
}

static void send_alert(int idx, int peak)
{
    char inst[4];
    snprintf(inst, sizeof(inst), "%d", idx);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", peak);
    sendInfo("Mic", "alert", inst, buf);
}

// initialisation
void mic_init()
{
    mic_initHardware();
}

// Lecture groupée : renvoie les dernières valeurs calculées (non bloquant)
void mic_handleReadList(const int *indices, int n, const char *type)
{
    const char *t = type ? type : "rms";

    // Si "*" -> all
    if (n == 0)
    {
        for (int i = 0; i < MIC_COUNT; ++i)
        {
            if (strcmp(t, "raw") == 0)
                send_val("raw", i, mic_lastRaw[i]);
            else if (strcmp(t, "peak") == 0)
                send_val("peak", i, mic_lastPeak[i]);
            else /*rms*/
                send_val("rms", i, mic_lastRMS[i]);
        }
        return;
    }

    // liste spécifique
    for (int k = 0; k < n; ++k)
    {
        int idx = indices[k];
        if (idx < 0 || idx >= MIC_COUNT)
            continue;
        if (strcmp(t, "raw") == 0)
            send_val("raw", idx, mic_lastRaw[idx]);
        else if (strcmp(t, "peak") == 0)
            send_val("peak", idx, mic_lastPeak[idx]);
        else
            send_val("rms", idx, mic_lastRMS[idx]);
    }
}

// réglages
void mic_setThreshold(const int *indices, int n, int th)
{
    if (n == 0)
    {
        for (int i = 0; i < MIC_COUNT; i++)
            cfg_mic_threshold[i] = th;
    }
    else
    {
        for (int k = 0; k < n; k++)
        {
            int idx = indices[k];
            if (idx >= 0 && idx < MIC_COUNT)
                cfg_mic_threshold[idx] = th;
        }
    }
}
void mic_setFreq(unsigned long ms) { cfg_mic_freq_ms = ms; }
void mic_setWin(unsigned long ms)
{
    if (ms < 5)
        ms = 5;
    cfg_mic_win_ms = ms;
}
void mic_setActive(bool on) { cfg_mic_active = on; }

// Orientation estimation (integer) : -90..+90
static int computeOrientation()
{
    // use last RMS values to compute weighted angle:
    // indices: 0=avant (0deg), 1=gauche (-90), 2=droite (+90)
    int rms0 = mic_lastRMS[0];
    int rms1 = mic_lastRMS[1];
    int rms2 = mic_lastRMS[2];

    // add small bias for stability
    int w0 = rms0 + 1;
    int w1 = rms1 + 1;
    int w2 = rms2 + 1;

    long num = (long)w0 * 0 + (long)w1 * (-90) + (long)w2 * 90;
    long den = (long)w0 + (long)w1 + (long)w2;
    if (den == 0)
        return 0;
    int ang = (int)(num / den);

    // smoothing simple (1st order)
    lastOrientation = (lastOrientation * 3 + ang) / 4;
    return lastOrientation;
}

// périodique non-bloquant : doit être appelé souvent (loop)
void mic_processPeriodic()
{
    static unsigned long lastPublish = 0;

    if (!cfg_mic_active)
        return;

    // 1) poll hardware samplers (1 sample per mic per call)
    mic_sampler_pollAll();
    // 2) envoyer alertes si peak dépasse threshold (on cadence cfg_mic_freq_ms)
    unsigned long now = millis();
    if (now - lastPublish < cfg_mic_freq_ms)
        return;
    lastPublish = now;

    // parcourir micros
    for (int i = 0; i < MIC_COUNT; ++i)
    {
        int peak = mic_lastPeak[i];
        if (peak > cfg_mic_threshold[i])
        {
            send_alert(i, peak);
        }
    }

    // envoyer orientation régulière (optionnel)
    int ang = computeOrientation();
    send_orient(ang);
}
