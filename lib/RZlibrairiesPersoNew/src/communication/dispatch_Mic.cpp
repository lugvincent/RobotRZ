/************************************************************
 * FICHIER  : src/communication/dispatch_Mic.cpp
 * CHEMIN   : arduino/mega/src/communication/dispatch_Mic.cpp
 * VERSION  : 2.1  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Dispatcher VPIV pour le module microphone (Mic).
 *   Reçoit les trames entrantes ($V:Mic:*:...) et configure mic.cpp.
 *
 * ============================================================
 * PROPRIÉTÉS SUPPORTÉES (SP->A)
 * ============================================================
 *
 * modeMicsF  — mode de publication flux (un seul actif à la fois)
 * ---------------------------------------------------------------
 *   Valeurs : "OFF" | "MOY" | "ANGLE" | "PIC"
 *   Ex : $V:Mic:*:modeMicsF:MOY#
 *        $V:Mic:*:modeMicsF:OFF#
 *
 * modeMicsI  — mode de détection événements (exclusif)
 * ---------------------------------------------------------------
 *   Valeurs : "OFF" | "InfoNuisce" | "DetectPresence"
 *   Ex : $V:Mic:*:modeMicsI:InfoNuisce#
 *        $V:Mic:*:modeMicsI:DetectPresence#
 *        $V:Mic:*:modeMicsI:OFF#
 *
 * paraMicsF  — paramètres du mode flux (FORMAT_VALUE objet CSV)
 * ---------------------------------------------------------------
 *   Format strict : 6 valeurs entières séparées par virgules, sans espace
 *   "freqMoy,freqPic,winMoy,winPic,deltaSeuil,deltaAngle"
 *
 *   freqMoy     (ms) : cadence publication micMoy et micAngle
 *   freqPic     (ms) : cadence publication micPIC
 *   winMoy      (ms) : fenêtre hardware pour MOY et ANGLE
 *   winPic      (ms) : fenêtre hardware pour PIC
 *   deltaSeuil       : variation min (unités ADC) pour publier MOY ou PIC
 *                      0 = toujours publier (pas d'anti-bruit)
 *   deltaAngle  (°)  : variation min d'angle pour publier micAngle
 *                      0 = toujours publier
 *   Ex : $V:Mic:*:paraMicsF:200,100,200,100,5,10#
 *
 * paraMicsI  — paramètres de détection événements (FORMAT_VALUE objet CSV)
 * ---------------------------------------------------------------
 *   Format strict : 7 valeurs entières séparées par virgules, sans espace
 *   "seuil0Parasite,seuil1Bruit,seuil2Bruit,seuilPresence,
 *    dureeNuisce,dureePresence,cooldown"
 *
 *   seuil0Parasite   : plancher commun matériel (ADC)
 *                      En dessous : signal ignoré (parasite du circuit)
 *   seuil1Bruit      : seuil de déclenchement bruitNiv1 (ADC)
 *   seuil2Bruit      : seuil de déclenchement bruitNiv2 (ADC)
 *   seuilPresence    : seuil de déclenchement detectPresenceSon (ADC)
 *   dureeNuisce (ms) : durée soutenue avant signalement InfoNuisce
 *   dureePresence(ms): durée soutenue avant signalement DetectPresence
 *   cooldown    (ms) : délai min entre deux signalements du même type
 *                      (bruitNiv1, bruitNiv2 et detectPresenceSon
 *                       ont chacun leur propre compteur cooldown)
 *   Ex : $V:Mic:*:paraMicsI:30,300,600,100,500,300,2000#
 *
 * ============================================================
 * PROPRIÉTÉS PUBLIÉES EN RETOUR (A->SP)
 * ============================================================
 *   modeMicsF (I) : confirmation du mode activé (ex : "MOY")
 *   modeMicsI (I) : confirmation du mode activé (ex : "InfoNuisce")
 *   paraMicsF (I) : confirmation "OK"
 *   paraMicsI (I) : confirmation "OK"
 *   (les publications de données sont dans mic.cpp)
 *
 * ============================================================
 * INSTANCE
 * ============================================================
 *   Toujours "*" — configuration globale des 3 micros.
 *
 * ============================================================
 * PRÉCAUTIONS D'ORDRE D'ENVOI
 * ============================================================
 *   Ordre recommandé :
 *     1. paraMicsI (seuils et durées)
 *     2. paraMicsF (cadences et fenêtres)
 *     3. modeMicsF (active le flux)
 *     4. modeMicsI (active la détection)
 *   Si on active un mode avant de configurer ses paramètres,
 *   les valeurs par défaut (conservatives) s'appliquent.
 *
 * ARTICULATION
 * ------------
 *   vpiv_dispatch.cpp → dispatch_Mic.cpp → mic.cpp → mic_hardware.cpp
 ************************************************************/

#include "vpiv_dispatch.h"
#include "sensors/mic.h"
#include "config/config.h"
#include "communication/communication.h"
#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

namespace Mic
{

    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop || !inst || !value)
            return false;

        // Toujours instance globale "*" pour ce module
        if (!(inst[0] == '*' && inst[1] == '\0'))
            return false;

        /* --------------------------------------------------------
         * modeMicsF — mode de publication flux
         * -------------------------------------------------------- */
        if (strcmp(prop, "modeMicsF") == 0)
        {
            uint8_t mode;

            if (strcmp(value, "OFF") == 0)
                mode = MIC_MODE_F_OFF;
            else if (strcmp(value, "MOY") == 0)
                mode = MIC_MODE_F_MOY;
            else if (strcmp(value, "ANGLE") == 0)
                mode = MIC_MODE_F_ANGLE;
            else if (strcmp(value, "PIC") == 0)
                mode = MIC_MODE_F_PIC;
            else
            {
                sendError("Mic", "modeMicsF", "*", "OFF|MOY|ANGLE|PIC attendu");
                return false;
            }

            mic_setModeF(mode);
            sendInfo("Mic", "modeMicsF", "*", value);
            return true;
        }

        /* --------------------------------------------------------
         * modeMicsI — mode de détection événements (exclusif)
         * -------------------------------------------------------- */
        if (strcmp(prop, "modeMicsI") == 0)
        {
            uint8_t mode;

            if (strcmp(value, "OFF") == 0)
                mode = MIC_MODE_I_OFF;
            else if (strcmp(value, "InfoNuisce") == 0)
                mode = MIC_MODE_I_NUISCE;
            else if (strcmp(value, "DetectPresence") == 0)
                mode = MIC_MODE_I_PRESENCE;
            else
            {
                sendError("Mic", "modeMicsI", "*", "OFF|InfoNuisce|DetectPresence attendu");
                return false;
            }

            mic_setModeI(mode);
            sendInfo("Mic", "modeMicsI", "*", value);
            return true;
        }

        /* --------------------------------------------------------
         * paraMicsF — paramètres flux
         *   "freqMoy,freqPic,winMoy,winPic,deltaSeuil,deltaAngle"
         *   6 champs obligatoires, séparés par virgules.
         *   En cas de valeur manquante : valeur par défaut appliquée.
         * -------------------------------------------------------- */
        if (strcmp(prop, "paraMicsF") == 0)
        {
            char buf[64];
            strncpy(buf, value, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';

            // Parsing position par position (strtok modifie le buffer)
            char *tok;

            tok = strtok(buf, ",");
            unsigned long freqMoy = tok ? (unsigned long)atol(tok) : 200;

            tok = strtok(NULL, ",");
            unsigned long freqPic = tok ? (unsigned long)atol(tok) : 100;

            tok = strtok(NULL, ",");
            unsigned long winMoy = tok ? (unsigned long)atol(tok) : 200;

            tok = strtok(NULL, ",");
            unsigned long winPic = tok ? (unsigned long)atol(tok) : 100;

            tok = strtok(NULL, ",");
            int deltaSeuil = tok ? atoi(tok) : 5;

            tok = strtok(NULL, ",");
            int deltaAngle = tok ? atoi(tok) : 10;

            mic_setParaF(freqMoy, freqPic, winMoy, winPic, deltaSeuil, deltaAngle);
            sendInfo("Mic", "paraMicsF", "*", "OK");
            return true;
        }

        /* --------------------------------------------------------
         * paraMicsI — paramètres événements
         *   "seuil0Parasite,seuil1Bruit,seuil2Bruit,seuilPresence,
         *    dureeNuisce,dureePresence,cooldown"
         *   7 champs obligatoires, séparés par virgules.
         *   Valeurs en unités ADC pour les seuils, ms pour les durées.
         * -------------------------------------------------------- */
        if (strcmp(prop, "paraMicsI") == 0)
        {
            char buf[80];
            strncpy(buf, value, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';

            char *tok;

            tok = strtok(buf, ",");
            int s0 = tok ? atoi(tok) : 30; // seuil0Parasite

            tok = strtok(NULL, ",");
            int s1 = tok ? atoi(tok) : 300; // seuil1Bruit

            tok = strtok(NULL, ",");
            int s2 = tok ? atoi(tok) : 600; // seuil2Bruit

            tok = strtok(NULL, ",");
            int sP = tok ? atoi(tok) : 100; // seuilPresence

            tok = strtok(NULL, ",");
            unsigned long dNuisce = tok ? (unsigned long)atol(tok) : 500;

            tok = strtok(NULL, ",");
            unsigned long dPresence = tok ? (unsigned long)atol(tok) : 300;

            tok = strtok(NULL, ",");
            unsigned long cooldown = tok ? (unsigned long)atol(tok) : 2000;

            mic_setParaI(s0, s1, s2, sP, dNuisce, dPresence, cooldown);
            sendInfo("Mic", "paraMicsI", "*", "OK");
            return true;
        }

        // Propriété non reconnue
        return false;
    }

} // namespace Mic