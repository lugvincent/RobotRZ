/* force_sensor.cpp
version calibrable
tester le capteur avec les valeurs brutes (rawReading)
calibrer avec un poids connu → envoyer le facteur depuis Node-RED
remettre à zéro sans recompiler ton code
*/
#include "config.h"
#include <HX711.h>
#include "communication.h"  // Pour MessageVPIV si tu l’utilises déjà

// Création de l’objet HX711
HX711 scale;

// Facteur d’échelle (modifiable via Node-RED)
float forceScaleFactor = 1.0;

// Initialisation du capteur de force
void initForceSensor() {
    scale.begin(HX711_DOUT, HX711_SCK);

    // Calibration par défaut
    scale.set_scale(forceScaleFactor);
    scale.tare();  // Mise à zéro au démarrage
}

// Vérification du capteur de force (lecture brute ou pondérée)
bool checkForceSensor() {
    if (scale.is_ready()) {
        long rawReading = scale.read();
        float weight = scale.get_units(1);  // Converti en unités calibrées

        Serial.print("$V:FS:U,0,R,");
        Serial.print(rawReading);
        Serial.print(",W,");
        Serial.print(weight);
        Serial.println("#");
        return true;
    } else {
        Serial.println("$E:FS:Force sensor not ready,,,0#");
        return false;
    }
}

// Traitement des messages VPIV pour configurer le capteur de force
void processForceSensorMessage(const MessageVPIV &msg) {
    if (msg.variable != 'F') return;  // 'F' pour Force Sensor

    for (int propIndex = 0; propIndex < msg.nbProperties; propIndex++) {
        char prop = msg.properties[propIndex];

        if (prop == 'C') {  
            // Calibration (facteur d’échelle)
            float newScale = atof(msg.values[propIndex][0]);  
            forceScaleFactor = newScale;
            scale.set_scale(forceScaleFactor);

            Serial.print("$I:FS:Calibration applied: ");
            Serial.print(forceScaleFactor);
            Serial.println("#");
        }
        else if (prop == 'T') {  
            // Tare (remise à zéro)
            scale.tare();
            Serial.println("$I:FS:Tare applied#");
        }
    }
}
