/************************************************************
 * FICHIER : src/safety/mvt_safe.h
 * ROLE    : Module anti-collision basé sur capteurs US
 *           (Mvt-Safe)
 ************************************************************/
#ifndef MVT_SAFE_H
#define MVT_SAFE_H

#include <Arduino.h>

// États internes du module sécurité
enum MvtSafeState {
    MVT_SAFE_OK = 0,
    MVT_SAFE_WARN,
    MVT_SAFE_DANGER
};

// API publique
void mvtsafe_init();
void mvtsafe_process();

// Activation / mode
void mvtsafe_setActive(bool en);
void mvtsafe_setMode(const char* m);

// Seuils personnalisés
void mvtsafe_setThresholds(int warn, int danger);

// Limitation globale vitesse
void mvtsafe_setMaxScale(float s);

// Lecture état
int  mvtsafe_getState();

// Utilitaire interne : meilleur obstacle avant/arrière
int mvtsafe_getMinFront();
int mvtsafe_getMinRear();

#endif
