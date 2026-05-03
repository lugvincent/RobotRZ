/************************************************************
 * FICHIER : src/actuators/lring.cpp
 * ROLE    : Implémentation des expressions du ring LED
 *
 * Implémente les expressions demandées :
 *  - "voyant"    : LED index 6 clignote vert 1s ON / 1s OFF (répétitif)
 *  - "eclairage" : toutes LEDs = blanc plein intensité (persistant)
 *  - "alerte"    : (déclenchée par urg) 3 clignotements rouges (all leds),
 *                  puis si urgence persistante -> clignotement sur LED 0
 *  - "neutre"    : LEDs 5,6,7 en vert continu (puissance moyenne)
 *  - "sourire"   : LEDs 4..8 en vert pendant 2s puis retour à "neutre"
 *  - "triste"    : LEDs 10,11,0,1,2 en orange pendant 2s puis retour à "neutre"
 *
 * Comportement :
 *  - une nouvelle commande remplace l'effet en cours
 *  - timeoutMs == 0 => effet persistant jusqu'à nouvelle commande
 *  - lring_process() doit être appelé régulièrement (loop)
 ************************************************************/

#include "lring.h"
#include "../hardware/lring_hardware.h"
#include "../configuration/config.h"
#include "../communication/communication.h"
#include <string.h>

// On utilise ces constantes pour lisibilité
static const int NUM_PIX = LRING_NUM_PIXELS;

// Etat interne
static char currentExpr[32] = "none"; // nom expr courant
static unsigned long exprEndTs = 0;   // ms timestamp quand l'effet doit s'arrêter (0 = persistant)
static bool exprActive = false;       // si effet en cours
static unsigned long lastBlinkTs = 0; // pour clignotements
static bool blinkState = false;       // état ON/OFF pour blink
static int alertPhaseCount = 0;       // compteur pour la séquence initiale d'alerte (3 clignotements)
static bool alertSecondStage = false; // si urgence persistante -> second stage (led 0 blink)
static unsigned long lastProcessTs = 0;

// helper couleur
static inline void _setRGB(int idx, int r, int g, int b) {
    if (idx < 0 || idx >= NUM_PIX) return;
    lring_setPixelColor(idx, r, g, b);
}

// applique la couleur neutre (config)
static void applyNeutral() {
    // neutre défini : leds 5,6,7 en vert puissance moyenne
    lring_fillColor(0,0,0); // clear others
    int med = cfg_lring_brightness / 2;
    for (int i=0;i<NUM_PIX;i++) lring_setPixelColor(i,0,0,0);
    lring_setPixelColor(5, 0, med, 0);
    lring_setPixelColor(6, 0, med, 0);
    lring_setPixelColor(7, 0, med, 0);
    lring_show();
}

// small utility to set all leds to color
static void fillAll(int r,int g,int b) {
    lring_fillColor(r,g,b);
    lring_show();
}

// Called by public API to set expression
void lring_applyExpression(const char* expr, unsigned long timeoutMs) {
    if (!expr) return;

    // normalize to lower-case simple
    char name[32];
    strncpy(name, expr, sizeof(name)-1);
    name[sizeof(name)-1] = '\0';
    for (char* p = name; *p; ++p) *p = tolower((unsigned char)*p);

    // Store current expression
    strncpy(currentExpr, name, sizeof(currentExpr)-1);
    currentExpr[sizeof(currentExpr)-1] = '\0';
    exprActive = true;
    lastBlinkTs = millis();
    blinkState = false;
    alertPhaseCount = 0;
    alertSecondStage = false;

    if (timeoutMs == 0) exprEndTs = 0;
    else exprEndTs = millis() + timeoutMs;

    // Apply immediate visual according to expr
    if (strcmp(name, "voyant") == 0) {
        // we initialize LED 6 to off; blink handled in process()
        _setRGB(6, 0, 0, 0);
        lring_show();
        return;
    }

    if (strcmp(name, "eclairage") == 0) {
        // all white full intensity
        lring_setIntensity(255);
        fillAll(255,255,255);
        return;
    }

    if (strcmp(name, "alerte") == 0) {
        // start alert initial stage: 3 clignotements of all leds
        alertPhaseCount = 0;
        alertSecondStage = false;
        lastBlinkTs = millis();
        blinkState = false;
        // nothing else now, process() will do the pattern
        return;
    }

    if (strcmp(name, "neutre") == 0) {
        applyNeutral();
        return;
    }

    if (strcmp(name, "sourire") == 0) {
        // leds 4..8 green medium
        int med = cfg_lring_brightness / 2;
        for (int i=0;i<NUM_PIX;i++) lring_setPixelColor(i,0,0,0);
        for (int i=4;i<=8 && i<NUM_PIX;i++) lring_setPixelColor(i, 0, med, 0);
        lring_show();
        return;
    }

    if (strcmp(name, "triste") == 0) {
        // leds 10,11,0,1,2 orange (r=med, g=med/2)
        int med = cfg_lring_brightness / 2;
        int r = med;
        int g = med/2;
        for (int i=0;i<NUM_PIX;i++) lring_setPixelColor(i,0,0,0);
        int idxs[] = {10,11,0,1,2};
        for (int j=0;j<5;j++) {
            int idx = idxs[j];
            if (idx >= 0 && idx < NUM_PIX) lring_setPixelColor(idx, r, g, 0);
        }
        lring_show();
        return;
    }

    // default: if unknown -> apply neutral
    applyNeutral();
}

// API init
void lring_init() {
    // Initialize hardware
    lring_initHardware();

    // Apply initial neutral or config color
    applyNeutral();

    exprActive = false;
    exprEndTs = 0;
    lastBlinkTs = millis();
    lastProcessTs = millis();
}

// emergency trigger (called by urg module)
void lring_emergencyTrigger(uint8_t urgCode) {
    // override current expression with "alerte"
    lring_applyExpression("alerte", 0); // persistent until urg cleared + process handles second stage
}

// return current expression name
const char* lring_currentExpression() {
    return currentExpr;
}

// process: manage blinking and timeouts; must be called often from loop()
void lring_processPeriodic() {
    unsigned long now = millis();

    // small guard: run at max ~20Hz to reduce work
    if (now - lastProcessTs < 30) return;
    lastProcessTs = now;

    if (!exprActive) return;

    // check timeout
    if (exprEndTs != 0 && now >= exprEndTs) {
        // timed out -> revert to neutral
        exprActive = false;
        exprEndTs = 0;
        strncpy(currentExpr, "neutre", sizeof(currentExpr));
        applyNeutral();
        return;
    }

    // handle patterns requiring time (voyant, alerte)
    if (strcmp(currentExpr, "voyant") == 0) {
        // period 2000 ms, ON for 1000ms
        if (now - lastBlinkTs >= 1000) {
            // toggle every 1000ms
            lastBlinkTs = now;
            blinkState = !blinkState;
            if (blinkState) {
                // ON: led 6 green medium
                int med = cfg_lring_brightness / 2;
                lring_setPixelColor(6, 0, med, 0);
            } else {
                lring_setPixelColor(6, 0, 0, 0);
            }
            lring_show();
        }
        return;
    }

    if (strcmp(currentExpr, "alerte") == 0) {
        // initial stage: 3 clignotements of whole ring:
        // we interpret "3 clignotement rouge of 1s every 2s" as:
        // each cycle = 2000ms (1s ON, 1s OFF). Do 3 cycles (3*2s = 6s).
        // After these 3 cycles, if urgency still active (cfg_urg_active true),
        // switch to second stage: blink only LED 0 at same rhythm.
        unsigned long cycleMs = 2000UL;
        unsigned long halfMs = 1000UL; // on/off duration
        // determine which cycle we are in
        unsigned long elapsedFromStart = now - (exprEndTs ? exprEndTs - 0 : 0); // exprEndTs might be 0; we don't rely on it
        // simpler: use lastBlinkTs + toggles counting via alertPhaseCount/time
        // We'll implement with time-based toggling and counting on rising edges.
        if (now - lastBlinkTs >= halfMs) {
            lastBlinkTs = now;
            blinkState = !blinkState; // toggle state
            if (blinkState) {
                // rising edge (ON): if alertPhaseCount < 3*2? we'll count ON events
                alertPhaseCount++;
            }
            // Render ON/OFF depending on stage
            if (!alertSecondStage) {
                // initial stage: whole ring blink
                if (blinkState) fillAll(cfg_lring_red ? cfg_lring_red : 255, 0, 0); // red
                else fillAll(0,0,0);
            } else {
                // second stage: blink only LED 0
                if (blinkState) {
                    // LED0 red
                    for (int i=0;i<NUM_PIX;i++) lring_setPixelColor(i,0,0,0);
                    lring_setPixelColor(0, cfg_lring_red ? cfg_lring_red : 255, 0, 0);
                    lring_show();
                } else {
                    // clear LED0
                    lring_setPixelColor(0,0,0,0);
                    lring_show();
                }
            }

            // We count ON events as part of the initial phase; we want 3 full cycles.
            // Each cycle has 1 ON and 1 OFF, with ON toggles counted here; after 3 ON toggles => 3 cycles.
            if (!alertSecondStage && alertPhaseCount >= 3) {
                // switch to second stage if urg still active, otherwise return to neutral
                if (cfg_urg_active) {
                    alertSecondStage = true;
                    alertPhaseCount = 0;
                    // ensure LED0 clear now; next toggles will blink it
                    for (int i=0;i<NUM_PIX;i++) lring_setPixelColor(i,0,0,0);
                    lring_show();
                } else {
                    // urg cleared -> revert to neutral
                    exprActive = false;
                    strncpy(currentExpr, "neutre", sizeof(currentExpr));
                    applyNeutral();
                }
            }
        }
        return;
    }

    // no special processing for other persistent expressions
}

