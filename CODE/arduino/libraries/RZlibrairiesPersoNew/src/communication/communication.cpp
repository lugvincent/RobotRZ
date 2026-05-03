// =======// ======================================================================
// FICHIER : src/communication/communication.cpp
// ROLE    : Implémentation du parser VPIV, lecture série, envoi VPIV
// Chemin  : src/communication/communication.cpp
//
// Respect strict du format : $<CAT>:<MODULE>:<PROP>:<INST>:<VALUE>#
// ---------------------------------------------------------------------

#include "communication.h"
#include "../system/urg.h"   // urg_isActive(), urg_handle(), urg_clear(), urg_sendAlert()
#include <string.h>
#include <stdlib.h>

// Buffer de réception (sans $ ni #)
static char rxBuffer[VPIV_MAX_MSG_LEN];
static size_t rxIndex = 0;
static bool receiving = false;

// ----------------------------------------------------------------------
// nextToken : extrait un token séparé par ':' depuis src[pos]
// ----------------------------------------------------------------------
bool nextToken(const char* src, char* dest, size_t maxlen, size_t &pos) {
    if (!src || !dest) return false;
    size_t di = 0;

    // si on est déjà en fin
    if (src[pos] == '\0') {
        dest[0] = '\0';
        return false;
    }

    // copier jusqu'à ':' ou fin ou maxlen-1
    while (src[pos] && src[pos] != ':' && di < maxlen - 1) {
        dest[di++] = src[pos++];
    }
    dest[di] = '\0';

    // sauter ':' si présent
    if (src[pos] == ':') pos++;

    return (di > 0);
}

// ----------------------------------------------------------------------
// parseVPIV : parse la ligne (sans $ ni #) dans MessageVPIV
// ----------------------------------------------------------------------
bool parseVPIV(const char* raw, MessageVPIV &out) {
    if (!raw) return false;
    size_t pos = 0;
    char buf[VPIV_MAX_TOKEN];

    // 1) type (cat) : premier token (attendu 1 caractère)
    if (!nextToken(raw, buf, sizeof(buf), pos)) return false;
    out.type = buf[0];

    // 2) module
    if (!nextToken(raw, out.module, sizeof(out.module), pos)) return false;

    // 3) prop
    if (!nextToken(raw, out.prop, sizeof(out.prop), pos)) return false;

    // 4) inst
    if (!nextToken(raw, out.inst, sizeof(out.inst), pos)) return false;

    // 5) value : copie brute du reste (peut contenir : , [ ])
    size_t vi = 0;
    while (raw[pos] && vi < sizeof(out.value) - 1) {
        out.value[vi++] = raw[pos++];
    }
    out.value[vi] = '\0';

    return true;
}

// ----------------------------------------------------------------------
// communication_init()
// ----------------------------------------------------------------------
void communication_init() {
    rxIndex = 0;
    receiving = false;
}

// ----------------------------------------------------------------------
// sendVPIV : construction canonique et envoi (utilise MAIN_SERIAL)
// Format : $<CAT>:<MODULE>:<PROP>:<INST>:<VALUE>#
// NOTE : n'utilise pas Serial direct afin d'être compatible MAIN_SERIAL macro
// ----------------------------------------------------------------------
void sendVPIV(char type,
              const char* module,
              const char* prop,
              const char* inst,
              const char* value)
{
    // Défensive : remplacer NULL par chaîne vide
    const char* m = module ? module : "";
    const char* p = prop ? prop : "";
    const char* i = inst ? inst : "";
    const char* v = value ? value : "";

    // Construction simple
    MAIN_SERIAL.print('$');
    MAIN_SERIAL.print(type);
    MAIN_SERIAL.print(':');
    MAIN_SERIAL.print(m);
    MAIN_SERIAL.print(':');
    MAIN_SERIAL.print(p);
    MAIN_SERIAL.print(':');
    MAIN_SERIAL.print(i);
    MAIN_SERIAL.print(':');
    MAIN_SERIAL.print(v);
    MAIN_SERIAL.print('#');
    MAIN_SERIAL.print('\n'); // facilite debugging côté host/Node-RED
}

// ----------------------------------------------------------------------
// communication_processInput()
// ----------------------------------------------------------------------
void communication_processInput() {
    // Lire tout ce qui est disponible
    while (MAIN_SERIAL.available()) {
        char c = (char) MAIN_SERIAL.read();
        // Ignorer CR/LF et caractères non imprimables
        if (c == '\r' || c == '\n') continue;
        if ((unsigned char)c < 32) continue;   // CTRL ASCII

        // Ignorer espaces parasites hors message
        if (!receiving && (c == ' ')) continue;
        
        // Début de trame
        if (c == '$') {
            receiving = true;
            rxIndex = 0;
            continue;
        }

        // Fin de trame
        if (c == '#' && receiving) {
            // Terminer la chaîne (débordement protégé)
            if (rxIndex >= sizeof(rxBuffer)) rxIndex = sizeof(rxBuffer) - 1;
            rxBuffer[rxIndex] = '\0';

            MessageVPIV msg;
            if (parseVPIV(rxBuffer, msg)) {
                // Si URG actif, n'accepter que les commandes de remise en état
                if (urg_isActive()) {
                    //  - Urg:clear:*:...   (clear via module Urg direct)
                    bool allowClear = false;

                    if (strcmp(msg.module, "Urg") == 0 && strcmp(msg.prop, "clear") == 0) {
                        allowClear = true;
                    }

                    if (allowClear) {
                        dispatchVPIV(msg.module, msg.inst, msg.prop, msg.value);
                    } else {
                        // notifier en erreur (message synthétique)
                        sendError("Urg", "lock", "*", "STOP actif");
                    }
                } else {
                    // Dispatch standard : on suit la signature dispatchVPIV(module, inst, prop, value)
                    dispatchVPIV(msg.module, msg.inst, msg.prop, msg.value);
                }
            } else {
                // parsing invalide
                sendError("COMM", "parse", "*", "Format invalide");
            }

            // reset réception
            receiving = false;
            rxIndex = 0;
            continue;
        }

        // Enregistrement si en train de recevoir (et capacité disponible)
        if (receiving) {
            if (rxIndex < (sizeof(rxBuffer) - 1)) {
                rxBuffer[rxIndex++] = c;
            } else {
                // overflow buffer -> état d'urgence (URG_BUFFER_OVERFLOW)
                receiving = false;
                rxIndex = 0;
                urg_handle(URG_BUFFER_OVERFLOW);
                sendError("COMM", "buf", "*", "Overflow");
            }
        }
    }
}
