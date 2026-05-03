/* communication.h
 Constantes ds config.h
 On utilise le type char plutôt que string
*/
#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <Arduino.h>
#include "config.h" 


// ====================
// Structure VPIV
// ====================
struct MessageVPIV {
    char type;   // I, F, V, E
    char variable; // ex : U (ultrason), D (LEDs), etc.
    char properties[MAX_PROPERTIES];   // Propriétés (D, C, etc.)
    char values[MAX_PROPERTIES][MAX_INSTANCES][MAX_VALUE_LEN]; // Valeurs associées
    uint8_t instances[MAX_INSTANCES];  // Instances concernées (0..n)
    int nbProperties;   // Nombre de propriétés présentes
    int nbInstances;    // Nombre d’instances présentes
    bool allInstances;  // true si le message utilise "*"
};

// ====================
// Variables globales
// ====================
extern MessageVPIV lastMessage;
extern bool newData;

// ====================
// Prototypes
// ====================
void recvSeWithStartEndMarkers();   // Réception série avec $...#
void parseVPIV(char *msg);          // Parsing du message VPIV
void SeTraitementNewV4();           // Traitement du dernier message
void sendAck(const char* message);  
void sendError(const char* errorMsg, const char* module = "", const char* details = "");

#endif
