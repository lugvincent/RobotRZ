/************************************************************
 * FICHIER : src/communication/dispatch_SecMvt.cpp
 * ROLE    : Dispatcher VPIV pour la sécurité mouvement
 *          - configure mvt_safe
 *          - peut déclencher des urgences
 ************************************************************/

#include "vpiv_dispatch.h"
#include "communication/communication.h"
#include "safety/mvt_safe.h"
#include "system/urg.h"
#include <string.h>
#include <stdlib.h>

namespace SecMvt
{

    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop)
            return false;

        // --------------------------------------------------
        // act : activation / désactivation
        // --------------------------------------------------
        if (strcmp(prop, "act") == 0)
        {
            bool on = (value && (strcmp(value, "1") == 0 || strcmp(value, "true") == 0));
            mvtsafe_setActive(on);
            sendInfo("SecMvt", "act", "*", on ? "1" : "0");
            return true;
        }

        // --------------------------------------------------
        // mode : soft / hard / idle
        // --------------------------------------------------
        if (strcmp(prop, "mode") == 0)
        {
            mvtsafe_setMode(value);
            sendInfo("SecMvt", "mode", "*", value);
            return true;
        }

        // --------------------------------------------------
        // seuils warn / danger
        // --------------------------------------------------
        if (strcmp(prop, "thd") == 0)
        {
            int w = 0, d = 0;
            sscanf(value, "%d,%d", &w, &d);
            mvtsafe_setThresholds(w, d);
            sendInfo("SecMvt", "thd", "*", value);
            return true;
        }

        // --------------------------------------------------
        // scale vitesse max
        // --------------------------------------------------
        if (strcmp(prop, "scale") == 0)
        {
            float s = atof(value);
            mvtsafe_setMaxScale(s);
            sendInfo("SecMvt", "scale", "*", value);
            return true;
        }

        // --------------------------------------------------
        // status
        // --------------------------------------------------
        if (strcmp(prop, "status") == 0)
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", mvtsafe_getState());
            sendInfo("SecMvt", "status", "*", buf);
            return true;
        }

        return false;
    }

} // namespace SecMvt
