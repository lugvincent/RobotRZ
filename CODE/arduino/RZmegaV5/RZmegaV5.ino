/* ========================================================================================
   FICHIER : RZmegaV5.ino
   OBJET   : Point d’entrée principal Arduino MEGA pour l’architecture modulaire RZ-BOT
   AUTEUR  : Version structurée & commentée générée par ChatGPT

   -------------------------------------------------------------------
   🧩 Rôle du fichier .ino :
   - Initialiser les communications série
   - Initialiser tous les modules (config, moteurs, capteurs, sécurité, etc.)
   - Assurer une boucle loop() stable, courte et réactive
   - Collecter en continu les messages VPIV reçus et déléguer vers communication_processInput()
   - Appeler les fonctions périodiques (micro, ultrasons, IR, odométrie…)
   - Gérer les timers (extinction LEDs, clignotants…)
   - Ne pas contenir de logique fonctionnelle lourde
   -------------------------------------------------------------------

   🟢 Aucun code tablette Android n'est présent.
   🔄 Compatible Arduino IOT UART Gateway → MQTT → Node-RED
   🧱 Compatible avec ton arborescence /src/xxx
   ======================================================================================== */


// =====================================================================
// INCLUDES GLOBAUX — toujours AVANT tout
// =====================================================================
#include <Arduino.h>

// Librairie perso (inclut automatiquement toutes tes sous-librairies)
#include <RZlibrairiesPersoNew.h>

// Sécurité
#include "src/safety/mvt_safe.h"

// Communication VPIV (réception, parsing, routage)
#include "src/communication/communication.h"

// Modules système
#include "src/config/config.h"
#include "src/system/urg.h"

// Modules capteurs et actionneurs
#include "src/sensors/us.h"
#include "src/sensors/mic.h"
#include "src/sensors/fs.h"
#include "src/sensors/mvt_ir.h"

#include "src/actuators/mtr.h"
#include "src/actuators/lring.h"
#include "src/actuators/lrub.h"
#include "src/actuators/srv.h"

#include "src/navigation/Odom.h"


// =====================================================================
// VARIABLES GÉNÉRALES POUR LE TEMPO LOOP()
// =====================================================================

unsigned long loopStartTime = 0;
const unsigned long LOOP_MAX_MS = 80;  
// 🔍 Objectif : un loop < 80ms (réactif + MQTT stable)


// =====================================================================
// SETUP()
// =====================================================================
void setup() {

    // -------------------------------
    // Ports série
    // -------------------------------

    Serial.begin(115200);        // → liaison principale vers Smartphone SE (IOT UART gateway)
    while(!Serial){}             // sécurité sur MEGA (optionnel)

    MOTEURS_SERIAL.begin(115200);  // → Arduino UNO moteurs
    // Exemple : #define MOTEURS_SERIAL Serial3

    // -------------------------------
    // Init des modules - def dans RZlibrairiesPersoNew.ccp
    // -------------------------------
    RZ_initAll();

    // -------------------------------
    // Message de diagnostic
    // -------------------------------

    Serial.println("$I:System:boot:*:OK#");
    Serial.flush();
}


// =====================================================================
// LOOP()
// =====================================================================
void loop() {

    loopStartTime = millis();   // pour diagnostic vitesse

    // -----------------------------------------------------------------
    // 1️⃣ Réception + parsing + routage VPIV
    // -----------------------------------------------------------------
    communication_processInput();
    // ✓ totalement non bloquant
    // ✓ tout message $...# déclenche une dispatch vers le bon module

    // -----------------------------------------------------------------
    // 2️⃣ Tâches périodiques
    // -----------------------------------------------------------------
    lring_processPeriodic();      // gestion des évolutions cercle leds
    mic_processPeriodic();        // analyse micro direction/intensité
    us_processPeriodic();         // ping ultrason
    fs_processPeriodic();         // capteur force
    ir_processPeriodic();         // détecteur IR
    odom_processPeriodic();       // odométrie → mise à jour des X/Y/θ
    mvtsafe_process();            // superviseur sécurité - use us et uno / distance secu


    // -----------------------------------------------------------------
    // 3️⃣ Gestion timers LED ring + ruban
    // -----------------------------------------------------------------
    lrub_processTimeout();

    // -----------------------------------------------------------------
    // 3' Autres non périodique
    // -----------------------------------------------------------------
    srv_process();  // appelle srv_hw_update() → PWM 50 Hz


    // -----------------------------------------------------------------
    // 4️⃣ Vérifications de sécurité
    // -----------------------------------------------------------------
    checkEmergencyConditions();     // surcharge → appel sendEmergencyAlert si besoin

    // -----------------------------------------------------------------
    // 5️⃣ Diagnostic du temps d'exécution (loop watchdog)
    // -----------------------------------------------------------------
    unsigned long dt = millis() - loopStartTime;
    if (dt > LOOP_MAX_MS) {
        // on ne coupe rien, mais on signale
        sendError("LoopTooSlow", "*", String(dt).c_str());
    }

    // -----------------------------------------------------------------
    // 6️⃣ → Fin de loop()
    // Pas de delay() ici !
    // -----------------------------------------------------------------
}
