/************************************************************
 * FILE : src/communication/vpiv_dispatch.cpp
 * ROLE : Routeur central VPIV -> dispatchers de modules
 *
 * Gère :
 *   - le filtrage en cas d'urgence (URG)
 *   - la redirection vers chaque dispatcher de module
 *   - SCfg::clear et Urg::clear même en urgence
 ************************************************************/

#include "vpiv_dispatch.h"
#include "vpiv_utils.h"

#include "../system/urg.h"                // gestion urgence
#include "../communication/communication.h"

#include <string.h>
#include <ctype.h>

/************************************************************
 * TABLE DES DISPATCHERS
 *
 *  👉 C’est ici que tu places UrgDispatch::dispatch
 ************************************************************/
struct DispatchEntry {
    const char* varName;
    bool (*func)(const char*, const char*, const char*);
};

// ---- TABLE FINALE ----
// ⚠️ L’ordre n’a pas d’impact, mais chaque module doit être listé.
static DispatchEntry ACTION_TABLE[] = {
    { "lrub",    lrub::dispatch },
    { "lring",   lring::dispatch },

    { "srv",     Srv::dispatch },
    { "mt",      Mt::dispatch },
    { "us",      US::dispatch },
    { "mic",     Mic::dispatch },

    { "odom",    Odom::dispatch },
    { "mvt",     Mvt_ir::dispatch },
    { "fs",      FS::dispatch },

    { "scfg",    SCfg::dispatch },

    { "mvtsafe", MvtSafeDispatch::dispatch },

    { "urg",     UrgDispatch::dispatch },   // 🔥 AJOUT : Dispatch urgence

    { nullptr,   nullptr }
};


/************************************************************
 * dispatchVPIV()
 ************************************************************/
bool dispatchVPIV(const char* var,
                  const char* inst,
                  const char* prop,
                  const char* value)
{
    if (!var || !prop) return false;

    /******************************************************
     * 1) BLOQUER LES COMMANDES SI URGENCE ACTIVE
     *
     *  Seules exceptions autorisées :
     *      - SCfg::clear
     *      - UrgDispatch::clear
     ******************************************************/
    if (urg_isActive()) {

        // Autoriser SCFG clear
        if (strcmp(var, "scfg") == 0 &&
            strcmp(prop, "clear") == 0)
        {
            return SCfg::dispatch(prop, inst, value);
        }

        // Autoriser URG clear
        if (strcmp(var, "urg") == 0 &&
            strcmp(prop, "clear") == 0)
        {
            return UrgDispatch::dispatch(prop, inst, value);
        }

        // Refuser tout le reste → renvoyer état urgence
        urg_sendAlert(urg_getCode());
        return false;
    }

    /******************************************************
     * 2) TROUVER LE MODULE DANS LA TABLE
     ******************************************************/
    for (int i = 0; ACTION_TABLE[i].varName; i++) {
        if (strcmp(ACTION_TABLE[i].varName, var) == 0) {
            return ACTION_TABLE[i].func(prop, inst, value);
        }
    }

    /******************************************************
     * 3) MODULE INCONNU → erreur parsing
     ******************************************************/
    urg_handle(URG_PARSING_ERROR);
    return false;
}
