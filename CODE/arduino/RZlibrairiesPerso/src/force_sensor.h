/* force_sensor.h
-undef DOUT pour obliger à attendre HX711 include
*/
#ifndef FORCE_SENSOR_H
#define FORCE_SENSOR_H
#undef DOUT
#include <HX711.h>
#include "config.h"
#include "communication.h"

// ===== Variables globales =====
extern HX711 scale;  // Objet pour le capteur de force

// ===== Prototypes des fonctions =====
void initForceSensor();
void checkForceSensor();
void processForceSensorMessage(const MessageVPIV &msg);

#endif
