/************************************************************
 * FICHIER : src/hardware/odom_hardware.h
 * ROLE    : Interface matérielle pour l'odométrie (encodeurs)
 *
 * Chemin  : src/hardware/odom_hardware.h
 *
 * Fournit :
 *   - init des encodeurs
 *   - lecture des ticks (accumulation sécurisée)
 *   - reset des compteurs
 *
 * Utilise la bibliothèque Encoder.h (installable via Library Manager).
 ************************************************************/
#ifndef ODOM_HARDWARE_H
#define ODOM_HARDWARE_H

#include <Arduino.h>
#include <Encoder.h>

// --- Prototypes publics pour la couche hardware ---
void odom_hw_init();                // à appeler dans setup()
long odom_hw_getAndClearLeftTicks();  // retourne delta ticks depuis dernier appel
long odom_hw_getAndClearRightTicks();
void odom_hw_forceClear();          // remettre compteurs à zéro (tare)
long odom_hw_peekLeftTicks();       // lire sans effacer
long odom_hw_peekRightTicks();

#endif
