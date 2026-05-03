/************************************************************
 * FICHIER : src/communication/dispatch_Urg.cpp
 * ROLE    : Dispatcher VPIV pour URG (urgence)
 ************************************************************/

#include "vpiv_dispatch.h"
#include "../system/urg.h"
#include "../communication/communication.h"
#include <string.h>
#include <stdlib.h>

namespace UrgDispatch {

/**
 *   PROP : "set"   value = "<code>"
 *   PROP : "clear"
 *   PROP : "status"
 */
bool dispatch(const char* prop, const char* inst, const char* value) {

    if (!prop) return false;

    // --------------------------------------
    // set → urg_handle(<code>)
    // --------------------------------------
    if (strcmp(prop, "set") == 0) {
        int code = atoi(value);
        urg_handle((uint8_t)code);
        return true;
    }

    // --------------------------------------
    // clear → urg_clear()
    // --------------------------------------
    if (strcmp(prop, "clear") == 0) {
        urg_clear();
        return true;
    }

    // --------------------------------------
    // status → renvoi immédiat VPIV
    // $I:Urg:status:*:<active>:<code>#
    // --------------------------------------
    if (strcmp(prop, "status") == 0) {

        char buf[32];
        snprintf(buf, sizeof(buf),
                 "%d:%d",
                 urg_isActive()?1:0,
                 urg_getCode());

        sendInfo("Urg", "status", "*", buf);
        return true;
    }

    return false;
}

} // namespace UrgDispatch
