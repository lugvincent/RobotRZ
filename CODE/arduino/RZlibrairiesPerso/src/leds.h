// leds.h
#ifndef LEDS_H
#define LEDS_H

#include <Adafruit_NeoPixel.h>
#include "communication.h"

// Constantes pour les LEDs
#define RUBAN_NUM_PIXELS 15
#define RING_NUM_PIXELS  12
#define RUBAN_LED_PIN    10
#define RING_LED_PIN      6
#define DELAYVAL          50  // Temps d'affichage des effets (ms)

// Variables globales pour les LEDs
extern Adafruit_NeoPixel rubanPixels;
extern Adafruit_NeoPixel ringPixels;
extern bool activeRing;
extern bool activeRuban;
extern int redRC, greenRC, blueRC;      // Couleur du ring
extern int redRB, greenRB, blueRB;      // Couleur du ruban
extern unsigned long ringTpsFin;        // Timer pour le ring
extern unsigned long rubanTpsFin;       // Timer pour le ruban
extern int intensiteRing;               // Intensité du ring
extern int intensiteRuban;              // Intensité du ruban

// Prototypes des fonctions
void initLeds();
void processLedMessage(const MessageVPIV &msg);
void setRubanLedColor(int r, int g, int b);
void setRingColor(int r, int g, int b);
void ringSourire();
void ringAlerte();
void rubanLedFuite();

#endif
