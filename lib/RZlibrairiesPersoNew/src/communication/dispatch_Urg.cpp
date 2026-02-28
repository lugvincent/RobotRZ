/************************************************************
 * FICHIER : src/communication/dispatch_Urg.cpp
 * ROLE    : Dispatcher VPIV pour URG (urgence)
 *           - accepte uniquement la commande clear
 *            (si un autre appareil déclenche une urgence, cela
 *             engendrera un clear sur Arduino l'initialisation arrête le moteur)
 *           - toute urgence est levée en interne
 ************************************************************/

#include "vpiv_dispatch.h"
#include "system/urg.h"
#include "communication/communication.h"
#include <string.h>
#include <stdlib.h>

namespace Urg
{

    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop)
            return false;

        // --------------------------------------
        // clear → urg_clear()
        // $V:Urg:clear:*:1#
        // --------------------------------------
        if (strcmp(prop, "clear") == 0)
        {
            urg_clear();
            sendInfo("Urg", "etat", "*", "cleared"); // ajout CAT=I pour ACK
            return true;
        }

        // Toute autre propriété est interdite
        return false;
    }

} // namespace Urg