// =====================================================================
// INCLUDES GLOBAUX — toujours AVANT tout
// =====================================================================
#include <Arduino.h>

// Librairie perso (inclut automatiquement toutes tes sous-librairies)
#include <RZlibrairiesPersoNew.h>

// Sécurité
#include "safety/mvt_safe.h"

// Communication VPIV (réception, parsing, routage)
#include "communication/communication.h"

// Modules système
#include "config/config.h"
#include "system/urg.h"
#include "control/ctrl_L.h"

// Modules capteurs et actionneurs
#include "sensors/us.h"
#include "sensors/mic.h"
#include "sensors/fs.h"
#include "sensors/mvt_ir.h"

#include "actuators/mtr.h"
#include "actuators/lring.h"
#include "actuators/lrub.h"
#include "actuators/srv.h"

// Modules navigation
#include "navigation/Odom.h"

// =====================================================================
// VARIABLES GÉNÉRALES POUR LE TEMPO LOOP()
// =====================================================================
unsigned long loopStartTime = 0;
const unsigned long LOOP_MAX_MS = 80; // Objectif : loop < 80ms

// =====================================================================
// SETUP()
// =====================================================================
void setup()
{
    // -------------------------------
    // Ports série
    // -------------------------------
    Serial.begin(115200);
    while (!Serial)
    {
    }                             // sécurité sur MEGA (optionnel)
    MOTEURS_SERIAL.begin(115200); // Arduino UNO moteurs

    // -------------------------------
    // Init des modules
    // -------------------------------
    RZ_initAll(); // init toutes les librairies perso

    // -------------------------------
    // Message de diagnostic
    // -------------------------------
    Serial.println("$I:System:boot:*:OK#");
    Serial.flush();
}

// =====================================================================
// LOOP()
// =====================================================================
void loop()
{
    loopStartTime = millis(); // début mesure durée loop

    // -----------------------------------------------------------------
    // 1️⃣ Réception + parsing VPIV
    // -----------------------------------------------------------------
    communication_processInput(); // non bloquant

    // -----------------------------------------------------------------
    // 2️⃣ Tâches périodiques
    // -----------------------------------------------------------------
    lring_processPeriodic();
    mic_processPeriodic();
    us_processPeriodic();
    fs_processPeriodic();
    mvt_ir_processPeriodic();
    odom_processPeriodic();
    mvtsafe_process();

    // -----------------------------------------------------------------
    //Pilotage par Laisse - ctrl_L (si actif)
    // -----------------------------------------------------------------
    ctrl_L_update();
    
    // -----------------------------------------------------------------
    // 3️⃣ Gestion timers LED ring + ruban
    // -----------------------------------------------------------------
    lrub_processTimeout();

    // -----------------------------------------------------------------
    // 4️⃣ Autres tâches non périodiques
    // -----------------------------------------------------------------
    srv_process(); // PWM 50 Hz

    // -----------------------------------------------------------------
    // 5️⃣ Vérifications de sécurité
    // -----------------------------------------------------------------
    // mvtsafe_process() déjà appelé au 2️⃣ → surcharge éventuelle gérée
    // Pas besoin de rappeler, sauf si tu veux double-check

    // -----------------------------------------------------------------
    // 6️⃣ Diagnostic du temps d'exécution (loop watchdog)
    // -----------------------------------------------------------------
    unsigned long dt = millis() - loopStartTime;
    if (dt > LOOP_MAX_MS)
    {
        // Crée un buffer pour envoyer le temps en string
        char buf[16];
        snprintf(buf, sizeof(buf), "%lu", dt);

        // Envoi VPIV en cas de loop trop long
        //sendError("LoopTooSlow", "dt", "*", buf);
        urg_handle(URG_LOOP_TOO_SLOW);

    }

    // -----------------------------------------------------------------
    // 7️⃣ → Fin de loop()
    // Pas de delay() ici !
    // -----------------------------------------------------------------
}
