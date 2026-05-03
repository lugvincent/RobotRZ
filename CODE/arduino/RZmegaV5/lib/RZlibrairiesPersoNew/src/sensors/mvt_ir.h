// FICHIER : src/sensors/mvt_ir.h
// ROLE : API haut-niveau logique pour le module MvtIR
// Chemin : src/sensors/mvt_ir.h

#ifndef MVT_IR_H
#define MVT_IR_H

#include <Arduino.h>

// Initialisation du module (appel dans setup)
void mvt_ir_init();

// Lecture instantanée (envoie VPIV, renvoie la valeur lue)
void mvt_ir_readInstant();

// Activation / désactivation
void mvt_ir_setActive(bool on);

// Réglage fréquence en ms (périodique)
void mvt_ir_setFreqMs(unsigned long ms);

// Réglage seuil (nombre d'impulsions consec.)
void mvt_ir_setThreshold(int n);

// Réglage rapide du mode ("idle","monitor","surveillance")
void mvt_ir_setMode(const char* mode);

// Process périodique à appeler depuis loop()
void mvt_ir_processPeriodic();

#endif // MVT_IR_H
