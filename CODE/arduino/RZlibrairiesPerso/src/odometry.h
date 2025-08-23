// odometry.h
#ifndef ODOMETRY_H
#define ODOMETRY_H

#include "config.h"
#include "communication.h"
#include <Encoder.h>

// Variables globales
extern Encoder knobLeft;
extern Encoder knobRight;
extern long comptG, comptD, oldComptG, oldComptD;
extern double xR, yR, orientation;
extern boolean odometrie;

// Prototypes des fonctions
void initOdometry();
void updateOdometry();
void processOdometryMessage(const MessageVPIV &msg);

#endif
