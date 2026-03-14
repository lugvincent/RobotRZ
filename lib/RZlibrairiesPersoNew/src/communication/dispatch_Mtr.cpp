/************************************************************
 * FICHIER  : src/communication/dispatch_Mtr.cpp
 * CHEMIN   : arduino/mega/src/communication/dispatch_Mtr.cpp
 * VERSION  : 2.0  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Dispatcher VPIV pour le module moteurs (Mtr).
 *   Reçoit les trames entrantes ($V:Mtr:*:...) et appelle
 *   les fonctions correspondantes de mtr.cpp.
 *
 * ============================================================
 * PROPRIÉTÉS SUPPORTÉES (SP->A)
 * ============================================================
 *
 * cmd — commande de déplacement principale
 * -----------------------------------------
 *   Format  : "v,omega,accelLevel"
 *   v       : translation  (-100..+100)  +100=avance, -100=recule
 *   omega   : rotation     (-100..+100)  +100=droite, -100=gauche
 *   accelLevel : lissage   (0..4)        0=direct, 4=très doux
 *   Ex : $V:Mtr:*:cmd:80,0,2#       → avance droit, lissage moyen
 *   Ex : $V:Mtr:*:cmd:0,-60,2#      → tourne gauche sur place
 *   Ex : $V:Mtr:*:cmd:-50,30,1#     → recule en virant droite
 *
 * stop — arrêt propre
 * --------------------
 *   Format  : "1" (toute valeur non vide est acceptée)
 *   Remet v=0, omega=0, envoie <0,0,0> à l'UNO.
 *   Ex : $V:Mtr:*:stop:1#
 *
 * override — arrêt forcé / lever d'arrêt forcé
 * ---------------------------------------------
 *   Format  : "stop" ou "clear"
 *   "stop"  → arrêt immédiat, toutes les cmd suivantes ignorées
 *   "clear" → reprend les commandes normales
 *   Ex : $V:Mtr:*:override:stop#
 *   Ex : $V:Mtr:*:override:clear#
 *
 * scale — facteur de réduction des vitesses
 * ------------------------------------------
 *   Format  : entier 0..1000 encodé ×1000 (règle pas-de-float-dans-VPIV)
 *   Réduit toutes les vitesses L et R par ce facteur.
 *     0    = arrêt complet (sans modifier les consignes cibles)
 *     500  = moitié de la vitesse nominale
 *     1000 = vitesse normale
 *   Ex : $V:Mtr:*:scale:500#    → moitié de vitesse (= 0.500)
 *   Ex : $V:Mtr:*:scale:750#    → 3/4 de vitesse    (= 0.750)
 *   Conversion interne : s_float = s_int / 1000.0f
 *
 * kturn — coefficient de rotation
 * ---------------------------------
 *   Format  : entier 0..2000 encodé ×1000 (règle pas-de-float-dans-VPIV)
 *   Amplifie ou réduit l'effet de omega sur les roues.
 *     500  = rotation douce
 *     1000 = standard
 *     1500 = rotation franche
 *   Ex : $V:Mtr:*:kturn:800#    → coefficient 0.800
 *   Ex : $V:Mtr:*:kturn:1000#   → coefficient 1.000 (standard)
 *   Conversion interne : k_float = k_int / 1000.0f
 *
 * act — activation du module
 * ---------------------------
 *   Format  : "1" ou "0"
 *   0 → le module calcule mais n'envoie plus de trame à l'UNO
 *   Ex : $V:Mtr:*:act:1#
 *
 * read — lecture de l'état courant
 * ----------------------------------
 *   Format  : toute valeur (ignorée)
 *   Publie $I:Mtr:state:*:L,R,A# avec les consignes cibles actuelles
 *   Ex : $V:Mtr:*:read:1#
 *
 * ============================================================
 * PROPRIÉTÉS PUBLIÉES EN RETOUR (A->SP)
 * ============================================================
 *   targets  (I) : L,R,A après chaque cmd (depuis mtr.cpp)
 *   stop     (I) : "OK" après stop
 *   override (I) : "stop" ou "cleared"
 *   scale    (I) : entier ×1000 appliqué (ex : 750 = 0.750)
 *   kturn    (I) : entier ×1000 appliqué (ex : 800 = 0.800)
 *   act      (I) : "1" ou "0"
 *   state    (I) : "L,R,A" état courant (réponse à read)
 *
 * ============================================================
 * SÉCURITÉ EN URGENCE
 * ============================================================
 *   Si urg_isActive() est vrai, seules les propriétés "read",
 *   "act", "override:clear" et "stop" sont autorisées.
 *   Toute autre commande est rejetée avec sendError().
 *
 * ARTICULATION
 * ------------
 *   vpiv_dispatch.cpp → dispatch_Mtr.cpp → mtr.cpp → mtr_hardware.cpp
 ************************************************************/

#include "vpiv_dispatch.h"
#include <Arduino.h>
#include "actuators/mtr.h"
#include "system/urg.h"
#include "config/config.h"
#include "communication/communication.h"
#include <string.h>
#include <stdlib.h>

namespace Mtr
{

    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop || !inst)
            return false;

        // Ce module n'accepte que l'instance globale "*"
        if (!(inst[0] == '*' && inst[1] == '\0'))
        {
            sendError("Mtr", prop, inst ? inst : "?", "instance * uniquement");
            return false;
        }

        /* --------------------------------------------------------
         * Vérification urgence active
         * En urgence, on n'autorise que :
         *   - read   : lire l'état sans agir
         *   - stop   : arrêt propre (sécurité)
         *   - act    : désactivation possible
         *   - override:clear : lever l'override si c'est voulu
         * Toute commande de mouvement est bloquée.
         * -------------------------------------------------------- */
        if (urg_isActive())
        {
            bool permis = (strcmp(prop, "read") == 0 ||
                           strcmp(prop, "stop") == 0 ||
                           strcmp(prop, "act") == 0 ||
                           strcmp(prop, "override") == 0);
            if (!permis)
            {
                sendError("Mtr", prop, "*", "urgence_active");
                return false;
            }
        }

        /* --------------------------------------------------------
         * cmd — commande de déplacement principale
         *   "v,omega,accelLevel"
         * -------------------------------------------------------- */
        if (strcmp(prop, "cmd") == 0)
        {
            if (!value)
            {
                sendError("Mtr", "cmd", "*", "valeur manquante");
                return false;
            }
            int v = 0, omega = 0, acc = 2; // acc=2 par défaut (lissage moyen)
            sscanf(value, "%d,%d,%d", &v, &omega, &acc);

            mtr_setTargetsSigned(v, omega, acc);
            // NOTE : mtr_setTargetsSigned() publie lui-même $I:Mtr:targets:*:...#
            return true;
        }

        /* --------------------------------------------------------
         * stop — arrêt propre
         * -------------------------------------------------------- */
        if (strcmp(prop, "stop") == 0)
        {
            mtr_stopAll();
            // NOTE : mtr_stopAll() publie lui-même $I:Mtr:stop:*:OK#
            return true;
        }

        /* --------------------------------------------------------
         * override — arrêt forcé ou lever d'arrêt forcé
         *   "stop"  → mtr_overrideStop()
         *   "clear" → mtr_clearOverride()
         * -------------------------------------------------------- */
        if (strcmp(prop, "override") == 0)
        {
            if (!value)
                return false;

            if (strcmp(value, "stop") == 0)
            {
                mtr_overrideStop();
                return true;
            }
            if (strcmp(value, "clear") == 0)
            {
                mtr_clearOverride();
                return true;
            }

            sendError("Mtr", "override", "*", "stop|clear attendu");
            return false;
        }

        /* --------------------------------------------------------
         * scale — facteur de réduction des vitesses
         *
         * Encodage VPIV : entier ×1000 (règle pas-de-float-dans-VPIV)
         *   Valeur reçue : 0..1000
         *   Conversion   : s_float = s_int / 1000.0f → transmise à mtr_scaleTargets()
         *
         * Effet de bord : mtr_scaleTargets() publie le retour ACK.
         *   Le retour est également en entier ×1000 ($I:Mtr:scale:*:750#).
         *
         * Garde-fou : valeurs < 0 → clamp à 0, valeurs > 1000 → clamp à 1000.
         * -------------------------------------------------------- */
        if (strcmp(prop, "scale") == 0)
        {
            if (!value)
                return false;

            // Lecture de la valeur entière ×1000 (ex : "750" = facteur 0.750)
            int s_int = atoi(value);

            // Clamp protecteur côté dispatcher (mtr_scaleTargets clampera aussi)
            if (s_int < 0)
                s_int = 0;
            if (s_int > 1000)
                s_int = 1000;

            // Conversion vers float interne (usage calcul uniquement, jamais dans VPIV)
            float s_float = s_int / 1000.0f;
            mtr_scaleTargets(s_float, s_int); // s_int transmis pour l'ACK conforme
            // NOTE : mtr_scaleTargets() publie $I:Mtr:scale:*:<s_int># (entier ×1000)
            return true;
        }

        /* --------------------------------------------------------
         * kturn — coefficient de rotation dans le calcul différentiel
         *
         * Encodage VPIV : entier ×1000 (règle pas-de-float-dans-VPIV)
         *   Valeur reçue : 0..2000
         *   Conversion   : k_float = k_int / 1000.0f → transmise à mtr_setKturn()
         *
         * Effet de bord : mtr_setKturn() publie le retour ACK.
         *   Le retour est également en entier ×1000 ($I:Mtr:kturn:*:800#).
         *
         * Garde-fou : valeurs < 0 → clamp à 0, valeurs > 2000 → clamp à 2000.
         *   kturn > 2.0 produirait des valeurs L/R hors plage → dangereux.
         * -------------------------------------------------------- */
        if (strcmp(prop, "kturn") == 0)
        {
            if (!value)
                return false;

            // Lecture de la valeur entière ×1000 (ex : "800" = coefficient 0.800)
            int k_int = atoi(value);

            // Clamp protecteur côté dispatcher (mtr_setKturn clampera aussi)
            if (k_int < 0)
                k_int = 0;
            if (k_int > 2000)
                k_int = 2000;

            // Conversion vers float interne (usage calcul uniquement, jamais dans VPIV)
            float k_float = k_int / 1000.0f;
            mtr_setKturn(k_float, k_int); // k_int transmis pour l'ACK conforme
            // NOTE : mtr_setKturn() publie $I:Mtr:kturn:*:<k_int># (entier ×1000)
            return true;
        }

        /* --------------------------------------------------------
         * act — activation / désactivation du module
         *   1 ou "true"  → actif (envoi des trames autorisé)
         *   0 ou "false" → inactif (calculs faits, trames non envoyées)
         * -------------------------------------------------------- */
        if (strcmp(prop, "act") == 0)
        {
            bool on = (value && (strcmp(value, "1") == 0 ||
                                 strcmp(value, "true") == 0));
            cfg_mtr_active = on;

            if (!on)
                mtr_stopAll(); // sécurité : arrêt avant désactivation

            sendInfo("Mtr", "act", "*", on ? "1" : "0");
            return true;
        }

        /* --------------------------------------------------------
         * read — lecture de l'état courant
         *   Publie les consignes cibles actuelles : L, R, A
         * -------------------------------------------------------- */
        if (strcmp(prop, "read") == 0)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d,%d,%d",
                     cfg_mtr_targetL, cfg_mtr_targetR, cfg_mtr_targetA);
            sendInfo("Mtr", "state", "*", buf);
            return true;
        }

        // Propriété non reconnue
        return false;
    }

} // namespace Mtr