#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>  // Pour byte, uint8_t, etc.

// ===== Modes de fonctionnement =====
enum ModeRzB {
    ARRET = 0,
    VEILLE = 1,
    SURVEILLANCE = 2,
    AUTONOME = 3,
    SUIVEUR = 4,
    TELECOMMANDE = 5
};

// ===== Communication - Paramètres VPIV =====
#define MAX_PROPERTIES 10   // Nombre max de propriétés par message
#define MAX_INSTANCES  10   // Nombre max d'instances
#define MAX_VALUE_LEN  20   // Taille max des valeurs (caractères)

// ===== LEDs =====
#define RUBAN_LED_PIN    10
#define RING_LED_PIN      6
#define RUBAN_NUM_PIXELS 15
#define RING_NUM_PIXELS  12

#define DELAYVAL 50             // Temps d’affichage effets (ms)
#define INTENSITE_DEFAUT 50     // Intensité par défaut (0–255)

// ===== Servos =====
#define SERVO_TGD_PIN 4   // Tête Gauche/Droite
#define SERVO_THB_PIN 5   // Tête Haut/Bas
#define SERVO_BASE_PIN 9  // Base (corps)

#define CORRECTION_TGD 30
#define CORRECTION_THB 45
#define CORRECTION_BASE 20

// ===== Moteurs =====
#define MOTEURS_SERIAL Serial3  // Port série pour les moteurs (Uno)
#define VITESSE_DEFAUT 0
#define ACCELERATION_DEFAUT 0
#define DIRECTION_DEFAUT 0

// ===== Ultrasoniques =====
#define SONAR_NUM 9
#define MAX_DISTANCE 200  // cm
#define PING_INTERVAL 40  // ms

// Tableau des pins TRIG/ECHO
    //"AvG", "AvC", "AvD",
    //"ArD", "ArC", "ArG",
    //"StE", "StD", "StG"
const uint8_t SONAR_PINS[SONAR_NUM][2] = {
    {32, 33}, {34, 35}, {36, 37},
    {38, 39}, {40, 41}, {42, 43},
    {30, 31}, {28, 29}, {26, 27}
};

// Nom symbolique des sonars
extern const char* nomSonar[SONAR_NUM];

// Seuils de danger (modifiable via Node-RED)
extern unsigned int sonarDanger[SONAR_NUM];

// ===== Capteurs de force (HX711) =====
#define HX711_DOUT  A4   // Data
#define HX711_SCK   A5   // Clock

// ===== Détecteur de mouvement (IR) =====
#define MVT_IR_PIN 8

// ===== Microphones =====
#define MICRO_AV_PIN    A1
#define MICRO_GCHE_PIN  A2
#define MICRO_DRT_PIN   A3

// ===== Variables globales =====
extern ModeRzB modeRzB;   // état courant du robot

// ===== Fonctions =====
void initConfig();

#endif
