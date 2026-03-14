/************************************************************
 * FICHIER  : src/communication/dispatch_Odom.cpp
 * CHEMIN   : arduino/mega/lib/RZlibrairiesPersoNew/src/communication/dispatch_Odom.cpp
 * VERSION  : 3.0 — 2026-03-13
 * AUTEUR   : Vincent Philippe
 *
 * OBJECTIF : Recevoir et traiter les commandes VPIV destinées au
 *            module odométrie. Chaque prop (propriété) est une
 *            commande ou un paramètre envoyé par SP (Node-RED) ou SE.
 *
 * PROPRIÉTÉS SUPPORTÉES :
 * ┌──────────────┬──────┬──────────────────────────────────────────────────┐
 * │ prop         │ Dir  │ Description / format value                       │
 * ├──────────────┼──────┼──────────────────────────────────────────────────┤
 * │ paraOdom     │ SP→A │ Configuration complète en une seule trame :      │
 * │              │      │ wheel_mm,track_mm,cpr[,gainG,gainD,fCalc,fPos,fVel]│
 * │              │      │ 3 premiers obligatoires, 5 suivants optionnels   │
 * ├──────────────┼──────┼──────────────────────────────────────────────────┤
 * │ read         │ SP→A │ Lecture immédiate : pos | vel | all              │
 * ├──────────────┼──────┼──────────────────────────────────────────────────┤
 * │ reset        │ SP→A │ Reset pose → value "1"                          │
 * ├──────────────┼──────┼──────────────────────────────────────────────────┤
 * │ act          │ SP→A │ Activer/désactiver → "1" ou "0"                 │
 * ├──────────────┼──────┼──────────────────────────────────────────────────┤
 * │ freq         │ SP→A │ Période calcul odométrique (ms)                 │
 * ├──────────────┼──────┼──────────────────────────────────────────────────┤
 * │ freqPos      │ SP→A │ Période envoi automatique position (ms)         │
 * ├──────────────┼──────┼──────────────────────────────────────────────────┤
 * │ freqVel      │ SP→A │ Période envoi automatique vitesse  (ms)         │
 * ├──────────────┼──────┼──────────────────────────────────────────────────┤
 * │ report       │ SP→A │ "0" = rapport one-shot immédiat (pos+vel)       │
 * ├──────────────┼──────┼──────────────────────────────────────────────────┤
 * │ coeff        │ SP→A │ Override direct : gL,dL,gA,dA (expert)         │
 * ├──────────────┼──────┼──────────────────────────────────────────────────┤
 * │ gyro         │ SE→A │ Données gyroscope SE : deg_s,ts_ms              │
 * ├──────────────┼──────┼──────────────────────────────────────────────────┤
 * │ compass      │ SE→A │ Données boussole SE  : deg,quality (0–100)      │
 * ├──────────────┼──────┼──────────────────────────────────────────────────┤
 * │ mode         │ SP→A │ Preset : idle | normal | surv                   │
 * └──────────────┴──────┴──────────────────────────────────────────────────┘
 *
 * EXEMPLES VPIV :
 *   Configuration complète (3 params min) :
 *     $V:Odom:*:paraOdom:65,200,256#
 *   Configuration avec tous les réglages :
 *     $V:Odom:*:paraOdom:65,200,256,1000,1000,50,200,40#
 *   Lecture position actuelle :
 *     $V:Odom:*:read:pos#
 *   Reset de la pose :
 *     $V:Odom:*:reset:1#
 *   Réception gyro SE (via SP) :
 *     $V:Odom:*:gyro:12,170932#
 *
 * SÉQUENCE D'INITIALISATION RECOMMANDÉE (depuis SP au démarrage) :
 *   $V:Odom:*:act:0#
 *   $V:Odom:*:paraOdom:65,200,256,1000,1000,50,200,40#
 *   $V:Odom:*:reset:1#
 *   $V:Odom:*:act:1#
 *
 * PRÉCAUTIONS :
 *   - Pas de String() → snprintf() uniquement
 *   - Toutes les sorties VPIV numériques sont des entiers (×1000)
 *
 * ARTICULATION :
 *   ← vpiv_dispatch.cpp  routeur central qui appelle Odom::dispatch()
 *   → odom.cpp           fonctions publiques du module odométrie
 *   → config.h           variables de configuration (cfg_odom_*)
 *   → communication.h    sendInfo / sendError
 *
 ************************************************************/
#include "vpiv_dispatch.h"
#include "system/urg.h"
#include "navigation/odom.h"
#include "config/config.h"
#include <string.h>
#include <stdlib.h>
#include "communication/communication.h"

/* ------------------------------------------------------------------ */
/* FONCTIONS UTILITAIRES INTERNES                                      */
/* ------------------------------------------------------------------ */

/**
 * parseInts()
 * Découpe une chaîne CSV en tableau d'entiers.
 *   s   : chaîne source ex: "65,200,256"
 *   out : tableau de sortie
 *   max : nombre maximum de valeurs à lire
 * Retourne le nombre de valeurs effectivement lues.
 */
static int parseInts(const char *s, int *out, int max)
{
    if (!s)
        return 0;
    char buf[128];
    strncpy(buf, s, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    int n = 0;
    char *tok = strtok(buf, ",");
    while (tok && n < max)
    {
        out[n++] = atoi(tok);
        tok = strtok(NULL, ",");
    }
    return n;
}

/**
 * parseDoubles()
 * Comme parseInts() mais pour les doubles (utilisé par "coeff").
 */
static int parseDoubles(const char *s, double *out, int max)
{
    if (!s)
        return 0;
    char buf[128];
    strncpy(buf, s, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    int n = 0;
    char *tok = strtok(buf, ",");
    while (tok && n < max)
    {
        out[n++] = atof(tok);
        tok = strtok(NULL, ",");
    }
    return n;
}

/* ------------------------------------------------------------------ */
/* ROUTEUR PRINCIPAL DU MODULE ODOM                                    */
/* ------------------------------------------------------------------ */

namespace Odom
{
    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop)
            return false;

        /* ============================================================
         * paraOdom — configuration physique complète en une seule trame
         * Format : wheel_mm,track_mm,cpr[,gainG,gainD,fCalc,fPos,fVel]
         * Les 3 premiers sont obligatoires. Les 5 suivants optionnels.
         * Passer 0 pour un paramètre optionnel = garder valeur actuelle.
         *
         * Appelle odom_setPhysical() qui recalcule les coefficients
         * et renvoie une trame de confirmation avec toutes les valeurs.
         * ============================================================ */
        if (strcmp(prop, "paraOdom") == 0)
        {
            if (!value)
            {
                sendError("Odom", "paraOdom", "*", "no_payload");
                return false;
            }

            /* Parsing — on lit jusqu'à 8 entiers */
            int vals[8] = {0, 0, 0,
                           ODO_LEFT_GAIN_X1000_DEFAULT,   /* gainG  */
                           ODO_RIGHT_GAIN_X1000_DEFAULT,  /* gainD  */
                           (int)ODO_FREQ_CALC_MS_DEFAULT, /* fCalc  */
                           (int)ODO_FREQ_POS_MS_DEFAULT,  /* fPos   */
                           (int)ODO_FREQ_VEL_MS_DEFAULT}; /* fVel   */

            int n = parseInts(value, vals, 8);

            /* Les 3 premiers paramètres sont obligatoires */
            if (n < 3 || vals[0] <= 0 || vals[1] <= 0 || vals[2] <= 0)
            {
                sendError("Odom", "paraOdom", "*", "badargs");
                return false;
            }

            /* Appel de odom_setPhysical() avec toutes les valeurs
             * (valeurs par défaut si non fournies)                */
            odom_setPhysical(
                vals[0],                                 /* wheel_mm     */
                vals[1],                                 /* track_mm     */
                vals[2],                                 /* cpr          */
                (n >= 5) ? vals[3] : 0,                  /* gainG  (0=inchangé) */
                (n >= 5) ? vals[4] : 0,                  /* gainD  (0=inchangé) */
                (n >= 6) ? (unsigned long)vals[5] : 0UL, /* fCalc  */
                (n >= 7) ? (unsigned long)vals[6] : 0UL, /* fPos   */
                (n >= 8) ? (unsigned long)vals[7] : 0UL  /* fVel   */
            );
            return true;
        }

        /* ============================================================
         * read — lecture immédiate d'une valeur
         * Valeurs : pos | vel | all
         * ============================================================ */
        if (strcmp(prop, "read") == 0)
        {
            const char *typ = value ? value : "pos";

            if (strcmp(typ, "pos") == 0)
            {
                /* Envoie la position actuelle en entiers ×1000 */
                double x, y, t;
                odom_getPose(&x, &y, &t);
                long x_mm = (long)(x * 1000.0);       /* m  → mm    */
                long y_mm = (long)(y * 1000.0);       /* m  → mm    */
                long theta_mrad = (long)(t * 1000.0); /* rad → mrad */
                char buf[48];
                snprintf(buf, sizeof(buf), "%ld,%ld,%ld", x_mm, y_mm, theta_mrad);
                sendInfo("Odom", "pos", "*", buf);
                return true;
            }
            else if (strcmp(typ, "vel") == 0)
            {
                /* Force l'envoi de la vitesse au prochain cycle
                 * (last_v et last_omega sont des variables statiques
                 *  internes à odom.cpp — on passe par requestReport)   */
                odom_requestReport();
                return true;
            }
            else /* "all" ou autre valeur non reconnue */
            {
                /* Envoie pos + vel ensemble (rapport combiné one-shot) */
                odom_requestReport();
                return true;
            }
        }

        /* ============================================================
         * reset — remet la pose à zéro
         * ============================================================ */
        if (strcmp(prop, "reset") == 0)
        {
            if (value && (strcmp(value, "1") == 0 || strcmp(value, "true") == 0))
            {
                odom_resetPose();
                sendInfo("Odom", "reset", "*", "OK");
                return true;
            }
            return false;
        }

        /* ============================================================
         * act — activer / désactiver le module
         * ============================================================ */
        if (strcmp(prop, "act") == 0)
        {
            bool on = (value && (strcmp(value, "1") == 0 || strcmp(value, "true") == 0));
            odom_setActive(on);
            sendInfo("Odom", "act", "*", on ? "1" : "0");
            return true;
        }

        /* ============================================================
         * freq — période de calcul odométrique (ms)
         * ============================================================ */
        if (strcmp(prop, "freq") == 0)
        {
            unsigned long ms = (unsigned long)atol(value ? value : "0");
            if (ms < 1)
                ms = 1;
            cfg_odom_period_ms = ms;
            char numBuf[12];
            snprintf(numBuf, sizeof(numBuf), "%lu", ms);
            sendInfo("Odom", "freq", "*", numBuf);
            return true;
        }

        /* ============================================================
         * freqPos — période d'envoi automatique de la position (ms)
         * ============================================================ */
        if (strcmp(prop, "freqPos") == 0)
        {
            unsigned long ms = (unsigned long)atol(value ? value : "0");
            cfg_odom_freq_pos_ms = ms; /* 0 = désactive l'envoi auto */
            char numBuf[12];
            snprintf(numBuf, sizeof(numBuf), "%lu", ms);
            sendInfo("Odom", "freqPos", "*", numBuf);
            return true;
        }

        /* ============================================================
         * freqVel — période d'envoi automatique de la vitesse (ms)
         * ============================================================ */
        if (strcmp(prop, "freqVel") == 0)
        {
            unsigned long ms = (unsigned long)atol(value ? value : "0");
            cfg_odom_freq_vel_ms = ms; /* 0 = désactive l'envoi auto */
            char numBuf[12];
            snprintf(numBuf, sizeof(numBuf), "%lu", ms);
            sendInfo("Odom", "freqVel", "*", numBuf);
            return true;
        }

        /* ============================================================
         * report — rapport one-shot combiné pos+vel
         * "0" ou absent → envoi immédiat
         * ============================================================ */
        if (strcmp(prop, "report") == 0)
        {
            odom_requestReport();
            return true;
        }

        /* ============================================================
         * coeff — surcharge directe des 4 coefficients (mode expert)
         * Format : gLong,dLong,gAngl,dAngl  (valeurs flottantes)
         * ⚠ Préférer paraOdom pour la configuration normale
         * ============================================================ */
        if (strcmp(prop, "coeff") == 0)
        {
            double arr[4];
            int n = parseDoubles(value, arr, 4);
            if (n >= 4)
            {
                odom_setCoefficients(arr[0], arr[1], arr[2], arr[3]);
                sendInfo("Odom", "coeff", "*", "OK");
                return true;
            }
            sendError("Odom", "coeff", "*", "badargs");
            return false;
        }

        /* ============================================================
         * gyro — données gyroscope SE transmises vers Arduino
         * Format : deg_s,ts_ms
         *   deg_s  : vitesse angulaire axe Z en degrés/seconde
         *   ts_ms  : horodatage de la mesure (ms côté SE)
         * Source : SE → SP → A (Node-RED relaie les données Gyro)
         * ============================================================ */
        if (strcmp(prop, "gyro") == 0)
        {
            if (!value)
            {
                sendError("Odom", "gyro", "*", "no_payload");
                return false;
            }
            char buf[64];
            strncpy(buf, value, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';

            char *tok = strtok(buf, ",");
            if (!tok)
            {
                sendError("Odom", "gyro", "*", "parse_err");
                return false;
            }
            float g = atof(tok); /* vitesse angulaire (deg/s) */
            tok = strtok(NULL, ",");
            unsigned long ts = tok ? (unsigned long)atol(tok) : 0;

            odom_receiveGyro(g, ts);
            sendInfo("Odom", "gyro", "*", "OK");
            return true;
        }

        /* ============================================================
         * compass — cap boussole SE transmis vers Arduino
         * Format : deg,quality
         *   deg     : cap en degrés (0–360)
         *   quality : fiabilité (0–100)
         * Source : SE → SP → A (Node-RED relaie les données Mag)
         * ============================================================ */
        if (strcmp(prop, "compass") == 0)
        {
            if (!value)
            {
                sendError("Odom", "compass", "*", "no_payload");
                return false;
            }
            char buf[64];
            strncpy(buf, value, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';

            char *tok = strtok(buf, ",");
            if (!tok)
            {
                sendError("Odom", "compass", "*", "parse_err");
                return false;
            }
            float deg = atof(tok);
            tok = strtok(NULL, ",");
            int quality = tok ? atoi(tok) : 0;

            odom_receiveCompass(deg, quality);
            sendInfo("Odom", "compass", "*", "OK");
            return true;
        }

        /* ============================================================
         * mode — preset de configuration rapide
         *   idle   : pause calcul, report lent (1s)
         *   normal : reprend calcul, fréquences par défaut
         *   surv   : surveillance rapprochée (fréquence calcul doublée)
         * ============================================================ */
        if (strcmp(prop, "mode") == 0)
        {
            if (!value)
                return false;

            if (strcmp(value, "idle") == 0)
            {
                odom_setActive(false);
                cfg_odom_freq_pos_ms = 1000;
                cfg_odom_freq_vel_ms = 1000;
            }
            else if (strcmp(value, "normal") == 0)
            {
                odom_setActive(true);
                cfg_odom_period_ms = ODO_FREQ_CALC_MS_DEFAULT;
                cfg_odom_freq_pos_ms = ODO_FREQ_POS_MS_DEFAULT;
                cfg_odom_freq_vel_ms = ODO_FREQ_VEL_MS_DEFAULT;
            }
            else if (strcmp(value, "surv") == 0)
            {
                odom_setActive(true);
                /* Surveillance : calcul et vitesse plus fréquents */
                if (cfg_odom_period_ms > 10)
                    cfg_odom_period_ms = cfg_odom_period_ms / 2;
                cfg_odom_freq_vel_ms = 20; /* 50 Hz */
            }
            sendInfo("Odom", "mode", "*", value);
            return true;
        }

        /* Propriété inconnue  */
        return false;

    } /* dispatch() */

} /* namespace Odom */