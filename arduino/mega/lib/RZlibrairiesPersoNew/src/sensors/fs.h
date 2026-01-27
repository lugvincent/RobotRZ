/************************************************************
 * FICHIER : src/sensors/fs.h
 * ROLE    : Interface du module FORCE SENSOR (FS)
 les fonctions suivantes ont pu évolué avec les versions, sans evol des notes
 *
 * API publique (à utiliser depuis dispatcher VPIV / application) :
 *  - fs_init()                 : initialisation
 *  - fs_readRaw()     ?         : lecture brute (long)
 *  - fs_readValueInt()  ?       : valeur convertie (int)
 *  - fs_getAngle()             : angle estimé [-90..90] (int)
 *  - fs_doTare()          : enregistre tare (offset)
 *  - fs_doCal()       : définit facteur de conversion
 *  - fs_setThreshold()         : seuil d'alerte (int)
 *  - fs_setFreqMs()            : période de monitoring (ms)
 *  - fs_setActive()            : activer / désactiver
 *  - fs_processPeriodic()      : tick périodique (appel depuis loop())
 ************************************************************/
#include <Arduino.h>
#ifndef FS_H
#define FS_H

// public functions
void fs_init();
void fs_processPeriodic();

// commands
void fs_doRead(const char *type);
void fs_doTare();
void fs_doCal(long factor);
void fs_setActive(bool on);
void fs_setFreq(unsigned long ms);
void fs_setThreshold(int th);

// direction (angle -90..+90)
int fs_lastAngle();
long fs_lastForce();

#endif
