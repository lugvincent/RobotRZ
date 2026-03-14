/************************************************************
 * FICHIER  : actuators/lrub.h
 * CHEMIN   : lib/RZlibrairiesPersoNew/src/actuators/lrub.h
 * VERSION  : 1.2  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   API haut-niveau du ruban LED (LRUB).
 *   Fournit les fonctions utilisées par dispatch_Lrub.cpp et loop().
 *
 * MODES
 * -----
 *   OFF          : ruban éteint
 *   STATIC       : couleur fixe
 *   EXTERN       : couleur pilotée depuis l'extérieur (VPIV)
 *   EFFET_FUITE  : alternance pairs/impairs (effet fuite)
 *
 * ARTICULATION
 * ------------
 *   dispatch_Lrub.cpp → lrub.h → lrub.cpp → lrub_hardware.cpp
 *   loop() appelle lrub_processTimeout() à chaque tour
 *
 * CORRECTION v1.2
 * ---------------
 *   Suppression de la déclaration lrub_dispatchVPIV() :
 *   - Cette fonction était un "dispatcher helper" dupliquant dispatch_Lrub.cpp
 *   - Elle n'était appelée nulle part dans le projet (code mort confirmé)
 *   - Son implémentation dans lrub.cpp utilisait encore l'ancien nom "int"
 *     au lieu de "lumin", créant un risque de confusion
 *   - L'implémentation correspondante dans lrub.cpp est également supprimée
 *
 * PRÉCAUTIONS
 * -----------
 *   - lrub_processTimeout() DOIT être appelé dans loop() pour le timeout.
 *   - Ne pas appeler lrub_hardware.h directement depuis le dispatcher —
 *     passer par cette API.
 ************************************************************/

#ifndef ACTUATORS_LRUB_H
#define ACTUATORS_LRUB_H

#include <stdint.h>

/* Initialisation (appel dans setup via RZ_initAll) */
void lrub_init();

/* Appliquer couleur à tout le ruban immédiatement (R,G,B 0..255) */
void lrub_applyColorAll(int r, int g, int b);

/* Appliquer couleur à une liste d'indices */
void lrub_applyColorIndices(const int *indices, int nIndices, int r, int g, int b);

/* Régler l'intensité globale (0..255) — n'affecte pas la couleur */
void lrub_setIntensity(int intensity);

/* Activer / désactiver le ruban */
void lrub_setActive(bool on);

/* Définir un timeout absolu (millis). 0 = pas de timeout.
   Exemple : lrub_setTimeout(millis() + 5000UL) */
void lrub_setTimeout(unsigned long expireAtMs);

/* À appeler dans loop() — gère l'extinction automatique par timeout */
void lrub_processTimeout();

/* Appliquer l'effet "fuite" (alternance pairs/impairs) — non bloquant */
void lrub_effectFuite();

#endif // ACTUATORS_LRUB_H