/************************************************************
 * FICHIER : src/hardware/odom_hardware.cpp
 * ROLE    : Implémentation hardware odométrie (encodeurs)
 *
 * Chemin  : src/hardware/odom_hardware.cpp
 *
 * Notes :
 *  - S'appuie sur Encoder (manages quadrature)
 *  - Les pins utilisés doivent être compatibles MEGA (interrupts)
 *  - Les variables de pin peuvent être prises depuis config.h si besoin
 ************************************************************/

#include "odom_hardware.h"
#include "../configuration/config.h"
#include <Encoder.h>

// === CONFIGURABLE (si tu veux, mets ces pins dans config.h) ===
// Ici on reprend l'usage classique (valeurs à adapter si nécessaire)
#ifndef ODOM_LEFT_A_PIN
#define ODOM_LEFT_A_PIN  2
#define ODOM_LEFT_B_PIN  19
#define ODOM_RIGHT_A_PIN 18
#define ODOM_RIGHT_B_PIN 3
#endif

// Encoders (objet global hardware)
static Encoder encLeft(ODOM_LEFT_A_PIN, ODOM_LEFT_B_PIN);
static Encoder encRight(ODOM_RIGHT_A_PIN, ODOM_RIGHT_B_PIN);

// stockage des dernières lectures lues par le "getAndClear" (protection simple)
static long lastReadLeft = 0;
static long lastReadRight = 0;

void odom_hw_init() {
    // initialisation matérielle minimale — Encoder gère pinMode interne
    lastReadLeft = encLeft.read();
    lastReadRight = encRight.read();
}

// retourne delta ticks depuis dernier getAndClear (atomique côté utilisateur)
long odom_hw_getAndClearLeftTicks() {
    long now = encLeft.read();
    long delta = now - lastReadLeft;
    lastReadLeft = now;
    return delta;
}
long odom_hw_getAndClearRightTicks() {
    long now = encRight.read();
    long delta = now - lastReadRight;
    lastReadRight = now;
    return delta;
}

// peek sans clear
long odom_hw_peekLeftTicks() {
    return encLeft.read();
}
long odom_hw_peekRightTicks() {
    return encRight.read();
}

// force clear -> remet le repère (tare) (attention : Encoder::write nécessite lib support)
void odom_hw_forceClear() {
    encLeft.write(0);
    encRight.write(0);
    lastReadLeft = 0;
    lastReadRight = 0;
}
