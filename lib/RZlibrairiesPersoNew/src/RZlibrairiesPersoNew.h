//  RZlibrairiesPersoNew.h
// role : point d’entrée unique de la librairie RZ,
// inclut tous les modules nécessaires au .ino et expose
// une fonction d’init globale
// version : 1.00
// date : 2024-06-01
// auteur : vincent Philippe
// remarques :

#pragma once

// Point d’entrée unique de la librairie RZ
// Elle n’expose QUE les modules nécessaires au .ino

#include "communication/communication.h"
#include "config/config.h"
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
#include "control/ctrl_L.h"

// Initialise tous les modules (optionnel)
void RZ_initAll();
