// =====================================================================
// FICHIER  : src/main.cpp
// CHEMIN   : arduino/mega/src/main.cpp
// VERSION  : 1.2  —  Mars 2026
// AUTEUR   : Vincent Philippe
//
// RÔLE
// ----
//   Point d'entrée du firmware Arduino Mega.
//   Initialise tous les modules via RZ_initAll() et appelle
//   les tâches périodiques à chaque tour de loop().
//
// ORDRE DE SETUP
// --------------
//   RZ_initAll() initialise tous les modules dans le bon ordre :
//   config → comm → urg → actionneurs → capteurs → navigation → sécurité
//   RZ_initAll() émet lui-même $I:System:boot:*:OK# via sendInfo()
//
//   ⚠ initConfig() (appelé en 1er par RZ_initAll()) ouvre Serial
//   et MOTEURS_SERIAL. NE PAS ouvrir les ports série dans setup() :
//   cela créerait une double initialisation.
//
// ORDRE DE LOOP (non bloquant)
// ----------------------------
//   1. Réception + parsing VPIV    (communication_processInput)
//   2. Tâches périodiques capteurs (lring, mic, us, fs, mvt_ir, odom)
//   3. Sécurité anti-collision     (mvtsafe_process)
//   4. Pilotage par laisse         (ctrl_L_update)
//   5. LED ruban timeout           (lrub_processTimeout)
//   6. Servo PWM                   (srv_process)
//   7. Moteurs keepalive UNO       (mtr_processPeriodic)
//   8. Watchdog loop trop lente    (urg_handle si > LOOP_MAX_MS)
//
// CORRECTIONS v1.2
// ----------------
//   - #include "navigation/odom.h" (minuscule) — casse correcte Linux
//   - Suppression de Serial.begin() en double dans setup()
//
// PRÉCAUTIONS
// -----------
//   - Ne jamais appeler delay() dans loop()
//   - LOOP_MAX_MS = 80ms : si dépassé → urgence URG_LOOP_TOO_SLOW_A
//   - mtr_processPeriodic() DOIT être dans loop() :
//     le keepalive 100ms vers l'UNO évite son timeout safety (1000ms)
// =====================================================================

#include <Arduino.h>
#include <RZlibrairiesPersoNew.h>

#include "safety/mvt_safe.h"
#include "communication/communication.h"
#include "config/config.h"
#include "system/urg.h"
#include "control/ctrl_L.h"

#include "sensors/us.h"
#include "sensors/mic.h"
#include "sensors/fs.h"
#include "sensors/mvt_ir.h"

#include "actuators/mtr.h"
#include "actuators/lring.h"
#include "actuators/lrub.h"
#include "actuators/srv.h"

#include "navigation/odom.h"

// =====================================================================
// TEMPORISATION LOOP
// =====================================================================
static unsigned long loopStartTime = 0;
static const unsigned long LOOP_MAX_MS = 80UL; // Objectif : loop < 80ms

// =====================================================================
// SETUP
// =====================================================================
void setup()
{
    // Initialisation de tous les modules (config → comm → capteurs → navigation…)
    // initConfig() — appelé en premier par RZ_initAll() — ouvre Serial et MOTEURS_SERIAL.
    // ⚠ Ne PAS appeler Serial.begin() ici : initConfig() le fait déjà (évite double init).
    // RZ_initAll() émet $I:System:boot:*:OK# via sendInfo() en fin d'init.
    RZ_initAll();
}

// =====================================================================
// LOOP
// =====================================================================
void loop()
{
    loopStartTime = millis();

    // -----------------------------------------------------------------
    // 1. Réception + parsing VPIV (non bloquant)
    // -----------------------------------------------------------------
    communication_processInput();

    // -----------------------------------------------------------------
    // 2. Tâches périodiques capteurs / actionneurs
    // -----------------------------------------------------------------
    lring_processPeriodic();
    mic_processPeriodic();
    us_processPeriodic();
    fs_processPeriodic();
    mvt_ir_processPeriodic();
    odom_processPeriodic();

    // -----------------------------------------------------------------
    // 3. Sécurité anti-collision
    // -----------------------------------------------------------------
    mvtsafe_process();

    // -----------------------------------------------------------------
    // 4. Pilotage par laisse (si actif, typePtge 3 ou 4)
    // -----------------------------------------------------------------
    ctrl_L_update();

    // -----------------------------------------------------------------
    // 5. LED ruban — gestion extinction automatique (timeout)
    // -----------------------------------------------------------------
    lrub_processTimeout();

    // -----------------------------------------------------------------
    // 6. Servo — PWM 50 Hz
    // -----------------------------------------------------------------
    srv_process();

    // -----------------------------------------------------------------
    // 7. Moteurs — keepalive vers UNO (toutes les 100ms)
    //    OBLIGATOIRE : le UNO a un timeout safety de 1000ms.
    //    Sans cet appel, l'UNO freine progressivement vers 0.
    // -----------------------------------------------------------------
    mtr_processPeriodic();

    // -----------------------------------------------------------------
    // 8. Watchdog — durée de loop
    //    Si la loop dépasse LOOP_MAX_MS → urgence URG_LOOP_TOO_SLOW_A
    // -----------------------------------------------------------------
    unsigned long dt = millis() - loopStartTime;
    if (dt > LOOP_MAX_MS)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%lu", dt);
        urg_handle(URG_LOOP_TOO_SLOW_A); // déclenche urgence + arrêt moteurs
    }
}