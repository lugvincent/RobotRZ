// **** ./sensors/mic.h
#ifndef MIC_H
#define MIC_H

#include <Arduino.h>

void mic_init();

void mic_handleReadList(const int* indices, int nIndices, const char* retType);

void mic_setThreshold(const int* indices, int nIndices, int th);
void mic_setFreq(unsigned long ms);
void mic_setWin(unsigned long ms);
void mic_setActive(bool on);

void mic_processPeriodic();

#endif
