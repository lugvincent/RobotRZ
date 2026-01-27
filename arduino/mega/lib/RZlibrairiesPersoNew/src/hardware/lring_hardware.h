/**********************************************************
 * FICHIER : src/hardware/lring_hardware.h
 * ROLE    : Interface matérielle brute pour le LED ring
 ************************************************************/

#ifndef L_RING_HARDWARE_H
#define L_RING_HARDWARE_H

#include <Adafruit_NeoPixel.h>

/* ==========================================================
 * PRIMITIVES MATÉRIELLES
 * ========================================================== */

// Initialisation matérielle
void lringhw_init();

// Activation / désactivation (clear)
void lringhw_setActive(bool on);

// LED individuelle
void lringhw_setPixel(int idx, int r, int g, int b);

// Remplir tout le ring
void lringhw_fill(int r, int g, int b);

// Effacer (noir)
void lringhw_clear();

// Appliquer (show)
void lringhw_show();

// Luminosité globale
void lringhw_setBrightness(int intensity);

// Etat matériel
bool lringhw_isActive();

#endif
