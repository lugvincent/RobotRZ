/************************************************************
 * FICHIER : src/communication/dispatch_Lrub.cpp
 * ROLE    : Dispatcher VPIV pour LRUB (ruban LED) - SIMPLE
 *
 * Propriétés supportées :
 *  - "col" : "R,G,B"    -> applique couleur (tous)
 *  - "int" : intensité 0..255
 *  - "act" : 1/0 ou true/false
 *  - "f"   : fuite (1/0)
 *  - "init": réinitialiser le module
 *
 * Instance :
 *  - "*" => tous (par défaut)
 *  - "N" => indice unique
 *  - "[a,b]" => liste d'indices
 *
 * Usage : $V:Lrub:<prop>:<inst>:<value>#
 ************************************************************/

#include "vpiv_dispatch.h"
#include "../system/urg.h"
#include "../actuators/lrub.h"
#include "../hardware/lrub_hardware.h"
#include "../configuration/config.h"
#include "../communication/vpiv_utils.h"
#include "../communication/communication.h"

#include <string.h>
#include <stdlib.h>

// parseIndexList (déjà dans vpiv_utils) : retourne 0 => ALL, >0 => n indices
// parseRGB(const char* s, int out[3]) -> out[0..2]

namespace LrubDispatch {

bool dispatch(const char* propAbbr, const char* inst, const char* value) {
    if (!propAbbr || !inst) return false;

    // parse instances
    int indices[LRUB_NUM_PIXELS];
    int n = parseIndexList(inst, indices, LRUB_NUM_PIXELS);
    if (n < 0) {
        sendError("Lrub","parse","*", "bad_inst");
        return false;
    }

    // ----- col -----
    if (strcmp(propAbbr, "col") == 0) {
        if (!value) { sendError("Lrub","col","*", "no_value"); return false; }
        int rgb[3];
        if (!parseRGB(value, rgb)) { sendError("Lrub","col","*", "bad_rgb"); return false; }

        if (n == 0) {
            // tous
            lrub_applyColorAll(rgb[0], rgb[1], rgb[2]);
        } else {
            // indices fournis
            lrub_applyColorIndices(indices, n, rgb[0], rgb[1], rgb[2]);
        }

        sendInfo("Lrub","col",inst,value);
        return true;
    }

    // ----- int (intensité) -----
    if (strcmp(propAbbr, "int") == 0) {
        int intensity = value ? atoi(value) : cfg_lrub_brightness;
        lrub_setIntensity(intensity);
        char buf[16]; snprintf(buf,sizeof(buf), "%d", intensity);
        sendInfo("Lrub","int","*", buf);
        return true;
    }

    // ----- act (on/off) -----
    if (strcmp(propAbbr, "act") == 0) {
        bool on = (value && (strcmp(value,"1")==0 || strcasecmp(value,"true")==0));
        lrub_setActive(on);
        sendInfo("Lrub","act","*", on ? "1":"0");
        return true;
    }

    // ----- f (fuite) -----
    if (strcmp(propAbbr, "f") == 0) {
        bool on = (value && (strcmp(value,"1")==0 || strcasecmp(value,"true")==0));
        if (on) lrub_effectFuite();
        else lrub_applyColorAll(cfg_lrub_redRB, cfg_lrub_greenRB, cfg_lrub_blueRB);
        sendInfo("Lrub","f","*", on ? "1":"0");
        return true;
    }

    // ----- init -----
    if (strcmp(propAbbr, "init") == 0) {
        lrub_init();
        sendInfo("Lrub","init","*","OK");
        return true;
    }

    // propriété inconnue
    return false;
}

} // namespace LrubDispatch
