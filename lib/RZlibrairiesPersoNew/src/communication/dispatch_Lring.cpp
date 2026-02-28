/************************************************************
 * FICHIER : src/communication/dispatch_Lring.cpp
 * ROLE    : Dispatcher VPIV pour LRING (anneau LED)
 * Propriétés supportées :
 *  - "expr" : expression interne (TOUJOURS globale)
 *  - "rgb"  : R,G,B (tous ou indices)
 *  - "clr"  : effacer (tous ou indices)
 *  - "int"  : intensité (tous ou indices)
 *  - "act"  : activation (tous ou indices)
 *  - "mode" : mode expr/extern
 *  - "init" : réinitialisation complète
 ************************************************************/

#include "vpiv_dispatch.h"
#include "system/urg.h"
#include "actuators/lring.h"
#include "hardware/lring_hardware.h"
#include "config/config.h"
#include "communication/vpiv_utils.h"
#include "communication/communication.h"

#include <string.h>
#include <stdlib.h>

namespace Lring
{
    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop || !inst)
            return false;

        // Parse instances (sauf pour expr qui est toujours global)
        int indices[LRING_NUM_PIXELS] = {0};
        int n = 0;
        bool useInstances = (strcmp(prop, "expr") != 0); // expr est toujours global

        if (useInstances)
        {
            n = parseIndexList(inst, indices, LRING_NUM_PIXELS);
            if (n < 0)
            {
                sendError("Lring", "parse", inst, "bad_inst");
                return false;
            }
        }

        // ----- act : activation/désactivation (par instance ou global) -----
        if (strcmp(prop, "act") == 0)
        {
            bool on = (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0);
            if (on)
            {
                lring_setExternMode(false);
                lring_applyExpression("voyant", 0);
            }
            else
            {
                lring_setExternMode(false);
                lring_setExpression(LRING_EXPR_NEUTRE);
                lring_applyExpression("neutre", 0);
            }
            sendInfo("Lring", "act", inst, on ? "1" : "0");
            return true;
        }

        // ----- expr : expression interne (TOUJOURS global) -----
        if (strcmp(prop, "expr") == 0)
        {
            lring_setExternMode(false);
            lring_applyExpression(value, 0);
            sendInfo("Lring", "expr", "*", value); // Toujours global
            return true;
        }

        // ----- rgb : couleur (par instance ou global) -----
        if (strcmp(prop, "rgb") == 0)
        {
            int rgb[3];
            if (!parseRGB(value, rgb))
            {
                sendError("Lring", "rgb", inst, "bad_rgb");
                return false;
            }

            lring_setExternMode(true);
            if (n == 0) // Tous
            {
                lringhw_fill(rgb[0], rgb[1], rgb[2]);
            }
            else // Indices spécifiques
            {
                for (int i = 0; i < n; i++)
                {
                    lringhw_setPixel(indices[i], rgb[0], rgb[1], rgb[2]);
                }
            }
            lringhw_show();
            sendInfo("Lring", "rgb", inst, value);
            return true;
        }

        // ----- clr : effacement (par instance ou global) -----
        if (strcmp(prop, "clr") == 0)
        {
            lring_setExternMode(true);
            if (n == 0) // Tous
            {
                lringhw_clear();
            }
            else // Indices spécifiques
            {
                for (int i = 0; i < n; i++)
                {
                    lringhw_setPixel(indices[i], 0, 0, 0);
                }
            }
            lringhw_show();
            sendInfo("Lring", "clr", inst, "OK");
            return true;
        }

        // ----- int : intensité (par instance ou global) -----
        if (strcmp(prop, "int") == 0)
        {
            int intensity = value ? atoi(value) : cfg_lring_brightness;
            if (n == 0) // Tous
            {
                lringhw_setBrightness(intensity);
            }
            else // Indices spécifiques
            {
                // Note: L'intensité est actuellement globale dans le hardware
                // TODO: Implémenter setPixelBrightness si nécessaire
                lringhw_setBrightness(intensity);
            }
            sendInfo("Lring", "int", inst, value);
            return true;
        }

        // ----- mode : expr/extern (toujours global) -----
        if (strcmp(prop, "mode") == 0)
        {
            if (!value)
                return false;

            if (strcasecmp(value, "expr") == 0)
            {
                lring_setExternMode(false);
            }
            else if (strcasecmp(value, "extern") == 0)
            {
                lring_setExternMode(true);
            }
            else
            {
                sendError("Lring", "mode", inst, "unknown");
                return false;
            }
            sendInfo("Lring", "mode", inst, value);
            return true;
        }

        // ----- init : réinitialisation (toujours global) -----
        if (strcmp(prop, "init") == 0)
        {
            lring_init();
            sendInfo("Lring", "init", "*", "OK");
            return true;
        }

        return false;
    }
} // namespace Lring
