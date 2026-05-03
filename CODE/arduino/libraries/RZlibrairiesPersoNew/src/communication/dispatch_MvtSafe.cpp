/************************************************************
 * FICHIER : src/communication/dispatch_MvtSafe.cpp
 ************************************************************/
#include "vpiv_dispatch.h"
#include "../communication/communication.h"
#include "../safety/mvt_safe.h"
#include <string.h>
#include <stdlib.h>

namespace MvtSafeDispatch {

bool dispatch(const char* prop, const char* inst, const char* value) {
    if (!prop) return false;

    // act : on/off
    if (strcmp(prop,"act")==0) {
        bool on = (value && (strcmp(value,"1")==0 || strcmp(value,"true")==0));
        mvtsafe_setActive(on);
        sendInfo("mvtSafe","act","*",on?"1":"0");
        return true;
    }

    // mode : soft/hard/idle
    if (strcmp(prop,"mode")==0) {
        mvtsafe_setMode(value);
        sendInfo("mvtSafe","mode","*",value);
        return true;
    }

    // cfg seuils : "warn,danger"
    if (strcmp(prop,"thd")==0) {
        int w=0,d=0;
        sscanf(value,"%d,%d",&w,&d);
        mvtsafe_setThresholds(w,d);
        sendInfo("mvtSafe","thd","*",value);
        return true;
    }

    // max speed scale
    if (strcmp(prop,"scale")==0) {
        float s = atof(value);
        mvtsafe_setMaxScale(s);
        sendInfo("mvtSafe","scale","*",value);
        return true;
    }

    // status
    if (strcmp(prop,"status")==0) {
        char buf[16];
        snprintf(buf,sizeof(buf),"%d",mvtsafe_getState());
        sendInfo("mvtSafe","status","*",buf);
        return true;
    }

    return false;
}

}
