/************************************************************
 * FICHIER : src/communication/dispatch_Lring.cpp
 * ROLE    : Dispatcher VPIV du module Lring
 *
 * Propriétés :
 *   - expr : appliquer expression interne (MODE = EXPR)
 *   - rgb  : R,G,B (MODE = EXTERN)
 *   - clr  : clear ring (MODE = EXTERN)
 *   - int  : intensité
 *   - act  : 1/0 (ON=EXPR/voyant, OFF=OFF)
 *   - mode : expr / extern (force mode)
 *   - init : reset complet
 ************************************************************/

#include "vpiv_dispatch.h"
#include "../system/urg.h"
#include "../actuators/lring.h"
#include "../hardware/lring_hardware.h"
#include "../config/config.h"
#include "../communication/communication.h"
#include "../communication/vpiv_utils.h"

#include <string.h>
#include <stdlib.h>

namespace Lring
{

    /* ----------------------------------------------------------
     * DISPATCH PRINCIPAL
     * ---------------------------------------------------------- */
    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        (void)inst; // mono-instance

        if (!prop)
            return false;

        /* -----------------------------------------
         * act : activation / désactivation
         * ----------------------------------------- */
        if (strcmp(prop, "act") == 0)
        {
            bool on = (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0);

            if (on)
            {
                lring_setExternMode(false); // retour expr
                lring_applyExpression("voyant", 0);
                sendInfo("Lring", "act", "*", "1");
            }
            else
            {
                lring_setExternMode(false); // EXPR mode
                lring_setExpression(LRING_EXPR_NEUTRE);
                lring_applyExpression("neutre", 0);
                sendInfo("Lring", "act", "*", "0");
            }
            return true;
        }

        /* -----------------------------------------
         * expr : expression interne
         * => MODE = EXPR
         * ----------------------------------------- */
        if (strcmp(prop, "expr") == 0)
        {
            lring_setExternMode(false);
            lring_applyExpression(value, 0);
            sendInfo("Lring", "expr", "*", value);
            return true;
        }

        /* -----------------------------------------
         * rgb : changement de couleur
         * => MODE = EXTERN
         * ----------------------------------------- */
        if (strcmp(prop, "rgb") == 0)
        {
            int rgb[3];
            if (!parseRGB(value, rgb))
            {
                sendError("Lring", "rgb", "*", "bad_rgb");
                return false;
            }

            lring_setExternMode(true);
            lringhw_setBrightness(cfg_lring_brightness);
            lringhw_fill(rgb[0], rgb[1], rgb[2]);
            lringhw_show();
            sendInfo("Lring", "rgb", "*", value);
            return true;
        }

        /* -----------------------------------------
         * clr : effacer toutes les LEDs
         * MODE = EXTERN
         * ----------------------------------------- */
        if (strcmp(prop, "clr") == 0)
        {
            lring_setExternMode(true);
            lringhw_clear();
            lringhw_show();
            sendInfo("Lring", "clr", "*", "OK");
            return true;
        }

        /* -----------------------------------------
         * int : intensité
         * ----------------------------------------- */
        if (strcmp(prop, "int") == 0)
        {
            int lvl = atoi(value);
            if (lvl < 0)
                lvl = 0;
            if (lvl > 255)
                lvl = 255;
            cfg_lring_brightness = lvl;

            sendInfo("Lring", "int", "*", value);
            return true;
        }

        /* -----------------------------------------
         * mode : expr / extern 
         (prop optionnelle permet d'écraser la logique de 
         répartition EXPR ou EXTERN lié à la propriété)
         * ----------------------------------------- */
        if (strcmp(prop, "mode") == 0)
        {
            if (!value)
                return false;

            if (strcasecmp(value, "expr") == 0)
            {
                lring_setExternMode(false);
                sendInfo("Lring", "mode", "*", "expr");
                return true;
            }

            if (strcasecmp(value, "extern") == 0)
            {
                lring_setExternMode(true);
                sendInfo("Lring", "mode", "*", "extern");
                return true;
            }

            sendError("Lring", "mode", "*", "unknown");
            return false;
        }

        /* -----------------------------------------
         * init : réinitialisation complète
         * ----------------------------------------- */
        if (strcmp(prop, "init") == 0)
        {
            lring_init();
            sendInfo("Lring", "init", "*", "OK");
            return true;
        }

        return false;
    }

} // namespace LringDispatch
