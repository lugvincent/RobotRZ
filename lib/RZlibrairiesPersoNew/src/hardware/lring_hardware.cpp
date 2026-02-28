/************************************************************
 * FICHIER : src/hardware/lring_hardware.cpp
 * ROLE    : Implémentation bas-niveau du ring LED
 *
 * Ce fichier ne contient AUCUNE logique métier :
 *   - pas d’expressions (neutre, sourire…)
 *   - pas de gestion d’urgence
 *   - pas de mode EXTERN/EXPR
 *   - pas de timeout
 *
 * Il fournit uniquement les primitives matérielles :
 *   - initialisation du NeoPixel
 *   - écrire une LED
 *   - remplir le ring
 *   - appliquer la luminosité
 *   - afficher (show)
 *
 * L’objectif est d’avoir un module matériel parfaitement
 * stable et prévisible, utilisé par lring.cpp.
 ************************************************************/

#include "lring_hardware.h"
#include "../config/config.h"     // LRING_PIN, LRING_NUM_PIXELS, cfg_lring_brightness

/* ==========================================================
 * OBJET NEOPIXEL
 * ========================================================== */

Adafruit_NeoPixel ringPixels(
    LRING_NUM_PIXELS,
    LRING_PIN,
    NEO_GRB + NEO_KHZ800
);

/* ==========================================================
 * ÉTAT LOCAL (simple indicateur activé/désactivé)
 * ========================================================== */
static bool ringActive = false;

/* ==========================================================
 * INITIALISATION
 * ========================================================== */

void lringhw_init() {
    ringPixels.begin();
    ringPixels.setBrightness(cfg_lring_brightness);
    ringPixels.clear();
    ringPixels.show();
    ringActive = true;
}

/* ==========================================================
 * ÉCRITURE D’UNE LED
 * ========================================================== */

void lringhw_setPixel(int idx, int r, int g, int b) {
    if (idx < 0 || idx >= LRING_NUM_PIXELS) return;
    ringPixels.setPixelColor(idx, ringPixels.Color(r, g, b));
}

/* ==========================================================
 * REMPLIR LE RING ENTIER
 * ========================================================== */

void lringhw_fill(int r, int g, int b) {
    for (int i = 0; i < LRING_NUM_PIXELS; i++)
        ringPixels.setPixelColor(i, ringPixels.Color(r, g, b));
}

/* ==========================================================
 * CLEAR (mettre toutes les LEDs à noir)
 * ========================================================== */

void lringhw_clear() {
    ringPixels.clear();
}

/* ==========================================================
 * AFFICHER LES MODIFICATIONS
 * ========================================================== */

void lringhw_show() {
    ringPixels.show();
}

/* ==========================================================
 * ACTIVE / DESACTIVE COMPLETEMENT LE MODULE
 * ========================================================== */

void lringhw_setActive(bool on) {
    ringActive = on;
    if (!on) {
        ringPixels.clear();
        ringPixels.show();
    }
}

/* ==========================================================
 * RÉGLAGE DE LA LUMINOSITÉ (0–255)
 * ========================================================== */

void lringhw_setBrightness(int intensity) {
    if (intensity < 0)   intensity = 0;
    if (intensity > 255) intensity = 255;

    cfg_lring_brightness = intensity;
    ringPixels.setBrightness(intensity);
    ringPixels.show();
}

/* ==========================================================
 * STATUT
 * ========================================================== */

bool lringhw_isActive() {
    return ringActive;
}
