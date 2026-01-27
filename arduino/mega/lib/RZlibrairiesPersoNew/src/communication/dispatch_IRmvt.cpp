/************************************************************
 * FICHIER : src/communication/dispatch_IRmvt.cpp 
 * ROLE    : Dispatcher VPIV du capteur IR de mouvement
 ************************************************************/

#include "vpiv_dispatch.h"
#include "system/urg.h"
#include "sensors/mvt_ir.h"
#include <string.h>
#include <stdlib.h>
#include "communication/communication.h"

// Vérifier que l'instance est "*" (module unique)
static bool _ensure_instance_star(const char *inst)
{
    // instIsAll provient de vpiv_utils (si present) sinon simple test
    if (!inst)
        return false;
    if (inst[0] == '*' && inst[1] == '\0')
        return true;
    // accepte aussi "0" si tu veux, mais par convention MvtIR -> "*"
    return false;
}

namespace IRmvt
{

    bool dispatch(const char *prop, const char *inst, const char *value)
    {

        if (!_ensure_instance_star(inst))
        {
            sendError("MvtIR", "instance", inst ? inst : "*", "only '*' supported");
            return false;
        }

        if (strcmp(prop, "read") == 0)
        {
            mvt_ir_readInstant();
            sendInfo("MvtIR", "read", "*", "OK");
            return true;
        }
        if (strcmp(prop, "act") == 0)
        {
            bool on = (value && (strcmp(value, "1") == 0 || strcmp(value, "true") == 0));
            mvt_ir_setActive(on);
            // ack already sent inside mvt_ir_setActive (but we can repeat)
            return true;
        }
        if (strcmp(prop, "freq") == 0)
        {
            unsigned long ms = (unsigned long)atoi(value);
            mvt_ir_setFreqMs(ms);
            return true;
        }
        if (strcmp(prop, "thd") == 0)
        {
            int n = atoi(value);
            mvt_ir_setThreshold(n);
            return true;
        }
        if (strcmp(prop, "mode") == 0)
        {
            mvt_ir_setMode(value);
            return true;
        }

        // propriété non reconnue
        sendError("MvtIR", "prop", prop ? prop : "*", "unknown");
        return false;
    }

} // namespace MvtIRDispatch