/************************************************************
 * FICHIER : src/communication/dispatch_Srv.cpp
 * ROLE    : Dispatcher VPIV pour les servomoteurs (Srv)
 *
 * Propriétés supportées :
 *   - angle   : définit l’angle (0–180)
 *   - speed   : définit la vitesse de déplacement
 *   - act     : active/désactive un servo
 *   - read    : renvoie angle / vitesse / état
 ************************************************************/

#include "vpiv_dispatch.h"
#include "system/urg.h"
#include "actuators/srv.h"
#include "config/config.h"
#include "communication/communication.h"
#include <string.h>
#include <stdlib.h>

namespace Srv
{

    static int resolveIndex(const char *inst)
    {
        if (!inst)
            return -1;

        // instance texte → index dans srvNames[]
        for (int i = 0; i < SERVO_COUNT; i++)
        {
            if (strcmp(inst, srvNames[i]) == 0)
                return i;
        }

        // instance "*" = s’applique à tous
        if (strcmp(inst, "*") == 0)
            return -2; // code interne = all servos

        return -1; // inconnu
    }

    /************************************************************
     * DISPATCH PRINCIPAL
     ************************************************************/
    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop || !inst)
            return false;

        int idx = resolveIndex(inst);
        bool applyAll = (idx == -2);

        // ------------ angle ------------
        if (strcmp(prop, "angle") == 0)
        {
            if (!value)
                return false; // ✅ protection

            int a = atoi(value);

            if (applyAll)
            {
                for (int i = 0; i < SERVO_COUNT; i++)
                    srv_setAngle(i, a);
            }
            else if (idx >= 0)
            {
                srv_setAngle(idx, a);
            }
            return true;
        }

        // ------------ speed ------------
        if (strcmp(prop, "speed") == 0)
        {
            if (!value)
                return false; // ✅ protection
            int s = atoi(value);
            if (applyAll)
            {
                for (int i = 0; i < SERVO_COUNT; i++)
                    srv_setSpeed(i, s);
            }
            else if (idx >= 0)
            {
                srv_setSpeed(idx, s);
            }
            return true;
        }

        // ------------ act (activation servo) ------------
        if (strcmp(prop, "act") == 0)
        {
            if (!value)
                return false; // ✅ protection
            bool on = (strcmp(value, "1") == 0 || strcmp(value, "true") == 0);
            if (applyAll)
            {
                for (int i = 0; i < SERVO_COUNT; i++)
                    srv_setActive(i, on);
            }
            else if (idx >= 0)
                if (!value)
                    return false; // ✅ protection

            {
                srv_setActive(idx, on);
            }
            return true;
        }

        // ------------ read (renvoie valeurs) ------------
        if (strcmp(prop, "read") == 0)
        {
            if (applyAll)
            {
                for (int i = 0; i < SERVO_COUNT; i++)
                {
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%d", cfg_srv_angles[i]);
                    sendInfo("Srv", "angle", srvNames[i], buf);
                }
            }
            else if (idx >= 0)
            {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d", cfg_srv_angles[idx]);
                sendInfo("Srv", "angle", srvNames[idx], buf);
            }
            return true;
        }

        return false;
    }

} // namespace Srv
