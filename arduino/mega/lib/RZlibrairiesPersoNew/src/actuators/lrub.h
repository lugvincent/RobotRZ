/************************************************************
 *  : actuators-lrub.h
 * ROLE    : API haut-niveau du ruban LED (LRUB) - version SIMPLE
 *
 * - Mode simple : OFF / STATIC / EXTERN / EFFET_FUITE
 * - Timeout géré (extinction après délai si configuré)
 * - API minimale pour dispatcher VPIV
 ************************************************************/

#ifndef ACTUATORS_LRUB_H
#define ACTUATORS_LRUB_H

#include <stdint.h>

/* Initialisation (appel dans setup) */
void lrub_init();

/* Appliquer couleur à tout le ruban immédiatement (R,G,B 0..255) */
void lrub_applyColorAll(int r, int g, int b);

/* Appliquer couleur à une liste d'indices (indices tableau, nIndices = 0 => tous) */
void lrub_applyColorIndices(const int *indices, int nIndices, int r, int g, int b);

/* Mettre l'intensité (0..255) */
void lrub_setIntensity(int intensity);

/* Activer / désactiver le ruban (on/off) */
void lrub_setActive(bool on);

/* Définir un timeout absolu (millis). 0 => pas de timeout.
   Exemple : lrub_setTimeout(millis() + 5000UL) */
void lrub_setTimeout(unsigned long expireAtMs);

/* Process à appeler périodiquement pour gérer timeout (utilise lrub_processTimeout() dans ta loop) */
void lrub_processTimeout();

/* Appliquer l'effet simple "fuite" défini (non bloquant) */
void lrub_effectFuite();

/* Dispatcher helper (optionnel) : applique la même chose que dispatcher VPIV */
bool lrub_dispatchVPIV(const char *prop, const char *inst, const char *value);

#endif // ACTUATORS_LRUB_H
