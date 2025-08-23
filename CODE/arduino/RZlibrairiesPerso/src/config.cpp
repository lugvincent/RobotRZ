/* config.cpp
Initialise modeRzB à ARRET (sécurité au démarrage).
Définit les noms des sonars (même ordre que dans SONAR_PINS).
Définit des seuils de danger par défaut 
Fournit une fonction initConfig() qui remet ces valeurs
 si besoin (ex. après un reset ou si tu veux recharger les paramètres par défaut)
*/
#include "config.h"
//#include <Arduino.h> //pour byte mais ne fonctionne pas
//#include <stdint.h>  // Pour uint8_t

// ===== Variables globales =====

// Robot démarré à l’arrêt
ModeRzB modeRzB = ARRET;

// Noms des 9 sonars (doivent correspondre à l’ordre dans SONAR_PINS)
const char* nomSonar[SONAR_NUM] = {
    "AvG", "AvC", "AvD",
    "ArD", "ArC", "ArG",
    "StE", "StD", "StG"
};

// Seuils de danger par défaut (cm)
// ⚠️ Ces valeurs pourront être modifiées à l’exécution via Node-RED
unsigned int sonarDanger[SONAR_NUM] = {
    20, 20, 20,   // Avant (G, C, D)
    20, 20, 20,   // Arrière (D, C, G)
    15, 15, 15    // Stabilisateurs (E, D, G)
};

// ===== Initialisation de la configuration =====
void initConfig() {
    modeRzB = ARRET;  // Sécurité : démarrage robot inactif

    // Réinitialiser les seuils si besoin
    for (uint8_t i = 0; i < SONAR_NUM; i++) {
        // Exemple : réappliquer les valeurs ci-dessus
        sonarDanger[i] = (i < 6) ? 20 : 15;
    }
}
