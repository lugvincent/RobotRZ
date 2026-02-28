/************************************************************
 * FICHIER : src/communication/dispatch_FS.cpp
 * ROLE    : Dispatcher VPIV pour Force Sensor (FS)
 *
 * Propriétés supportées :
 *   - "read" :   lit valeur   ("value" ou "raw")
 *   - "tare" :   remise à zéro
 *   - "cal"  :   calibration (entier)
 *   - "act"  :   active/désactive (1/0)
 *   - "freq" :   fréquence ms du monitoring
 *   - "thd"  :   seuil d’alerte
 *
 * Instances :
 *   - "*" toujours (capteur unique)
 *
 * Exemples :
 *   $V:FS:*:read:value#
 *   $V:FS:*:read:raw#
 *   $V:FS:*:tare:1#
 *   $V:FS:*:cal:950#
 *   $V:FS:*:act:0#
 *   $V:FS:*:freq:100#
 *   $V:FS:*:thd:80#
 ************************************************************/

#include "vpiv_dispatch.h"
#include "system/urg.h"
#include "sensors/fs.h"
#include "config/config.h"
#include "communication/communication.h"
#include <string.h>
#include <stdlib.h>

namespace FS
{

    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop || !inst || !value)
            return false;

        // INSTANCE = capteur unique → "*" uniquement
        if (!(inst[0] == '*' && inst[1] == '\0'))
            return false;

        // -------- read --------
        if (strcmp(prop, "read") == 0)
        {
            // value = "value" ou "raw"
            fs_doRead(value);
            return true;
        }

        // -------- tare --------
        if (strcmp(prop, "tare") == 0)
        {
            fs_doTare();
            sendInfo("FS", "tare", "*", "OK");
            return true;
        }

        // -------- cal --------
        if (strcmp(prop, "cal") == 0)
        {
            long cal = atol(value);
            fs_doCal(cal);
            sendInfo("FS", "cal", "*", "OK");
            return true;
        }

        // -------- act --------
        if (strcmp(prop, "act") == 0)
        {
            bool on = (strcmp(value, "1") == 0 || strcmp(value, "true") == 0);
            fs_setActive(on);
            sendInfo("FS", "act", "*", on ? "1" : "0");
            return true;
        }

        // -------- freq --------
        if (strcmp(prop, "freq") == 0)
        {
            unsigned long ms = (unsigned long)atoi(value);
            fs_setFreq(ms);
            sendInfo("FS", "freq", "*", "OK");
            return true;
        }

        // -------- threshold --------
        if (strcmp(prop, "thd") == 0)
        {
            int th = atoi(value);
            fs_setThreshold(th);
            sendInfo("FS", "thd", "*", "OK");
            return true;
        }

        return false;
    }

} // namespace FSdispatch
