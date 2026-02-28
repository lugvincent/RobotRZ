/************************************************************
 * FICHIER : src/hardware/mtr_hardware.cpp
 * ROLE : Implémentation envoi trame Mega -> UNO (format <L,R,A>)
 ************************************************************/

#include "mtr_hardware.h"
#include "config/config.h"

// trames (format littéral legacy)
char transMoteurP[8] = {'<', '0', ',', '0', ',', '0', '>', '\0'};
char oldTransMoteurP[8] = {'<', '0', ',', '0', ',', '0', '>', '\0'};

// modern buffer (ASCII) : "<L,R,A>\n"
char transMoteurModernBuf[64];

// initialisation hardware (ici, on aligne la trame avec cfg existants)
void mtr_hw_init()
{
    transMoteurP[0] = '<';
    transMoteurP[2] = ',';
    transMoteurP[4] = ',';
    transMoteurP[6] = '>';
    mtr_hw_updateTrameFromConfigLegacy();
    // modern initial empty
    snprintf(transMoteurModernBuf, sizeof(transMoteurModernBuf), "<0,0,1>");
}

// Update legacy trame from cfg (legacy chars)
void mtr_hw_updateTrameFromConfigLegacy()
{
    transMoteurP[1] = cfg_mtr_acc;
    transMoteurP[3] = cfg_mtr_spd;
    transMoteurP[5] = cfg_mtr_dir;
}

// Send legacy trame to UNO
void mtr_hw_sendLegacy()
{
    // only send 7 characters like old system (no newline required)
    for (int i = 0; i < 7; i++)
        MOTEURS_SERIAL.print(transMoteurP[i]);
    MOTEURS_SERIAL.print('\n');
}

// Update modern buffer from targets (L,R in raw units, A level)
void mtr_hw_updateTrameFromConfigModern(int L, int R, int A)
{
    // build ascii "<L,R,A>"
    // clip L,R to safe range
    if (L > 10000)
        L = 10000;
    if (L < -10000)
        L = -10000;
    if (R > 10000)
        R = 10000;
    if (R < -10000)
        R = -10000;
    if (A < 0)
        A = 0;
    if (A > 9)
        A = 9;
    snprintf(transMoteurModernBuf, sizeof(transMoteurModernBuf), "<%d,%d,%d>", L, R, A);
}

// Send modern trame (ASCII) to UNO
void mtr_hw_sendModern()
{
    MOTEURS_SERIAL.print(transMoteurModernBuf);
    MOTEURS_SERIAL.print('\n');
}

// Force urgent stop
void mtr_hw_forceUrgStop()
{
    // both modes: send zero
    // legacy:
    transMoteurP[1] = '0';
    transMoteurP[3] = '0';
    transMoteurP[5] = '0';
    mtr_hw_sendLegacy();
    // modern: 0,0,0
    mtr_hw_updateTrameFromConfigModern(0, 0, 0);
    mtr_hw_sendModern();
}

// Hook pour urg
void urg_stopAllMotors()
{
    mtr_hw_forceUrgStop();
}
