//../sensors/us.h
#ifndef US_H
#define US_H

#include <Arduino.h>
#include "../config/config.h"

// valeurs spéciales
#define US_UNREPORTED -999 // jamais publié
#define US_DISABLED -1     // capteur en échec
#define US_SILENCE 0       // timeout pulseIn()

void us_init();
void us_processPeriodic();
void us_handleReadList(const int *idxs, int n);
// --- Ajout nécessaire ---
int us_peekCurrValue(int index);
#endif
