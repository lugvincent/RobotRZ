/************************************************************
 * FICHIER  : src/communication/dispatch_Lrub.cpp
 * CHEMIN   : arduino/mega/src/communication/dispatch_Lrub.cpp
 * VERSION  : 1.2  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Dispatcher VPIV pour le ruban LED (module Lrub).
 *   Reçoit les trames VPIV entrantes ($V:Lrub:...) et
 *   appelle les fonctions correspondantes du module lrub.cpp.
 *
 * PROPRIÉTÉS SUPPORTÉES (SP->A)
 * ------------------------------
 *   col        : couleur RGB — valeur = "R,G,B"  (ex : "255,0,0" = rouge)
 *                Ex : $V:Lrub:*:col:255,128,0#
 *
 *   lumin      : intensité lumineuse — valeur = 0..255
 *                (ex : $V:Lrub:*:lumin:128#)
 *                NOTE : anciennement "int" — renommé car "int" est un type C++,
 *                ce qui prête à confusion lors de la lecture du code.
 *
 *   act        : activation — 1/0 ou true/false
 *                Ex : $V:Lrub:*:act:1#
 *
 *   timeout    : extinction automatique après N millisecondes
 *                valeur = durée en ms (ex : "5000" = 5 secondes)
 *                valeur = "0" pour annuler un timeout en cours
 *                Ex : $V:Lrub:*:timeout:5000#
 *
 *   effetFuite : effet visuel "fuite" — 1 pour activer, 0 pour revenir couleur config
 *                Ex : $V:Lrub:*:effetFuite:1#
 *
 *   init       : réinitialise le module (couleur et état par défaut config)
 *                Ex : $V:Lrub:*:init:1#
 *
 * INSTANCES
 * ---------
 *   "*"     → tous les pixels (défaut)
 *   "N"     → pixel unique à l'indice N
 *   "[a,b]" → liste de pixels
 *   (pour col uniquement — les autres propriétés s'appliquent au ruban entier)
 *
 * PROPRIÉTÉS PUBLIÉES EN RETOUR (A->SP)
 * ---------------------------------------
 *   col        (I) : confirmation couleur appliquée
 *   lumin      (I) : confirmation intensité appliquée
 *   act        (I) : confirmation activation/désactivation
 *   timeout    (I) : notification "OK" quand l'extinction automatique s'est déclenchée
 *   effetFuite (I) : confirmation effet appliqué
 *   init       (I) : confirmation réinitialisation
 *
 * ARTICULATION
 * ------------
 *   vpiv_dispatch.cpp → dispatch_Lrub.cpp → lrub.cpp → lrub_hardware.cpp
 *   La gestion du timeout (décompte et extinction) est dans lrub_processTimeout()
 *   appelé périodiquement depuis loop() dans main.cpp.
 *
 * CORRECTION v1.2
 * ---------------
 *   Suppression de #include "hardware/lrub_hardware.h" : inutile dans ce fichier.
 *   Ce dispatcher appelle uniquement les fonctions de lrub.cpp (API haut-niveau).
 *   L'accès au matériel se fait via lrub.cpp → lrub_hardware.cpp, pas directement ici.
 *   Inclure lrub_hardware.h ici créait une dépendance inutile et rendait le code
 *   plus fragile aux évolutions de l'API matérielle.
 *
 * PRÉCAUTIONS
 * -----------
 *   - lrub_processTimeout() DOIT être appelé dans loop() pour que le timeout fonctionne.
 *   - Un timeout de 0 annule un timeout en cours sans éteindre le ruban.
 *   - lumin n'affecte pas la couleur, seulement la luminosité globale.
 ************************************************************/

#include "vpiv_dispatch.h"
#include "system/urg.h"
#include "actuators/lrub.h"
// ⚠ PAS d'include lrub_hardware.h ici — accès matériel via lrub.cpp uniquement
#include "config/config.h"
#include "communication/vpiv_utils.h"
#include "communication/communication.h"

#include <string.h>
#include <stdlib.h>

namespace Lrub
{

    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop || !inst)
            return false;

        // Décode les indices d'instance (0 = tous, >0 = liste)
        int indices[LRUB_NUM_PIXELS];
        int n = parseIndexList(inst, indices, LRUB_NUM_PIXELS);
        if (n < 0)
        {
            sendError("Lrub", "parse", "*", "bad_inst");
            return false;
        }

        // ----------------------------------------------------------------
        // col — couleur RGB
        //   value = "R,G,B"  (entiers 0..255 séparés par virgules)
        // ----------------------------------------------------------------
        if (strcmp(prop, "col") == 0)
        {
            if (!value)
            {
                sendError("Lrub", "col", "*", "no_value");
                return false;
            }
            int rgb[3];
            if (!parseRGB(value, rgb))
            {
                sendError("Lrub", "col", "*", "bad_rgb");
                return false;
            }

            if (n == 0)
                lrub_applyColorAll(rgb[0], rgb[1], rgb[2]); // tous les pixels
            else
                lrub_applyColorIndices(indices, n, rgb[0], rgb[1], rgb[2]); // pixels spécifiques

            sendInfo("Lrub", "col", inst, value);
            return true;
        }

        // ----------------------------------------------------------------
        // lumin — intensité lumineuse globale (0..255)
        //   Anciennement "int" — renommé pour éviter la confusion avec le type C++ int.
        //   N'affecte pas la couleur, seulement la luminosité.
        // ----------------------------------------------------------------
        if (strcmp(prop, "lumin") == 0)
        {
            int intensity = value ? atoi(value) : cfg_lrub_brightness;
            lrub_setIntensity(intensity);

            char buf[16];
            snprintf(buf, sizeof(buf), "%d", intensity);
            sendInfo("Lrub", "lumin", inst, buf);
            return true;
        }

        // ----------------------------------------------------------------
        // act — activation / désactivation du ruban
        //   1 ou "true" → allumé ; 0 ou "false" → éteint
        // ----------------------------------------------------------------
        if (strcmp(prop, "act") == 0)
        {
            bool on = (value && (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0));
            lrub_setActive(on);
            sendInfo("Lrub", "act", inst, on ? "1" : "0");
            return true;
        }

        // ----------------------------------------------------------------
        // timeout — extinction automatique après N millisecondes
        //   value = "5000" → extinction dans 5 secondes
        //   value = "0"    → annule le timeout en cours (ne pas éteindre)
        //   La gestion du décompte est dans lrub_processTimeout() / loop()
        // ----------------------------------------------------------------
        if (strcmp(prop, "timeout") == 0)
        {
            unsigned long ms = value ? (unsigned long)atol(value) : 0;
            if (ms == 0)
            {
                // Annulation du timeout sans extinction
                lrub_setTimeout(0);
                sendInfo("Lrub", "timeout", "*", "annule");
            }
            else
            {
                // Planifie l'extinction dans ms millisecondes
                lrub_setTimeout(millis() + ms);

                char buf[16];
                snprintf(buf, sizeof(buf), "%lu", ms);
                sendInfo("Lrub", "timeout", "*", buf); // confirme la durée programmée
            }
            return true;
        }

        // ----------------------------------------------------------------
        // effetFuite — effet visuel alterné pairs/impairs
        //   1 → déclenche l'effet
        //   0 → revient à la couleur de configuration
        // ----------------------------------------------------------------
        if (strcmp(prop, "effetFuite") == 0)
        {
            bool on = (value && (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0));
            if (on)
                lrub_effectFuite();
            else
                lrub_applyColorAll(cfg_lrub_redRB, cfg_lrub_greenRB, cfg_lrub_blueRB);

            sendInfo("Lrub", "effetFuite", "*", on ? "1" : "0");
            return true;
        }

        // ----------------------------------------------------------------
        // init — réinitialisation complète du module
        //   Recharge les valeurs par défaut (config), annule timeout
        // ----------------------------------------------------------------
        if (strcmp(prop, "init") == 0)
        {
            lrub_init();
            sendInfo("Lrub", "init", "*", "OK");
            return true;
        }

        // Propriété non reconnue
        return false;
    }

} // namespace Lrub