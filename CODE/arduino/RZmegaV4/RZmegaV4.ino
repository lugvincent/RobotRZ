/* === INCLUSIONS ET DÉCLARATIONS DE BIBLIOTHÈQUES AA  voir ds fichier .h === */

/*
Info : Mon fichier RZmegaV4.ino est le fichier principal à la réception d'un message dans la boucle
 mes données au format "VPIV" sont lus. la séquence d'include est de type :
RZlibrairiesPerso.h>config.h (récup des constantes) > communication.h (lecture VPIV) >
traitement du message en fonction de la variable servo.h , led.h,... dans communication.cpp le switch
qui traite en fonction de la variable et gère les retour d'infos et d'erreur.
format des messages échangés : $<type>:<contenu>:<variable>,<instances>,<propriétés>,<valeurs>#

*/

// bibliothèque perso : RZlibairiePerso
#include <RZlibrairiesPerso.h>

/*
#include "config.h"
#include "emergency.h"
#include "force_sensor.h"
#include "communication.h"
#include "leds.h"
#include "servos.h"
#include "moteurs.h"
#include "ultrasonic.h"
#include "microphone_array.h"
#include "sound_tracker.h"
#include "odometry.h"
#include "ir_motion.h"
*/

// Variables globales pour la gestion du temps
unsigned long loopStartTime = 0;
unsigned long loopMaxTime = 100;  // Temps max autorisé pour loop() en ms

void setup() {

    // Initialisation de la communication série principale (pour Node-RED)
    Serial.begin(115200);

    // Initialisation de la communication série avec l'Arduino Uno (moteurs)
    MOTEURS_SERIAL.begin(115200);  // Port série pour les moteurs (Uno)

    //initialisation des modules
    initConfig();
    initLeds();
    initServos();
    initMoteurs();
    initUltrasonic();
    initEmergency();
    initForceSensor();
    initMicrophoneArray();
    initSoundTracker();
    initOdometry();
    initIRMotion();
}

void loop() {
    loopStartTime = millis();

    // Gestion des timers pour éteindre les LEDs après un délai 
    if (millis() > ringTpsFin && ringTpsFin != 0) {
        ringPixels.clear();
        ringPixels.show();
        ringTpsFin = 0;
    }
    if (millis() > rubanTpsFin && rubanTpsFin != 0) {
        rubanPixels.clear();
        rubanPixels.show();
        rubanTpsFin = 0;
    }

    // Réception des messages série ( de SE :node-red...) 
    recvSeWithStartEndMarkers();

    //Traitement des modules
    microTraitement();  // Microphones
    updateOdometry();   // Odométrie
    checkIRMotion();     // Détecteur IR

    // Mesure des distances ultrasoniques
    fSonarSe();

    // Vérification des urgences et capteurs
    checkEmergencyConditions();
    checkForceSensor();  

    // Vérifier le temps d'exécution de loop()
    unsigned long loopDuration = millis() - loopStartTime;
    if (loopDuration > loopMaxTime) {
        sendError("Dépassement loop", "", String(loopDuration).c_str());
    }
}
