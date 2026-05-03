/************************************************************
 * FICHIER : src/communication/dispatch_Odom.cpp
 * ROLE    : Dispatcher VPIV pour le module Odométrie (Odo)
 *
 * Propriétés supportées (propAbbr) :
 *   - "read"   : read type (pos|vel|raw) -> $V:Odo:*:read:pos#
 *   - "reset"  : reset pose -> value "1"
 *   - "act"    : 1/0 activer ou désactiver odom
 *   - "freq"   : période calcul ODOM_period_ms (ms)
 *   - "report" : période d'envoi (ms) ou "1" pour activer default
 *   - "coeff"  : coeffGLong,coeffDLong,gAngl,dAngl  (valeurs séparées par ,)
 *   - "gyro"   : "<deg/s>,<timestamp_ms>"  (valeur fournie par SE via Tasker)
 *   - "compass": "<deg>,<quality>"  (SE boussole)
 *   - "mode"   : idle|normal|surv (mapping interne)
 *
 * Exemples :
 *   $V:Odo:*:read:pos#
 *   $V:Odo:*:reset:1#
 *   $V:Odo:*:coeff:0.0712,0.0712,0.08035,0.08035#
 *   $V:Odo:*:gyro:12.4,170932#
 ************************************************************/
#include "vpiv_dispatch.h"
#include "../system/urg.h"
#include "../sensors/odom.h"
#include "../configuration/config.h"
#include <string.h>
#include <stdlib.h>
#include "../communication/communication.h"

// helper: split CSV into array of doubles (simple)
static int parseDoubles(const char* s, double* out, int max) {
    if (!s) return 0;
    char buf[256];
    strncpy(buf, s, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    int n = 0;
    char* tok = strtok(buf, ",");
    while (tok && n < max) {
        out[n++] = atof(tok);
        tok = strtok(NULL, ",");
    }
    return n;
}

// parse two int values from "deg,quality"
static bool parseCompass(const char* s, float* deg, int* quality) {
    if (!s) return false;
    char buf[128];
    strncpy(buf, s, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';
    char* tok = strtok(buf, ",");
    if (!tok) return false;
    *deg = atof(tok);
    tok = strtok(NULL, ",");
    if (!tok) { *quality = 0; return true; }
    *quality = atoi(tok);
    return true;
}

namespace OdoDispatch {

bool dispatch(const char* propAbbr, const char* inst, const char* value) {
    if (!propAbbr) return false;

    // read
    if (strcmp(propAbbr, "read") == 0) {
        const char* typ = value ? value : "pos";
        if (strcmp(typ, "pos") == 0) {
            double x,y,t;
            odom_getPose(&x,&y,&t);
            char buf[128];
            snprintf(buf, sizeof(buf), "%.3f,%.3f,%.2f", x, y, t * 180.0 / M_PI);
            sendInfo("Odo", "pos", "*", buf);
            return true;
        } else if (strcmp(typ,"vel")==0) {
            odom_requestReport(); // le module enverra "vel" au prochain cycle
            sendInfo("Odo", "req", "vel", "OK");
            return true;
        } else {
            odom_requestReport();
            sendInfo("Odo", "req", "pos", "OK");
            return true;
        }
    }

    // reset
    if (strcmp(propAbbr,"reset")==0) {
        if (value && (strcmp(value,"1")==0 || strcmp(value,"true")==0)) {
            odom_resetPose();
            sendInfo("Odo", "cmd", "reset", "ok");
            return true;
        }
        return false;
    }

    // act
    if (strcmp(propAbbr,"act")==0) {
        bool on = (value && (strcmp(value,"1")==0 || strcmp(value,"true")==0));
        odom_setActive(on);
        sendInfo("Odo", "act", "*", on ? "1" : "0");
        return true;
    }

    // freq
    if (strcmp(propAbbr,"freq")==0) {
        unsigned long ms = (unsigned long)atoi(value);
        if (ms < 1) ms = 1;
        cfg_odom_period_ms = ms;
        sendInfo("Odo", "freq", "*", String(ms).c_str());
        return true;
    }

    // report
    if (strcmp(propAbbr,"report")==0) {
        unsigned long ms = (unsigned long)atoi(value);
        if (ms == 0) {
            // immediate one-shot report
            odom_requestReport();
            sendInfo("Odo", "report", "once", "OK");
            return true;
        } else {
            cfg_odom_report_period_ms = ms;
            sendInfo("Odo", "report", "*", String(ms).c_str());
            return true;
        }
    }

    // coeff
    if (strcmp(propAbbr,"coeff")==0) {
        double arr[4];
        int n = parseDoubles(value, arr, 4);
        if (n >= 4) {
            odom_setCoefficients(arr[0], arr[1], arr[2], arr[3]);
            sendInfo("Odo", "cmd", "coeff", "ok");
            return true;
        }
        sendError("Odo", "cmd", "coeff", "badargs");
        return false;
    }

    // gyro
    if (strcmp(propAbbr,"gyro")==0) {
        if (!value) { sendError("Odo","gyro","*","no_payload"); return false; }
        char buf[64];
        strncpy(buf, value, sizeof(buf)-1);
        buf[sizeof(buf)-1] = '\0';
        char* tok = strtok(buf, ",");
        if (!tok) { sendError("Odo","gyro","*","parse_err"); return false; }
        float g = atof(tok);
        tok = strtok(NULL, ",");
        unsigned long ts = 0;
        if (tok) ts = (unsigned long)atol(tok);
        odom_receiveGyro(g, ts);
        sendInfo("Odo","gyro","*","OK");
        return true;
    }
    // geom : "<diam_mm>,<entraxe_mm>"
    if (strcmp(propAbbr, "geom") == 0) {
        if (!value) {
            sendError("Odo","geom","*","no_payload");
            return false;
        }

        double vals[2];
        int n = parseDoubles(value, vals, 2);
        if (n < 2) {
            sendError("Odo","geom","*","badargs");
            return false;
        }

        double diam_mm = vals[0];
        double entraxe_mm = vals[1];

        if (diam_mm <= 0 || entraxe_mm <= 0) {
            sendError("Odo","geom","*","invalid");
            return false;
        }

        // recalcul coefficients
        double radius_m = (diam_mm / 1000.0) / 2.0;
        double tick_m = (2.0 * M_PI * radius_m) / 64.0;  // 64 CPR
        double wheelbase_m = entraxe_mm / 1000.0;

        double longCoeff  = tick_m;
        double angCoeff   = tick_m / wheelbase_m;

        odom_setCoefficients(longCoeff, longCoeff, -angCoeff, angCoeff);

        sendInfo("Odo","geom","*", "OK");
        return true;
    }

    // compass
    if (strcmp(propAbbr,"compass")==0) {
        float deg; int quality;
        if (!parseCompass(value, &deg, &quality)) { sendError("Odo","compass","*","parse_err"); return false; }
        odom_receiveCompass(deg, quality);
        sendInfo("Odo","compass","*","OK");
        return true;
    }

    // mode
    if (strcmp(propAbbr,"mode")==0) {
        if (!value) return false;
        if (strcmp(value,"idle")==0) {
            odom_setActive(false);
            cfg_odom_report_period_ms = 1000;
        } else if (strcmp(value,"normal")==0) {
            odom_setActive(true);
            if (cfg_odom_period_ms == 0) cfg_odom_period_ms = 50;
        } else if (strcmp(value,"surv")==0) {
            odom_setActive(true);
            cfg_odom_period_ms = max(5UL, cfg_odom_period_ms/2);
        }
        sendInfo("Odo","mode","*", value);
        return true;
    }

    
   

    return false;
}

} // namespace OdoDispatch