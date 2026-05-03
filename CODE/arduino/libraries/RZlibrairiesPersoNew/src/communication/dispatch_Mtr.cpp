/************************************************************
 * FICHIER : src/communication/dispatch_Mtr.cpp
 * ROLE    : Dispatcher VPIV pour Mtr (moteurs)- support legacy & modern
 *
 * Accepté :
 *  - nouvelle propriété "lr" : valeur "L,R,A" envoyée directement
 *  - ancienne propriété "triplet" : "acc,spd,dir" → converti puis envoi
 *  - acc/spd/dir séparés : si reçus, convertit et envoie
 ************************************************************/

#include "vpiv_dispatch.h"
#include "../actuators/mtr.h"
#include <string.h>
#include <stdlib.h>
#include "../communication/communication.h"
#include "../configuration/config.h"

namespace MtrDispatch {

static bool instIsAll_local(const char* inst) {
    return (inst && inst[0]=='*' && inst[1]=='\0');
}

bool dispatch(const char* prop, const char* inst, const char* value) {
    if (!prop) return false;

    // n'accepte que "*" en instance
    if (!instIsAll_local(inst)) {
        sendError("Mtr","inst", inst?inst:"?","Only '*' supported");
        return false;
    }

    // --- legacy properties (compatibilité) ---
    if (strcmp(prop,"acc")==0) {
        int v = atoi(value);
        mtr_setAcceleration(v);
        sendInfo("Mtr","acc","*", "OK");
        return true;
    }
    if (strcmp(prop,"spd")==0) {
        int v = atoi(value);
        mtr_setSpeed(v);
        sendInfo("Mtr","spd","*","OK");
        return true;
    }
    if (strcmp(prop,"dir")==0) {
        int v = atoi(value);
        mtr_setDirection(v);
        sendInfo("Mtr","dir","*","OK");
        return true;
    }

    if (strcmp(prop,"act")==0) {
        bool on = (value && (strcmp(value,"1")==0 || strcmp(value,"true")==0));
        cfg_mtr_active = on;
        sendInfo("Mtr","act","*", on? "1":"0");
        return true;
    }

    // --- modern properties ---
    // cmd: "<v,omega,acc>"  v = -100..100, omega = -100..100, acc = 0..4
    if (strcmp(prop,"cmd")==0) {
        if (!value) { sendError("Mtr","cmd","*","no_payload"); return false; }
        int v=0, w=0, a=1;
        sscanf(value, "%d,%d,%d", &v, &w, &a);
        mtr_setTargetsSigned(v, w, a);
        sendInfo("Mtr","cmd","*", "OK");
        return true;
    }

    // mode: modern|legacy
    if (strcmp(prop,"mode")==0) {
        if (!value) return false;
        if (strcmp(value,"modern")==0) { mtr_setModeModern(true); sendInfo("Mtr","mode","*","modern"); return true; }
        if (strcmp(value,"legacy")==0) { mtr_setModeModern(false); sendInfo("Mtr","mode","*","legacy"); return true; }
        sendError("Mtr","mode","*","unknown");
        return false;
    }

    // kturn: float param
    if (strcmp(prop,"kturn")==0) {
        cfg_mtr_kturn = atof(value);
        sendInfo("Mtr","kturn","*", String(cfg_mtr_kturn).c_str());
        return true;
    }

    // scale: inputScale,outputScale  ex "100,400"
    if (strcmp(prop,"scale")==0) {
        char buf[64];
        strncpy(buf, value?value:"", sizeof(buf)-1); buf[sizeof(buf)-1]=0;
        char* tok = strtok(buf,",");
        if (!tok) { sendError("Mtr","scale","*","parse"); return false; }
        cfg_mtr_inputScale = atoi(tok);
        tok = strtok(NULL,",");
        if (tok) cfg_mtr_outputScale = atoi(tok);
        sendInfo("Mtr","scale","*", "OK");
        return true;
    }

    // read : renvoie état courant
    if (strcmp(prop,"read")==0) {
        char b[64];
        snprintf(b,sizeof(b), "%d,%d,%d,%s", cfg_mtr_targetL, cfg_mtr_targetR, cfg_mtr_targetA, cfg_mtr_mode_modern?"modern":"legacy");
        sendInfo("Mtr","state","*", b);
        return true;
    }

    // triplet legacy compat (acc,spd,dir)
    if (strcmp(prop,"triplet")==0) {
        int a=0,s=0,d=0;
        sscanf(value,"%d,%d,%d",&a,&s,&d);
        mtr_setTriplet(a,s,d);
        sendInfo("Mtr","triplet","*","OK");
        return true;
    }

    return false;
}

} // namespace MtrDispatch
