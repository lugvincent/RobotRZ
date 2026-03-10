/************************************************************
 * FICHIER  : src/communication/dispatch_Urg.cpp
 * CHEMIN   : arduino/mega/src/communication/dispatch_Urg.cpp
 * VERSION  : 1.1  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Dispatcher VPIV pour le module urgence (Urg).
 *   N'accepte qu'une seule propriété : "clear".
 *
 * PRINCIPE
 * --------
 *   Les urgences sont toujours déclenchées EN INTERNE par le firmware
 *   (urg_handle() appelé depuis mvt_safe, communication, watchdog loop...).
 *   Elles ne peuvent jamais être déclenchées via VPIV depuis SP.
 *
 *   La seule commande acceptée est l'effacement : $V:Urg:*:clear:1#
 *   Elle appelle urg_clear() qui :
 *     - remet le système en état opérationnel
 *     - publie lui-même $I:Urg:etat:*:cleared# (pas de doublon ici)
 *
 * PROPRIÉTÉ SUPPORTÉE (SP->A)
 * ----------------------------
 *   clear : efface l'urgence active
 *     Ex : $V:Urg:*:clear:1#
 *
 * PROPRIÉTÉ PUBLIÉE EN RETOUR (A->SP)
 * -------------------------------------
 *   etat (I) : "cleared" — publié par urg_clear() dans urg.cpp
 *
 * ARTICULATION
 * ------------
 *   vpiv_dispatch.cpp → dispatch_Urg.cpp → urg.cpp
 *   Note : ce dispatcher est autorisé même si urg_isActive() est vrai
 *   (exception gérée dans vpiv_dispatch.cpp et communication.cpp).
 *
 * PRÉCAUTIONS
 * -----------
 *   - Ne pas ajouter d'autres propriétés ici sans réfléchir aux
 *     implications de sécurité (ce module bypasse le filtre urgence).
 ************************************************************/

#include "vpiv_dispatch.h"
#include "system/urg.h"
#include "communication/communication.h"
#include <string.h>

namespace Urg
{

    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop)
            return false;

        // clear → efface l'urgence active
        // urg_clear() publie lui-même $I:Urg:etat:*:cleared#
        if (strcmp(prop, "clear") == 0)
        {
            urg_clear();
            return true;
        }

        // Toute autre propriété est refusée
        sendError("Urg", prop, inst ? inst : "*", "seule clear est autorisee");
        return false;
    }

} // namespace Urg