/************************************************************
 * FICHIER : src/communication/dispatch_Ctrl_L.cpp
 * ROLE    : Dispatcher VPIV pour le module ctrl_L (pilotage laisse)
 ************************************************************/

#include "vpiv_dispatch.h"
#include "communication/communication.h"
#include "control/ctrl_L.h"
#include <string.h>
#include <stdlib.h>

namespace Ctrl_L
{
    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop)
            return false;

        // act : activation forcée (tests)
        if (strcmp(prop, "activT") == 0)
        {
            bool on = value && (strcmp(value, "1") == 0 || strcmp(value, "true") == 0);
            ctrl_L_setEnabled(on);
            sendInfo("ctrl_L", "activT", "*", on ? "OK" : "OFF");
            return true;
        }

        // dist : distance cible (cm)
        if (strcmp(prop, "dist") == 0 && value)
        {
            ctrl_L_setTargetDistance(atoi(value));
            sendInfo("ctrl_L", "dist", "*", "OK");
            return true;
        }

        /* kp : gain proportionnel - abandonné car géré directement par  le UNO dédié aux moteurs
        if (strcmp(prop, "kp") == 0 && value)
        {
            ctrl_L_setKp(atof(value));
            sendInfo("ctrl", "L", "*", "OK");
            return true;
        }*/

        // vmax : vitesse max
        if (strcmp(prop, "vmax") == 0 && value)
        {
            ctrl_L_setMaxSpeed(atoi(value));
            sendInfo("ctrl_L", "vmax", "*", "OK");
            return true;
        }

        // dead : zone morte
        if (strcmp(prop, "dead") == 0 && value)
        {
            ctrl_L_setDeadZone(atoi(value));
            sendInfo("ctrl_L", "dead", "*", "OK");
            return true;
        }

        // status
        if (strcmp(prop, "status") == 0)
        {
            const char *s = ctrl_L_isEnabled() ? "OK" : "OFF";
            sendInfo("ctrl_L", "status", "*", s);
            return true;
        }

        return false;
    }
}
