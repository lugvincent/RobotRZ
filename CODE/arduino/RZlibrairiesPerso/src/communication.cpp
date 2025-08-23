/*communication.cpp
-j'ai gardé un communication old, Les anciens champs supprimés (instance, propriete, valeur) → remplacés par
 les tableaux instances[], properties[], values[][]
 - Gestion du cas * (toutes les instances → lastMessage.allInstances = true).
 - limites avec MAX_PROPERTIES, MAX_INSTANCES, MAX_VALUE_LEN dans config.h
 - on renvoie les erreurs de taille de buffer etc ... vers emergency.* pour centraliser les erreurs
 - On utilise le type char plutôt que string pour la vitesse et éviter la fragmentation de la mémoire
 - suit la structure Node-Red du programme
 - inclus un exemple
 */

#include "communication.h"
#include "config.h"
#include <string.h>  // Pour strncpy, strcmp,... utile pour char
#include "emergency.h"  // Ajout pour gérer les erreurs critiques
// ====================
// Variables globales
// ====================
MessageVPIV lastMessage;  // Dernier message reçu
bool newData = false;     // Flag pour nouveau message

// ====================
// Réception des messages série (SE)
// ====================
void recvSeWithStartEndMarkers() {
    static char receivedChars[120];
    static size_t idx = 0;
    static bool recvInProgress = false;

    while (Serial.available() > 0) {
        char rc = Serial.read();
        if (rc == '$') {
            recvInProgress = true;
            idx = 0;
            memset(receivedChars, 0, sizeof(receivedChars));
        }
        else if (rc == '#') {
            receivedChars[idx] = '\0';
            recvInProgress = false;
            parseVPIV(receivedChars);
            newData = true;
        }
        else if (recvInProgress) {
            if (idx >= sizeof(receivedChars) - 1) {
                handleEmergency(EMERGENCY_BUFFER_OVERFLOW);  // Débordement détecté
                return;
            }
            receivedChars[idx++] = rc;
        }
    }
}

// ====================
// Parsing du message VPIV
// Format : $<type>:<contenu>:<variable>,<instances>,<propriétés>,<valeurs>#
// ====================
void parseVPIV(char *msg) {
    char *token;
    int propIndex = -1;
    int instIndex = 0;

    // Réinitialisation de la structure
    memset(&lastMessage, 0, sizeof(lastMessage));
    lastMessage.nbProperties = 0;
    lastMessage.nbInstances = 0;
    lastMessage.allInstances = false;

    // Découpage du message avec strtok
    token = strtok(msg, ":,");
    if (token != NULL) lastMessage.type = token[0];  // Type de message (V, I, F, E)

    token = strtok(NULL, ":,");  // Contenu (ex: "US") → ignoré pour l'instant
    token = strtok(NULL, ":,");
    if (token != NULL) lastMessage.variable = token[0];  // Variable (ex: U)

    // Lecture des instances, propriétés et valeurs
    while ((token = strtok(NULL, ",#")) != NULL) {
        if (strcmp(token, "*") == 0) {
            // Cas spécial : * = toutes les instances
            lastMessage.allInstances = true;
        }
        else if (isdigit(token[0])) {
            // Instance numérique
            if (lastMessage.nbInstances < MAX_INSTANCES) {
                lastMessage.instances[lastMessage.nbInstances++] = atoi(token);
            }
        }
        else if (isalpha(token[0]) && strlen(token) == 1) {
            // Nouvelle propriété
            if (lastMessage.nbProperties < MAX_PROPERTIES) {
                propIndex = lastMessage.nbProperties++;
                lastMessage.properties[propIndex] = token[0];
                instIndex = 0;  // Réinitialisation de l'index d'instance
            }
        }
        else {
            // Valeur associée à la dernière propriété
            if (propIndex >= 0 && propIndex < MAX_PROPERTIES &&
                instIndex < MAX_INSTANCES) {
                strncpy(lastMessage.values[propIndex][instIndex],
                        token, MAX_VALUE_LEN - 1);
                lastMessage.values[propIndex][instIndex][MAX_VALUE_LEN - 1] = '\0';
                instIndex++;
            }
        }
    }
}

// ====================
// Traitement du message reçu - zz ici exemple couleur
// ====================
void SeTraitementNewV4() {
    if (!newData) return;

    if (lastMessage.variable == 'D') {
        for (int p = 0; p < lastMessage.nbProperties; p++) {
            if (lastMessage.properties[p] == 'C') {
                int r, g, b;
                // Vérification du retour de sscanf
                if (sscanf(lastMessage.values[p][0], "%d,%d,%d", &r, &g, &b) != 3) {
                    handleEmergency(EMERGENCY_SENSOR_FAIL);  // Format invalide
                    sendError("Format couleur invalide", "LED", lastMessage.values[p][0]);
                    return;
                }
                // Traiter r, g, b
            }
        }
    }
    newData = false;
}

// ================================
// Envoi d’un accusé de réception
// ================================
// Dans communication.cpp
void sendAck(const char* message) {
    Serial.print("$I:ACK:");
    Serial.print(message);  // Serial.print gère les char*
    Serial.println("#");
}

// ================================
// Envoi d’un message d’erreur
// ================================
void sendError(const char* errorMsg, const char* module, const char* details) {
    Serial.print("$E:");
    Serial.print(errorMsg);
    Serial.print(":");
    Serial.print(module);
    Serial.print(",,,");
    Serial.print(details);
    Serial.println("#");
}
