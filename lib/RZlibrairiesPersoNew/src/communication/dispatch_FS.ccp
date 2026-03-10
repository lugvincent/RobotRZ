/************************************************************
 * FICHIER  : src/communication/dispatch_FS.cpp
 * CHEMIN   : arduino/mega/src/communication/dispatch_FS.cpp
 * VERSION  : 2.0  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Dispatcher VPIV pour le module Force Sensor (FS).
 *   Reçoit les trames VPIV entrantes ($V:FS:...) et appelle
 *   les fonctions correspondantes du module fs.cpp.
 *
 * PROPRIÉTÉS SUPPORTÉES (SP->A)
 * ------------------------------
 *   read  : lecture immédiate — valeur = "force" (défaut) ou "raw"
 *           Ex : $V:FS:*:read:force#   → publie $F:FS:force:*:<val>#
 *           Ex : $V:FS:*:read:raw#     → publie $F:FS:raw:*:<val>#
 *
 *   tare  : remise à zéro (offset = valeur actuelle à vide)
 *           Ex : $V:FS:*:tare:1#       → confirme $I:FS:tare:*:OK#
 *
 *   cal   : facteur de calibration (entier, stocké x1000 dans Table A)
 *           Ex : $V:FS:*:cal:950#      → confirme $I:FS:cal:*:OK#
 *
 *   act   : active (1) ou désactive (0) le module
 *           Ex : $V:FS:*:act:1#        → confirme $I:FS:act:*:1#
 *
 *   freq  : fréquence de publication périodique (ms)
 *           Ex : $V:FS:*:freq:200#     → confirme $I:FS:freq:*:OK#
 *
 *   thd   : seuil de déclenchement alerte (même unité que force)
 *           Ex : $V:FS:*:thd:80#       → confirme $I:FS:thd:*:OK#
 *
 * PROPRIÉTÉS PUBLIÉES EN RETOUR (A->SP)
 * ---------------------------------------
 *   (voir fs.cpp pour la liste complète et les CAT)
 *
 * INSTANCE
 * --------
 *   Toujours "*" — il n'y a qu'un seul capteur de force sur le robot.
 *
 * ARTICULATION
 * ------------
 *   vpiv_dispatch.cpp → dispatch_FS.cpp → fs.cpp → fs_hardware.cpp
 *
 * PRÉCAUTIONS
 * -----------
 *   - La valeur "force" dans la commande read est le nom de propriété,
 *     pas une valeur numérique.
 *   - cal attend un entier (le facteur × 1000, ex : 950 pour 0.950).
 ************************************************************/

#include "vpiv_dispatch.h"
#include "system/urg.h"
#include "sensors/fs.h"
#include "config/config.h"
#include "communication/communication.h"
#include <string.h>
#include <stdlib.h>

namespace FS
{

    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop || !inst || !value)
            return false;

        // Ce module n'a qu'une instance : le capteur unique -> toujours "*"
        if (!(inst[0] == '*' && inst[1] == '\0'))
            return false;

        // ----------------------------------------------------------------
        // read — lecture immédiate à la demande
        //   value = "force" (défaut) : publie la force normalisée
        //   value = "raw"            : publie la valeur brute HX711
        // ----------------------------------------------------------------
        if (strcmp(prop, "read") == 0)
        {
            fs_doRead(value); // value = "force" ou "raw"
            return true;
        }

        // ----------------------------------------------------------------
        // tare — remise à zéro (enregistre la valeur à vide comme offset)
        // ----------------------------------------------------------------
        if (strcmp(prop, "tare") == 0)
        {
            fs_doTare();
            sendInfo("FS", "tare", "*", "OK");
            return true;
        }

        // ----------------------------------------------------------------
        // cal — facteur de calibration
        //   Entier x1000 (ex : 950 pour un facteur réel de 0.950)
        // ----------------------------------------------------------------
        if (strcmp(prop, "cal") == 0)
        {
            long cal = atol(value);
            if (cal == 0)
            {
                // Valeur 0 provoquerait une division par zéro dans fs.cpp
                sendError("FS", "cal", "*", "valeur 0 interdite");
                return false;
            }
            fs_doCal(cal);
            sendInfo("FS", "cal", "*", "OK");
            return true;
        }

        // ----------------------------------------------------------------
        // act — activation / désactivation du module
        //   1 ou "true" → actif ; 0 ou "false" → inactif
        // ----------------------------------------------------------------
        if (strcmp(prop, "act") == 0)
        {
            bool on = (strcmp(value, "1") == 0 || strcmp(value, "true") == 0);
            fs_setActive(on);
            sendInfo("FS", "act", "*", on ? "1" : "0");
            return true;
        }

        // ----------------------------------------------------------------
        // freq — fréquence de publication périodique (ms)
        // ----------------------------------------------------------------
        if (strcmp(prop, "freq") == 0)
        {
            unsigned long ms = (unsigned long)atoi(value);
            fs_setFreq(ms);
            sendInfo("FS", "freq", "*", "OK");
            return true;
        }

        // ----------------------------------------------------------------
        // thd — seuil de déclenchement alerte
        //   Même unité que la force normalisée
        // ----------------------------------------------------------------
        if (strcmp(prop, "thd") == 0)
        {
            int th = atoi(value);
            fs_setThreshold(th);
            sendInfo("FS", "thd", "*", "OK");
            return true;
        }

        // Propriété non reconnue
        return false;
    }

} // namespace FS