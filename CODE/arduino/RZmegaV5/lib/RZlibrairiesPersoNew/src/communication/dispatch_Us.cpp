/************************************************************
 * FICHIER : src/communication/dispatch_Us.cpp
 * ROLE    : Dispatcher VPIV pour US (ultrason)
 ************************************************************/
#include "vpiv_dispatch.h"
#include "../system/urg.h"
#include "../communication/vpiv_utils.h"
#include "../sensors/us.h"
#include "../config/config.h"
#include "../communication/communication.h"

#include <string.h>
#include <stdlib.h>

namespace US {

bool dispatch(const char* prop, const char* inst, const char* value)
{
    if (!prop || !inst) return false;

    int idxs[US_NUM_SENSORS];
    int n = parseIndexList(inst, idxs, US_NUM_SENSORS);
    if (n < 0) return false;

    // -------- read --------
    if (strcmp(prop,"read") == 0) {
        us_handleReadList(idxs, n);
        sendInfo("Us","read",inst,"OK");
        return true;
    }

    // -------- thd --------
    if (strcmp(prop,"thd") == 0) {
        int th = atoi(value);
        if (n == 0)
            for (int i=0;i<US_NUM_SENSORS;i++)
                cfg_us_danger_thresholds[i] = th;
        else
            for (int k=0;k<n;k++)
                cfg_us_danger_thresholds[idxs[k]] = th;

        sendInfo("Us","thd",inst,"OK");
        return true;
    }

    // -------- freq --------
    if (strcmp(prop,"freq") == 0) {
        cfg_us_ping_cycle_ms = max(5, atoi(value));
        sendInfo("Us","freq",inst,"OK");
        return true;
    }

    // -------- act --------
    if (strcmp(prop,"act") == 0) {
        cfg_us_active = (strcmp(value,"1")==0 || strcmp(value,"true")==0);
        sendInfo("Us","act",inst,"OK");
        return true;
    }

    // -------- sendall --------
    if (strcmp(prop,"sendall") == 0) {
        cfg_us_send_all = (strcmp(value,"1")==0 || strcmp(value,"true")==0);
        sendInfo("Us","sendall",inst,"OK");
        return true;
    }

    // -------- delta --------
    if (strcmp(prop,"delta") == 0) {
        cfg_us_delta_threshold = atoi(value);
        sendInfo("Us","delta",inst,"OK");
        return true;
    }

    // -------- suspect_ms --------
    if (strcmp(prop,"suspect_ms") == 0) {
        cfg_us_suspect_timeout_ms = atoi(value);
        sendInfo("Us","suspect_ms",inst,"OK");
        return true;
    }

    // -------- mode --------
    if (strcmp(prop,"mode") == 0) {

        if (strcmp(value,"idle")==0) {
            cfg_us_active = false;
        }
        else if (strcmp(value,"normal")==0) {
            cfg_us_active = true;
            cfg_us_ping_cycle_ms = US_PING_CYCLE_MS_DEFAULT;
        }
        else if (strcmp(value,"surv")==0) {
            cfg_us_active = true;
            cfg_us_ping_cycle_ms = max(10UL, US_PING_CYCLE_MS_DEFAULT/2);
        }

        sendInfo("Us","mode",inst,"OK");
        return true;
    }

    return false;
}

} // namespace US
