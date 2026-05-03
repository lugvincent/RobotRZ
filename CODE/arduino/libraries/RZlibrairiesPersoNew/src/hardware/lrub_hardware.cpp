/************************************************************
 * FICHIER : src/hardware/lrub_hardware.cpp
 * ROLE    : Implémentation matérielle du ruban LED (NeoPixel)
 *
 * Aucune logique métier ici :
 *  - pas de mode
 *  - pas de timeout
 *  - pas d'effets
 *
 * Ce fichier fournit uniquement les primitives bas-niveau :
 *  - init
 *  - setPixel
 *  - fill
 *  - clear
 *  - show
 *  - brightness
 *  - active / inactive
 ************************************************************/

#include "lrub_hardware.h"
#include "../config/config.h"

Adafruit_NeoPixel lrubPixels(
    LRUB_NUM_PIXELS,
    LRUB_PIN,
    NEO_GRB + NEO_KHZ800
);

static bool lrubActive = false;

/* ==========================================================
 * INITIALISATION
 * ========================================================== */
void lrubhw_init() {
    lrubPixels.begin();
    lrubPixels.setBrightness(cfg_lrub_brightness);
    lrubPixels.clear();
    lrubPixels.show();
    lrubActive = true;
}

/* ==========================================================
 * ÉCRITURE PIXEL
 * ========================================================== */
void lrubhw_setPixel(int idx, int r, int g, int b) {
    if (idx < 0 || idx >= LRUB_NUM_PIXELS) return;
    lrubPixels.setPixelColor(idx, lrubPixels.Color(r, g, b));
}

/* ==========================================================
 * REMPLIR TOUT LE RUBAN
 * ========================================================== */
void lrubhw_fill(int r, int g, int b) {
    for (int i = 0; i < LRUB_NUM_PIXELS; i++)
        lrubPixels.setPixelColor(i, lrubPixels.Color(r, g, b));
}

/* ==========================================================
 * CLEAR = effacer
 * ========================================================== */
void lrubhw_clear() {
    lrubPixels.clear();
}

/* ==========================================================
 * SHOW = appliquer
 * ========================================================== */
void lrubhw_show() {
    lrubPixels.show();
}

/* ==========================================================
 * ACTIVE / DESACTIVE
 * ========================================================== */
void lrubhw_setActive(bool on) {
    lrubActive = on;
    if (!on) {
        lrubPixels.clear();
        lrubPixels.show();
    }
}

/* ==========================================================
 * BRIGHTNESS 0..255
 * ========================================================== */
void lrubhw_setBrightness(int intensity) {
    if (intensity < 0) intensity = 0;
    if (intensity > 255) intensity = 255;

    cfg_lrub_brightness = intensity;
    lrubPixels.setBrightness(intensity);
    lrubPixels.show();
}

/* ==========================================================
 * ÉTAT
 * ========================================================== */
bool lrubhw_isActive() {
    return lrubActive;
}
