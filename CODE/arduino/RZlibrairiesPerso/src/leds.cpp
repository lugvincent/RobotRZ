// leds.cpp
#include "leds.h"
#include <Arduino.h>

// Initialisation des objets NeoPixel
Adafruit_NeoPixel rubanPixels(RUBAN_NUM_PIXELS, RUBAN_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ringPixels(RING_NUM_PIXELS, RING_LED_PIN, NEO_GRB + NEO_KHZ800);

// Variables globales
bool activeRing = false;
bool activeRuban = false;
int redRC = 0, greenRC = 0, blueRC = 0;
int redRB = 0, greenRB = 0, blueRB = 0;
unsigned long ringTpsFin = 0;
unsigned long rubanTpsFin = 0;
int intensiteRing = 50;
int intensiteRuban = 50;

// Initialisation des LEDs
void initLeds() {
    rubanPixels.begin();
    rubanPixels.clear();
    rubanPixels.show();
    ringPixels.begin();
    ringPixels.clear();
    ringPixels.show();
}

// Appliquer une couleur au ruban
void setRubanLedColor(int r, int g, int b) {
    redRB = r;
    greenRB = g;
    blueRB = b;
    rubanPixels.fill(rubanPixels.Color(r, g, b), 0, RUBAN_NUM_PIXELS);
    rubanPixels.setBrightness(intensiteRuban);
    rubanPixels.show();
    rubanTpsFin = millis() + 2000;
}

// Appliquer une couleur au ring
void setRingColor(int r, int g, int b) {
    redRC = r;
    greenRC = g;
    blueRC = b;
    ringPixels.fill(ringPixels.Color(r, g, b), 0, RING_NUM_PIXELS);
    ringPixels.setBrightness(intensiteRing);
    ringPixels.show();
    ringTpsFin = millis() + 2000;
}

// Effet "sourire" sur le ring
void ringSourire() {
    ringPixels.clear();
    ringPixels.fill(ringPixels.Color(155, 0, 0), 4, 5);  // LEDs 4 à 8 en rouge
    ringPixels.show();
    ringTpsFin = millis() + 2000;
}

// Effet "alerte" sur le ring
void ringAlerte() {
    for (int i = 0; i < RING_NUM_PIXELS; i += 2) {
        ringPixels.setPixelColor(i, ringPixels.Color(0, 255, 0));  // Vert
        ringPixels.setPixelColor(i + 1, ringPixels.Color(0, 0, 0));  // Éteint
    }
    ringPixels.show();
    ringTpsFin = millis() + 1000;
}

// Effet "fuite" sur le ruban
void rubanLedFuite() {
    rubanPixels.clear();
    for (int i = 0; i < RUBAN_NUM_PIXELS; i++) {
        rubanPixels.setPixelColor(i, rubanPixels.Color(255, 0, 0));  // Rouge
    }
    rubanPixels.show();
    rubanTpsFin = millis() + 2000;
}

// Traitement des messages pour les LEDs
void processLedMessage(const MessageVPIV &msg) {
    if (msg.variable == 'B') {  // Ruban LED
        for (int propIndex = 0; propIndex < msg.nbProperties; propIndex++) {
            char prop = msg.properties[propIndex];
            if (prop == 'C') {
                if (msg.allInstances) {
                    // Toutes les LEDs : $B:*:C:255,0,0#
                    int r, g, b;
                    sscanf(msg.values[propIndex][0], "%d,%d,%d", &r, &g, &b);
                    setRubanLedColor(r, g, b);
                } else {
                    // LEDs spécifiques : $B:[0,1,2]:C:[[255,0,0],[0,255,0],[0,0,255]]#
                    for (int instIndex = 0; instIndex < msg.nbInstances; instIndex++) {
                        int ledNum = msg.instances[instIndex];
                        int r, g, b;
                        sscanf(msg.values[propIndex][instIndex], "%d,%d,%d", &r, &g, &b);
                        rubanPixels.setPixelColor(ledNum, rubanPixels.Color(r, g, b));
                    }
                    rubanPixels.show();
                }
            } else if (prop == 'A') {
                // Activation : $B:*:A:1#
                bool state = (strcmp(msg.values[propIndex][0], "1") == 0);
                activeRuban = state;
                if (state) {
                    setRubanLedColor(redRB, greenRB, blueRB);  // Réutilise la couleur actuelle
                } else {
                    rubanPixels.clear();
                    rubanPixels.show();
                }
            }
        }
    } else if (msg.variable == 'R') {  // Ring LED
        for (int propIndex = 0; propIndex < msg.nbProperties; propIndex++) {
            char prop = msg.properties[propIndex];
            if (prop == 'C') {
                if (msg.allInstances) {
                    // Toutes les LEDs : $R:*:C:255,0,0#
                    int r, g, b;
                    sscanf(msg.values[propIndex][0], "%d,%d,%d", &r, &g, &b);
                    setRingColor(r, g, b);
                } else {
                    // LEDs spécifiques : $R:[0,1,2]:C:[[255,0,0],[0,255,0],[0,0,255]]#
                    for (int instIndex = 0; instIndex < msg.nbInstances; instIndex++) {
                        int ledNum = msg.instances[instIndex];
                        int r, g, b;
                        sscanf(msg.values[propIndex][instIndex], "%d,%d,%d", &r, &g, &b);
                        ringPixels.setPixelColor(ledNum, ringPixels.Color(r, g, b));
                    }
                    ringPixels.show();
                }
            } else if (prop == 'A') {
                // Activation : $R:*:A:1#
                bool state = (strcmp(msg.values[propIndex][0], "1") == 0);
                activeRing = state;
                if (state) {
                    setRingColor(redRC, greenRC, blueRC);  // Réutilise la couleur actuelle
                } else {
                    ringPixels.clear();
                    ringPixels.show();
                }
            } else if (prop == 'E') {
                // Effets spéciaux : $R:*:E:sourire#
                if (strcmp(msg.values[propIndex][0], "sourire") == 0) {
                    ringSourire();
                } else if (strcmp(msg.values[propIndex][0], "alerte") == 0) {
                    ringAlerte();
                }
            }
        }
    }
}
