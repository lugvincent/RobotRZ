// ======================================================================
// FICHIER : src/communication/dispatch_CfgS.cpp
// Objet   : Dispatcher VPIV pour le module CfgS (configuration système)
// Rôle    :
//   - gestion du mode global du robot (modeRz)
//   - reset système
//   - interface VPIV avec la gestion d'urgence (clear uniquement)
// ======================================================================

#include <string.h>
#include <stdlib.h>

#include "vpiv_dispatch.h"
#include "config/config.h"               // cfg_modeRz
#include "system/urg.h"                  // urg_clear(), urg_isActive()
#include "communication/communication.h" // sendInfo(), sendError()
#include "control/ctrl_L.h"

namespace CfgS
{
    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop)
            return false;

        // --------------------------------------------------
        // MODE GLOBAL ROBOT (modeRZ)
        // strcmp compare la valeur de prop et le second argument "modeRZ" si égaux renvoie 0
        // ce qui signifie qu'ils sont identiques -> vrai
        // $V:CfgS:*:modeRZ:<n>#
        // --------------------------------------------------
        if (strcmp(prop, "modeRZ") == 0) // strcmp compare la valeur de prop et le second argument 0
        {
            if (!value) // test si value est null
                return false;

            cfg_modeRz = (uint8_t)atoi(value);
            sendInfo("CfgS", "modeRZ", "*", value);
            return true;
        }

        // --------------------------------------------------
        // Activation du pilotage par laisse ctrl_L
        // Dépend du TYPE DE PILOTAGE GLOBAL
        // $V:CfgS:*:typePtge:<n># -> n : 3 laisse seul ou 4 laisse et vocal
        // --------------------------------------------------
        if (strcmp(prop, "typePtge") == 0)
        {
            if (!value)
                return false;

            cfg_typePtge = (uint8_t)atoi(value);

            // Activation automatique du contrôle laisse
            if (cfg_typePtge == 3 || cfg_typePtge == 4)
                ctrl_L_setEnabled(true);
            else
                ctrl_L_setEnabled(false);

            sendInfo("CfgS", "typePtge", "*", value);
            return true;
        }

        // --------------------------------------------------
        //EFFACEMENT URGENCE : clear uniquement
        // $V:CfgS:*:emg:clear#
        // --------------------------------------------------
        if (strcmp(prop, "emg") == 0)
        {
            // Vérification sécurité supplémentaire
            if (!value || strcmp(value, "clear") != 0) {
                sendError("CfgS", "emg", "*", "valeur incorrecte, 'clear' attendu");
                return false;
            }

            if (value && strcmp(value, "clear") == 0)
            {
                urg_clear();
                sendInfo("CfgS", "emg", "*", "0");
                return true;
            }
            return false;
        }

        // --------------------------------------------------
        // RESET SYSTEME
        // $V:CfgS:*:reset:1#
        // --------------------------------------------------
        if (strcmp(prop, "reset") == 0)
        {
            // Vérifier la valeur
            if (!value || strcmp(value, "1") != 0) {
                sendError("CfgS", "reset", "*", "bad value, expected '1'");
                return false;
            }
            
            // Vérifier que l'urgence est cleared
            if (urg_isActive()) {
                sendError("CfgS", "reset", "*", "urgence active, clear first");
                return false;
            }
            
            // Effectuer le reset
            RZ_initAll();  // Votre fonction de réinitialisation
            
            // Confirmation
            sendInfo("CfgS", "reset", "*", "OK");
            
            // Optionnel : remettre en mode veille
            cfg_modeRz = 1; // VEILLE
            sendInfo("CfgS", "modeRZ", "*", "1");
            
            return true;
        }

        // Propriété inconnue
        sendError("CfgS", prop, "*", "unknown property");
        return false;
    }
} // namespace CfgS
