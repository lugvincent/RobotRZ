// ir_motion.h
#ifndef IR_MOTION_H
#define IR_MOTION_H

#include "config.h"
#include "communication.h"

// Variables globales
extern boolean detecteurMvt;
extern int mvtIrVal;

// Prototypes des fonctions
void initIRMotion();
void checkIRMotion();
void processIRMotionMessage(const MessageVPIV &msg);

#endif
