/************************************************************
 * FICHIER : src/hardware/mic_hardware.cpp
 * ROLE    : Sampling non-bloquant pour les microphones (3 canaux)
 *
 * Principe :
 *  - Un sampler par micro collecte des échantillons répartis
 *    sur les appels pour éviter blocage.
 *  - A la fin de la fenêtre (cfg_mic_win_ms) on calcule raw, peak, rms
 *    (tous en entiers) et on marque le résultat prêt à être lu.
 ************************************************************/

#include "mic_hardware.h"
#include "config/config.h"
#include <Arduino.h>
#include <stdint.h>

// Tableau de pins (défini dans config.h)
const int MIC_PINS[MIC_COUNT] = {
    MIC_AV_PIN, // 0 = Avant
    MIC_G_PIN,  // 1 = Gauche
    MIC_D_PIN   // 2 = Droite
};

// Dernières valeurs publiées (extern dans header)
int mic_lastRaw[MIC_COUNT] = {0, 0, 0};
// Déclaration initiale correcte :  int
int mic_lastRMS[MIC_COUNT] = {0, 0, 0};
int mic_lastPeak[MIC_COUNT] = {0, 0, 0};

// ---------- structure de sampling ----------
struct MicSampler
{
    // accumuteurs
    int64_t sum;   // somme des samples
    int64_t sumSq; // somme des carrés
    int peak;      // peak courant dans la fenêtre
    uint32_t n;    // nombre d'échantillons accumulés

    // timing fenêtre
    unsigned long winStartMs;
    unsigned long winEndMs;

    // flag résultat prêt
    bool resultReady;
    int lastRaw;  // moyenne entière
    int lastRms;  // RMS entier
    int lastPeak; // peak
};

static MicSampler samplers[MIC_COUNT];

// integer sqrt (entier) - Babylonian / Newton method
static uint32_t isqrt_u64(uint64_t x)
{
    if (x == 0)
        return 0;
    uint64_t r = x;
    uint64_t s = (r + 1) >> 1;
    while (s < r)
    {
        r = s;
        s = (r + x / r) >> 1;
    }
    return (uint32_t)r;
}

// initialisation hardware (appelée depuis mic_init())
void mic_initHardware()
{
    for (int i = 0; i < MIC_COUNT; ++i)
    {
        // init state
        samplers[i].sum = 0;
        samplers[i].sumSq = 0;
        samplers[i].peak = 0;
        samplers[i].n = 0;
        samplers[i].winStartMs = millis();
        samplers[i].winEndMs = samplers[i].winStartMs + cfg_mic_win_ms;
        samplers[i].resultReady = false;

        // initial read
        int v = analogRead(MIC_PINS[i]);
        mic_lastRaw[i] = v;
        mic_lastPeak[i] = v;
        mic_lastRMS[i] = 0;
    }
}

/**
 * mic_sampler_pollOne(index)
 *  - appelé fréquemment (ex : depuis mic_processPeriodic / loop)
 *  - lit un échantillon et l'ajoute aux accumulateurs
 *  - si la fenêtre est terminée, calcule result (raw, rms, peak)
 */
void mic_sampler_pollOne(int idx)
{
    if (idx < 0 || idx >= MIC_COUNT)
        return;

    unsigned long now = millis();

    // lecture hardware
    int v = analogRead(MIC_PINS[idx]);

    MicSampler &s = samplers[idx];
    s.sum += v;
    s.sumSq += (int64_t)v * (int64_t)v;
    if (v > s.peak)
        s.peak = v;
    s.n++;

    // vérifier fin de fenêtre
    if (now >= s.winEndMs)
    {
        // calc moyenne entière
        int raw = (s.n == 0) ? 0 : (int)(s.sum / (int64_t)s.n);

        // variance numerator = sumSq * n - sum * sum
        // RMS = isqrt(numerator) / n  (entier)
        uint64_t numerator = 0;
        if (s.n > 0)
        {
            // sumSq * n and sum*sum may be big -> use 128 not available: use 64-bit carefully
            // sumSq * n fits in 64-bit for reasonable n (n ~ few thousands)
            numerator = (uint64_t)s.sumSq * (uint64_t)s.n;
            uint64_t sumsq = (uint64_t)s.sum * (uint64_t)s.sum;
            if (numerator > sumsq)
                numerator = numerator - sumsq;
            else
                numerator = 0;
        }

        uint32_t rms_raw = 0;
        if (s.n > 0)
        {
            uint32_t isq = isqrt_u64(numerator);
            rms_raw = isq / s.n;
        }

        // store results
        s.lastRaw = raw;
        s.lastRms = (int)rms_raw;
        s.lastPeak = s.peak;
        s.resultReady = true;

        // copy to public variables (atomique-ish)
        mic_lastRaw[idx] = s.lastRaw;
        mic_lastRMS[idx] = s.lastRms;
        mic_lastPeak[idx] = s.lastPeak;

        // reset accumulators for next window
        s.sum = 0;
        s.sumSq = 0;
        s.peak = 0;
        s.n = 0;
        s.winStartMs = now;
        s.winEndMs = now + cfg_mic_win_ms;
    }
}

/**
 * mic_sampler_pollAll(maxPerCall)
 *  - appel pratique : échantillonne un sample par micro par appel
 *  - si tu veux plus d'échantillons par appel, tu peux appeler plusieurs fois
 */
void mic_sampler_pollAll()
{
    // On fait 1 sample par micro et on retourne : distribution "fair"
    for (int i = 0; i < MIC_COUNT; ++i)
    {
        mic_sampler_pollOne(i);
    }
}
