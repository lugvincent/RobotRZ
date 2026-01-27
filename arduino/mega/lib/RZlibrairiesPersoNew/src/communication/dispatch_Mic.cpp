/************************************************************
 * FICHIER : src/communication/dispatch_Mic.cpp
 * ROLE    : Dispatcher VPIV pour Microphones (Mic)
 *
 * Propriétés supportées :
 *  - "read" : read type -> value can be "raw","rms","peak","orient"
 *             ex: $V:Mic:*:read:rms#
 *  - "thd"  : threshold en unités analogiques (ex: 600)
 *  - "freq" : intervalle périodique en ms
 *  - "win"  : fenêtre d'échantillonnage en ms
 *  - "act"  : active (1)/desactive(0)
 *  - "mode" : "idle"|"monitor"|"orient"|"surveillance" (stocké côté Mega si besoin)
 *
 * Instances :
 *  - "*" (all), "0"/"1"/"2", "[a,b]"
 *
 ************************************************************/

#include "vpiv_dispatch.h"
#include "system/urg.h"
#include "sensors/mic.h"
#include "config/config.h"
#include "communication/communication.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Parse inst list ("*", "0", "[0,2]")
static int parseInstanceList(const char *inst, int *out, int max)
{
    if (!inst || !out)
        return -1;

    // "*" = all
    if (inst[0] == '*' && inst[1] == '\0')
        return 0;

    // [0,1,2]
    if (inst[0] == '[')
    {
        char buf[64];
        strncpy(buf, inst + 1, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        char *end = strchr(buf, ']');
        if (end)
            *end = '\0';

        int n = 0;
        char *tok = strtok(buf, ",");
        while (tok && n < max)
        {
            while (*tok == ' ')
                tok++;
            out[n++] = atoi(tok);
            tok = strtok(NULL, ",");
        }
        return n;
    }

    // numeric single
    for (const char *p = inst; *p; p++)
        if (!isdigit((unsigned char)*p))
            return -1;

    out[0] = atoi(inst);
    return 1;
}

namespace Mic
{

    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop || !inst || !value)
            return false;

        int indices[MIC_COUNT];
        int n = parseInstanceList(inst, indices, MIC_COUNT);
        if (n < 0)
            return false;

        // read
        if (strcmp(prop, "read") == 0)
        {
            mic_handleReadList(indices, n, value);
            return true;
        }

        // thd
        if (strcmp(prop, "thd") == 0)
        {
            int th = atoi(value);
            mic_setThreshold(indices, n, th);
            sendInfo("Mic", "thd", inst, "OK");
            return true;
        }

        // freq
        if (strcmp(prop, "freq") == 0)
        {
            mic_setFreq((unsigned long)atoi(value));
            sendInfo("Mic", "freq", inst, "OK");
            return true;
        }

        // win
        if (strcmp(prop, "win") == 0)
        {
            mic_setWin((unsigned long)atoi(value));
            sendInfo("Mic", "win", inst, "OK");
            return true;
        }

        // act
        if (strcmp(prop, "act") == 0)
        {
            bool on = (strcmp(value, "1") == 0 || strcmp(value, "true") == 0);
            mic_setActive(on);
            sendInfo("Mic", "act", inst, "OK");
            return true;
        }

        // mode
        if (strcmp(prop, "mode") == 0)
        {
            if (strcmp(value, "idle") == 0)
            {
                cfg_mic_active = false;
            }
            else if (strcmp(value, "monitor") == 0)
            {
                cfg_mic_active = true;
                cfg_mic_freq_ms = MIC_FREQ_MS_DEFAULT;
            }
            else if (strcmp(value, "orient") == 0)
            {
                cfg_mic_active = true;
                cfg_mic_freq_ms = 100;
            }
            else if (strcmp(value, "surveillance") == 0)
            {
                cfg_mic_active = true;
                cfg_mic_freq_ms = 50;
            }
            sendInfo("Mic", "mode", inst, "OK");
            return true;
        }

        return false;
    }

} // namespace MicDispatch