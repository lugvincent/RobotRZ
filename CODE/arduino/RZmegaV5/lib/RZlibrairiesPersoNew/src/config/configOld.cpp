// ======================================================================
// FICHIER : src/config/configold.cpp
// Objet   : Définition des constantes centralisées et initialisation
// ======================================================================

#include "config.h"

// ======================================================================
//  TABLES PINS ULTRASON (EXACTES DE TON ANCIEN INO)
// ======================================================================

const uint8_t usTrigPins[US_NUM_SENSORS] = {
    32, 34, 36, 38, 40, 42, 30, 28, 26
};

const uint8_t usEchoPins[US_NUM_SENSORS] = {
    33, 35, 37, 39, 41, 43, 31, 29, 27
};

const char* usNames[US_NUM_SENSORS] = {
    "AvG","AvC","AvD",
    "ArD","ArC","ArG",
    "StE","StD","StG"
};

const unsigned int usDangerThresholds[US_NUM_SENSORS] = {
    15,10,15,
    15,10,15,
    20,20,20
};

// ======================================================================
//                FONCTION D’INITIALISATION GENERALE
// ======================================================================

void config_init() {

    // Serial
    MAIN_SERIAL.begin(MAIN_BAUD);

    // Moteurs (UNO)
    MOTEURS_SERIAL.begin(MOTEURS_BAUD);

    // Rien d’autre à initialiser ici pour l’instant, mais c’est ici
    // qu’on peut centraliser tous les “default states”.
}
