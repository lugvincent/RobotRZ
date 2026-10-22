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

        // activation (remplace activT)
        if (strcmp(prop, "act") == 0)
        {
            bool on = value && (strcmp(value, "1") == 0 || strcmp(value, "true") == 0);
            ctrl_L_setEnabled(on);
            // 22/10/2026 — renvoie l'ÉTAT OBTENU, plus un simple "OK".
            // Même motif que les 9 autres dispatcheurs, et conforme à la
            // Table A : Ctrl_L.act est TYPE=boolean, FORMAT_VALUE=0|1.
            // ⚠ La copie de Table A embarquée dans node-red/config/varBrute.csv
            //   déclare encore « enum OK|OFF » pour cette ligne : c'est une
            //   version antérieure. Voir docs/convention_VPIV_ordre.md §5.5.
            sendInfo("Ctrl_L", "act", "*", on ? "1" : "0");
            return true;
        }

        // dist : distance cible (mm)
        if (strcmp(prop, "dist") == 0 && value)
        {
            ctrl_L_setTargetDistance(atoi(value));
            sendInfo("Ctrl_L", "dist", "*", "OK");
            return true;
        }

        // vmax : vitesse max
        if (strcmp(prop, "vmax") == 0 && value)
        {
            ctrl_L_setMaxSpeed(atoi(value));
            sendInfo("Ctrl_L", "vmax", "*", "OK");
            return true;
        }

        // dead : zone morte
        if (strcmp(prop, "dead") == 0 && value)
        {
            ctrl_L_setDeadZone(atoi(value));
            sendInfo("Ctrl_L", "dead", "*", "OK");
            return true;
        }

        // status : état actuel (OK/FceKO/VtKO)
        if (strcmp(prop, "status") == 0)
        {
            const char *status = ctrl_L_getStatus();   // On récupère la valeur retournée
            sendInfo("Ctrl_L", "status", "*", status); // state
            return true;
        }

        return false;
    }
}
