#pragma once

// Point d’entrée unique de la librairie RZ
// Elle n’expose QUE les modules nécessaires au .ino

#include "communication/communication.h"
#include "configuration/config.h"
#include "system/urg.h"

#include "sensors/us.h"
#include "sensors/mic.h"
#include "sensors/fs.h"
#include "sensors/mvt_ir.h"

#include "actuators/mtr.h"
#include "actuators/lring.h"
#include "actuators/lrub.h"
#include "actuators/srv.h"

#include "navigation/Odom.h"
#include "safety/mvt_safe.h"

// Initialise tous les modules (optionnel)
void RZ_initAll();
