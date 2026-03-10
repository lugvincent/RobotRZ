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

#include "system/urg.h"                  // urg_clear(), urg_isActive()
#include "communication/communication.h" // sendInfo(), sendError()
#include <string.h>
#include <ctype.h>

/************************************************************
 * TABLE DES DISPATCHERS
 ************************************************************/
struct DispatchEntry
{
    const char *varName;
    bool (*func)(const char *, const char *, const char *);
};

// ---- TABLE FINALE ----
// ⚠️ L’ordre n’a pas d’impact, mais chaque module doit être listé.
static DispatchEntry ACTION_TABLE[] = {
    {"Lrub", Lrub::dispatch},
    {"Lring", Lring::dispatch},

    {"Srv", Srv::dispatch},
    {"Mtr", Mtr::dispatch},
    {"US", US::dispatch},
    {"Mic", Mic::dispatch},
    {"Odom", Odom::dispatch},
    {"IRmvt", IRmvt::dispatch},
    {"FS", FS::dispatch},

    {"Ctrl_L", Ctrl_L::dispatch},

    {"CfgS", CfgS::dispatch},

    {"SecMvt", SecMvt::dispatch},

    {"Urg", Urg::dispatch}, // 🔥 AJOUT : Dispatch urgence

    {nullptr, nullptr}};

/************************************************************
 * dispatchVPIV()
 ************************************************************/
bool dispatchVPIV(const char *var,
                  const char *prop,
                  const char *inst,
                  const char *value)
{
    if (!var || !prop)
        return false;

    /******************************************************
     * 1) BLOQUER LES COMMANDES SI URGENCE ACTIVE
     *
     *  Seules exceptions autorisées :
     *      - CfgS::emg:clear (effacement urgence)
     *      - Urg::clear (alternative pour effacement)
     ******************************************************/
    if (urg_isActive())
    {

        // Autoriser SCFG clear
        if (strcmp(var, "CfgS") == 0 && // Module CfgS uniquement
            strcmp(prop, "emg") == 0 && // Propriété "emg"
            value &&                    // Vérifie que value n'est pas NULL
            strcmp(value, "clear") == 0 // Valeur doit être "clear"
        )
        {
            return CfgS::dispatch(prop, inst, value);
        }

        // Autoriser URG clear (alternative)
        if (strcmp(var, "Urg") == 0 &&
            strcmp(prop, "clear") == 0)
        {
            return Urg::dispatch(prop, inst, value);
        }

        // Refuser tout le reste
        sendError(var, prop, inst, "urgence active");
        return false;
    }

    /******************************************************
     * 2) TROUVER LE MODULE DANS LA TABLE
     ******************************************************/
    for (int i = 0; ACTION_TABLE[i].varName; i++)
    {
        if (strcmp(ACTION_TABLE[i].varName, var) == 0)
        {
            return ACTION_TABLE[i].func(prop, inst, value);
        }
    }

    /******************************************************
     * 3) MODULE INCONNU → erreur parsing
     ******************************************************/
    urg_handle(URG_PARSING_ERROR);
    return false;
}