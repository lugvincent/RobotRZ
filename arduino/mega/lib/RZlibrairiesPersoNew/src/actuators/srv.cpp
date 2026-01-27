/************************************************************
 * FICHIER : src/actuators/srv.cpp
 * ROLE    : Logique métier des servomoteurs (module Srv)
 * Chemin  : src/actuators/srv.cpp
 *
 * FONCTIONS :
 *   - srv_init()        → initialise les servos à leurs positions config
 *   - srv_setAngle()    → applique un angle à un servo
 *   - srv_setSpeed()    → modifie la vitesse d’un servo
 *   - srv_setActive()   → active/désactive un servo
 *   - srv_process()     → tick métier (appelle hardware update)
 *
 * NOTE :
 *   Ce module NE connaît pas les pins. Il ne gère que :
 *    → cfg_srv_angles[]
 *    → cfg_srv_speed[]
 *    → cfg_srv_active[]
 *
 *   Le bas-niveau est géré dans :
 *       src/hardware/srv_hardware.cpp
 ************************************************************/

#include "srv.h"
#include "config/config.h"
#include "communication/communication.h"
#include "hardware/srv_hardware.h"

/************************************************************
 * Initialisation du module (doit être appelé depuis setup())
 ************************************************************/
void srv_init()
{
    // Charge les valeurs déjà définies dans config.cpp :
    //   cfg_srv_angles[] = {SERVO_TGD_ZERO, SERVO_THB_ZERO, SERVO_BASE_ZERO}
    //   cfg_srv_speed[]  = {0, 0, 0}
    //   cfg_srv_active[] = {true, true, true}

    srv_hw_init();

    // Envoi VPIV initial pour que Node-RED sache la position et l’état
    for (int i = 0; i < SERVO_COUNT; i++)
    {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", cfg_srv_angles[i]);
        sendInfo("Srv", "angle", srvNames[i], buf);
    }
}

/************************************************************
 * Changer un angle (valide automatiquement cfg_* et hardware)
 ************************************************************/
void srv_setAngle(int index, int angle)
{
    if (index < 0 || index >= SERVO_COUNT)
        return;

    angle = constrain(angle, 0, 180);
    cfg_srv_angles[index] = angle;

    srv_hw_setTarget(index, angle);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", angle);
    sendInfo("Srv", "angle", srvNames[index], buf);
}

/************************************************************
 * Changer la vitesse d’un servo
 *   speed = 1 → rapide
 *   speed = 5 → moyen
 *   speed = 10 → très lent
 ************************************************************/
void srv_setSpeed(int index, int speed)
{
    if (index < 0 || index >= SERVO_COUNT)
        return;

    speed = max(1, speed);
    cfg_srv_speed[index] = speed;

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", speed);
    sendInfo("Srv", "speed", srvNames[index], buf);
}

/************************************************************
 * Activation / désactivation
 ************************************************************/
void srv_setActive(int index, bool on)
{
    if (index < 0 || index >= SERVO_COUNT)
        return;

    cfg_srv_active[index] = on;

    sendInfo("Srv", "act", srvNames[index], on ? "1" : "0");
}

/************************************************************
 * Tick métier (doit être appelé dans loop())
 ************************************************************/
void srv_process()
{
    // délègue 100 % au module hardware
    srv_hw_update();
}
