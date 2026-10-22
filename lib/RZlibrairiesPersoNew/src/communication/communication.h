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
// Le format VPIV sur la liaison série est :
//   $<CAT>:<MODULE>:<PROP>:<INST>:<VALUE>#
//   ex : $V:Mic:modeMicsF:*:MOY#
//        → MODULE=Mic, PROP=modeMicsF, INST=*, VALUE=MOY
//
//   PROP en position 3, INST en position 4. Source de vérité : la Table A
//   (docs/tables_RZ_descripteur.md §5). Confirmé côté SE par mqtt_bridge.py,
//   qui transmet la trame telle quelle, et côté émission par sendVPIV().
//
// parseVPIV() remplit donc les champs sans aucune permutation :
//   out.prop  ← token 3 = PROP
//   out.inst  ← token 4 = INST
//
// ⚠️ CORRIGÉ LE 22/10/2026 — ne pas rétablir l'ancienne version.
//   Ce bloc affirmait que le fil portait INST avant PROP, et que les champs
//   de cette structure contenaient l'inverse de leur nom. communication.cpp
//   « compensait » en permutant les arguments à l'appel de dispatchVPIV().
//   Le fil ayant changé de convention et pas la réception, cette
//   compensation INTRODUISAIT l'erreur : plus aucune commande entrante
//   n'était reconnue, et l'urgence devenait ineffaçable.
//   Démontré sur matériel : à $V:Urg:clear:*:1# la carte répondait
//   $E:Urg:*:clear:urgence active# — les champs permutés, visibles dans sa
//   propre sortie. Détail en §9.5 de docs/synthese_etat.md.
// ---------------------------------------------------------------
typedef struct
{
    char type;                    // 'V','I','F','E','A'
    char module[VPIV_MAX_TOKEN];  // token 2 — ex "Mic", "US", "CfgS"
    char prop[VPIV_MAX_TOKEN];    // token 3 — la PROPRIÉTÉ (ex "modeMicsF")
    char inst[VPIV_MAX_TOKEN];    // token 4 — l'INSTANCE   (ex "*", "0", "TGD")
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