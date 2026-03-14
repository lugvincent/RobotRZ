// ======================================================================
// FICHIER : src/communication/communication.h
// ROLE    : Interface centralisée pour le parser VPIV / envoi VPIV.
// Chemin  : src/communication/communication.h
//
// Format VPIV canonique (validé) :
//    $<CAT>:<MODULE>:<PROP>:<INST>:<VALUE>#
// Exemple : $I:Mic:rms:0:512#
// ----------------------------------------------------------------------
// Ce fichier expose :
//  - initialisation / boucle : communication_init(), communication_processInput()
//  - parse basique : parseVPIV(const char*, MessageVPIV&)
//  - envoi unifié : sendVPIV(type, module, prop, inst, value)
//  - helpers : sendCmd/sendInfo/sendError/sendFlux (+ String overloads)
// ---------------------------------------------------------------------
#ifndef SRC_COMMUNICATION_COMMUNICATION_H
#define SRC_COMMUNICATION_COMMUNICATION_H

#include <Arduino.h>
#include "config/config.h" // MAIN_SERIAL, MAIN_BAUD, VPIV_MAX_MSG_LEN (si défini)
#include "vpiv_utils.h"
#include "vpiv_dispatch.h"

// Valeurs par défaut si non définies dans config.h
#ifndef VPIV_MAX_MSG_LEN
#define VPIV_MAX_MSG_LEN 128
#endif
#ifndef VPIV_MAX_TOKEN
#define VPIV_MAX_TOKEN 64
#endif

// Représentation parsée d'un message VPIV (raw sans $ et #)
//
// ⚠️  CONVENTION — à lire avant toute modification
// ---------------------------------------------------------------
// Le format VPIV réel sur la liaison série est :
//   $<CAT>:<MODULE>:<INST>:<PROP>:<VALUE>#
//   ex : $V:Mic:*:modeMicsF:MOY#
//        → MODULE=Mic, INST=*, PROP=modeMicsF, VALUE=MOY
//
// parseVPIV() remplit les champs dans l'ordre d'arrivée (token 3 et 4) :
//   out.prop  ← INST (token 3)  ⚠️ contient l'INSTANCE, pas la propriété
//   out.inst  ← PROP (token 4)  ⚠️ contient la PROPRIÉTÉ, pas l'instance
//
// Cette inversion est compensée dans l'appel au routeur :
//   dispatchVPIV(msg.module, msg.inst, msg.prop, msg.value)
// Les dispatchers reçoivent bien (prop=PROP, inst=INST).
// ---------------------------------------------------------------
typedef struct
{
    char type;                    // 'V','I','F','E'...
    char module[VPIV_MAX_TOKEN];  // ex "Mic", "Us", "CfgS"
    char prop[VPIV_MAX_TOKEN];    // ⚠️ contient l'INST de la trame (ex "*", "0")
    char inst[VPIV_MAX_TOKEN];    // ⚠️ contient la PROP de la trame (ex "modeMicsF")
    char value[VPIV_MAX_MSG_LEN]; // le reste (peut contenir des ',','[]' etc.)
} MessageVPIV;

// --------------------------- API publique ------------------------------
/**
 * Initialise les structures locales (buffers) pour la communication.
 * N'initalise PAS les Serial; initConfig() doit appeler MAIN_SERIAL.begin(...)
 */
void communication_init();

/**
 * Doit être appelé fréquemment dans loop().
 * Lit MAIN_SERIAL et traite les trames $...# :
 *  - reconstitue la trame
 *  - parseVPIV()
 *  - effectue dispatch via dispatchVPIV(...)
 *
 * Filtrage urgence : si isEmergencyActive() alors seules les trames
 * SCfg:emg:clear (ou celles explicitement autorisées) sont acceptées.
 */
void communication_processInput();

/**
 * parseVPIV(raw, out)
 *   - raw : chaîne C (sans $ ni #)
 *   - out : structure MessageVPIV remplie
 * Retourne true si parsing ok, false si format invalide.
 */
bool parseVPIV(const char *raw, MessageVPIV &out);

/**
 * sendVPIV(type, module, prop, inst, value)
 *   - construit et envoie via MAIN_SERIAL la trame VPIV complète
 *   - type : 'V','I','E','F' ...
 *   - tous les arguments doivent être C-strings non null (mais peuvent être "")
 */
void sendVPIV(char type,
              const char *module,
              const char *prop,
              const char *inst,
              const char *value);

// Helpers pratiques (appellent sendVPIV)
inline void sendCmd(const char *module, const char *prop, const char *inst, const char *value)
{
    sendVPIV('V', module, prop, inst, value);
}
inline void sendInfo(const char *module, const char *prop, const char *inst, const char *value)
{
    sendVPIV('I', module, prop, inst, value);
}
inline void sendError(const char *module, const char *prop, const char *inst, const char *value)
{
    sendVPIV('E', module, prop, inst, value);
}
inline void sendFlux(const char *module, const char *prop, const char *inst, const char *value)
{
    sendVPIV('F', module, prop, inst, value);
}

// String overloads
inline void sendCmdS(const String &module, const String &prop, const String &inst, const String &value)
{
    sendCmd(module.c_str(), prop.c_str(), inst.c_str(), value.c_str());
}
inline void sendInfoS(const String &module, const String &prop, const String &inst, const String &value)
{
    sendInfo(module.c_str(), prop.c_str(), inst.c_str(), value.c_str());
}
inline void sendErrorS(const String &module, const String &prop, const String &inst, const String &value)
{
    sendError(module.c_str(), prop.c_str(), inst.c_str(), value.c_str());
}
inline void sendFluxS(const String &module, const String &prop, const String &inst, const String &value)
{
    sendFlux(module.c_str(), prop.c_str(), inst.c_str(), value.c_str());
}

// --------------------------- utilitaires internes exportés --------------
/**
 * nextToken(src, dest, maxlen, pos)
 *   - extrait le token courant depuis src[pos] jusqu'au prochain ':' ou fin
 *   - copie dans dest (NUL term)
 *   - avance pos au char suivant le séparateur
 *   - retourne true si token non vide
 */
bool nextToken(const char *src, char *dest, size_t maxlen, size_t &pos);

#endif // SRC_COMMUNICATION_COMMUNICATION_H