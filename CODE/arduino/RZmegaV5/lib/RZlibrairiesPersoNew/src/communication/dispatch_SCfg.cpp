// ======================================================================
// FICHIER : src/communication/dispatch_SCfg.cpp
// Objet   : Dispatcher VPIV pour le module SCfg (system config / control)
// Rôle    : traiter les commandes de configuration système envoyées via VPIV
//           Exemples supportés : changer de mode, reset soft/hard, gestion EMG.
// Auteur  : généré pour ton projet (commentaires détaillés en français)
// ======================================================================

#include "vpiv_dispatch.h"
#include "../system/urg.h"
#include <string.h>
#include <stdlib.h>

#include "../system/config.h"       // modeRzB, initConfig(), etc.
#include "../system/emergency.h"    // emergencyStop, emergencyCode, handleEmergency()
#include "../communication/communication.h" // sendVPIV(), sendError()

// NOTE : chemins d'inclusion à adapter si ton arborescence est différente.
// Ce fichier suppose : src/communication/dispatch_SCfg.cpp
//                         ../system/config.h
//                         ../system/emergency.h
//                         ../communication/communication.h

namespace SCfg {

static void sendAck(const char* prop, const char* inst, const char* msg) {
    // Réponse positive standard : type I (Information)
    // Exemple envoyé : $I:SCfg:<prop>:<inst>:<msg>#
    sendVPIV('I', "SCfg", prop, inst, msg ? msg : "OK");
}

static void sendNack(const char* prop, const char* inst, const char* err) {
    // Réponse d'erreur : type E (Erreur) via sendVPIV en gardant format simple
    sendVPIV('E', "SCfg", prop, inst, err ? err : "ERR");
}

// Convertit une chaîne en booléen simple (0/1, "true"/"false")
static bool strToBool(const char* s) {
    if (!s) return false;
    if (s[0] == '1') return true;
    if (strcmp(s, "true") == 0) return true;
    if (strcmp(s, "True") == 0) return true;
    return false;
}

// Dispatch principal pour SCfg
// propAbbr : propriété reçue (par ex. "mode", "reset", "emg")
// inst     : instance (généralement "*" ou identifiant non utilisé ici)
// value    : valeur / payload
bool dispatch(const char* propAbbr, const char* inst, const char* value) {
    // Sécurité : valider les pointeurs
    if (!propAbbr) return false;
    const char* prop = propAbbr;
    const char* instance = inst ? inst : "*";
    const char* val = value ? value : "";

    // ---------------------------
    // 1) Changer le mode opérationnel
    //    Commande attendue : prop="mode" value="0|1|2|3|4|5" ou "ARRET", "VEILLE", ...
    // ---------------------------
    if (strcmp(prop, "mode") == 0) {
        // essayer d'interpréter comme entier
        int newMode = atoi(val);
        if (newMode >= 0 && newMode <= 5) {
            modeRzB = (byte)newMode;
            // envoyer ack et info
            char info[32];
            snprintf(info, sizeof(info), "mode:%d", newMode);
            sendAck("mode", instance, info);
            return true;
        } else {
            // accepter quelques mots-clé courants (flexible)
            if (strcasecmp(val, "ARRET") == 0 || strcasecmp(val, "stop") == 0) {
                modeRzB = 0;
                sendAck("mode", instance, "mode:ARRET");
                return true;
            } else if (strcasecmp(val, "VEILLE") == 0) {
                modeRzB = 1; sendAck("mode", instance, "mode:VEILLE"); return true;
            } else if (strcasecmp(val, "SURVEILLANCE") == 0) {
                modeRzB = 2; sendAck("mode", instance, "mode:SURVEILLANCE"); return true;
            } else if (strcasecmp(val, "AUTONOME") == 0) {
                modeRzB = 3; sendAck("mode", instance, "mode:AUTONOME"); return true;
            } else if (strcasecmp(val, "SUIVEUR") == 0) {
                modeRzB = 4; sendAck("mode", instance, "mode:SUIVEUR"); return true;
            } else if (strcasecmp(val, "TELECOMMANDE") == 0) {
                modeRzB = 5; sendAck("mode", instance, "mode:TELECOMMANDE"); return true;
            }
        }
        sendNack("mode", instance, "valeur invalide");
        return true; // on a traité la requête mais elle est invalide
    }

    // ---------------------------
    // 2) RESET système
    //    prop="reset" val="soft" | "hard" | "factory"
    //    - soft : reinit logique (initConfig)
    //    - hard : soft + réinitialisation plus large (si disponible)
    // ---------------------------
    if (strcmp(prop, "reset") == 0) {
        if (strcasecmp(val, "soft") == 0 || strcmp(val, "1") == 0) {
            initConfig();   // reset des paramètres à valeur par défaut
            initEmergency(); // clear emergency state as well
            sendAck("reset", instance, "soft");
            return true;
        } else if (strcasecmp(val, "hard") == 0 || strcasecmp(val, "factory") == 0) {
            // Hard = soft + éventuelles actions supplémentaires (écrire flags, re-init deeper)
            initConfig();
            initEmergency();
            // TODO : ajouter sauvegarde/fichier usine si nécessaire
            sendAck("reset", instance, "hard");
            return true;
        } else {
            sendNack("reset", instance, "valeur invalide (soft/hard)");
            return true;
        }
    }

    // ---------------------------
    // 3) URGENCE (URG)
    //    prop="urg" val="clear" | "code" (numérique) | "stop"
    //    - "clear" : efface l'état d'urgence (autorise reprise)
    //    - numeric : déclenche handleEmergency(code)
    // ---------------------------
   if (strcmp(prop, "urg") == 0 || strcmp(prop, "URG") == 0) {

    if (strcasecmp(val, "clear") == 0) {
        urg_clear();
        sendAck("urg", instance, "clear");
        return true;
    }

    if (strcasecmp(val, "stop") == 0 || strcmp(val, "1") == 0) {
        urg_handle(URG_MOTOR_STALL);
        sendAck("urg", instance, "stop");
        return true;
    }

    int code = atoi(val);
    urg_handle((uint8_t)code);

    char info[32];
    snprintf(info, sizeof(info), "code:%d", code);
    sendAck("urg", instance, info);
    return true;
}

    // ---------------------------
    // 4) Dump / status (lecture)
    //    prop="status" -> renvoie un résumé (mode, emergency)
    // ---------------------------
    if (strcmp(prop, "status") == 0) {
        char out[128];
        snprintf(out, sizeof(out), "mode:%d,emg:%d,stop:%d", (int)modeRzB, (int)emergencyCode, (emergencyStop?1:0));
        sendVPIV('I', "SCfg", "status", instance, out);
        return true;
    }

    // ---------------------------
    // Propriété non gérée -> renvoyer false pour laisser d'autres dispatchers tenter
    // ---------------------------
    return false;
}

} // namespace SCfg

// Exporter symboliquement (optionnel selon compilation) :
// bool SCfg::dispatch(const char*, const char*, const char*);  // déjà défini
