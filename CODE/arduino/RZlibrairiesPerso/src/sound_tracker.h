// sound_tracker.h
#ifndef SOUND_TRACKER_H
#define SOUND_TRACKER_H

#include "config.h"
#include "communication.h"

// Variables globales
extern int lastSoundDirection;  // Dernière direction détectée (0: avant, 1: gauche, 2: droit)

// Prototypes des fonctions
void initSoundTracker();
void updateSoundDirection(int av, int gche, int drt);
void processSoundTrackerMessage(const MessageVPIV &msg);

#endif
