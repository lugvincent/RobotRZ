/************************************************************
 * FICHIER : src/safety/mvt_safe.h
 * ROLE    : Superviseur de sécurité anti-collision (mvtSafe)
 *
 * But :
 *  - Surveiller les distances ultrasoniques (via us_peekCurrValue())
 *  - Détecter warn / danger en fonction de seuils configurables
 *  - En mode "soft" réduire la consigne moteurs (ralentir)
 *  - En mode "hard" arrêter les moteurs et déclencher urgence si nécessaire
 *
 * API publique utilisée par dispatch_MvtSafe.cpp
 ************************************************************/

#ifndef MVT_SAFE_H
#define MVT_SAFE_H

#include <Arduino.h>

// Modes opérationnels
#define MVTSAFE_MODE_IDLE   0
#define MVTSAFE_MODE_SOFT   1   // ralentit
#define MVTSAFE_MODE_HARD   2   // arrêt immédiat

// États internes : simple enumeration renvoyée par status
#define MVTSAFE_STATE_OK      0
#define MVTSAFE_STATE_WARN    1
#define MVTSAFE_STATE_DANGER  2
#define MVTSAFE_STATE_DISABLED 3

// Initialisation (appel depuis setup())
void mvtsafe_init();

// Tick périodique (appel depuis loop())
void mvtsafe_process();

// Activation / configuration (appelé par dispatcher)
void mvtsafe_setActive(bool on);
void mvtsafe_setMode(const char* mode);                 // "idle", "soft", "hard"
void mvtsafe_setThresholds(int warn_cm, int danger_cm); // seuils globaux (cm)
void mvtsafe_setMaxScale(float s);                      // facteur max vitesse (0.0..1.0)

// Query
int mvtsafe_getState();     // MVTSAFE_STATE_*
bool mvtsafe_isActive();

#endif // MVT_SAFE_H
