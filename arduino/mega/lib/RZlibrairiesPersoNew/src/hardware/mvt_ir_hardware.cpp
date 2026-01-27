/************************************************************
 * FICHIER : src/hardware/mvt_ir_hardware.cpp
 * ROLE    : Lecture bas niveau du capteur IR
 ************************************************************/
#include "mvt_ir_hardware.h"
#include "config/config.h"
#include <Arduino.h>

void mvt_ir_hw_init()
{
    // MVT_IR_PIN défini dans config.h
    pinMode(MVT_IR_PIN, INPUT);
}

int mvt_ir_hw_readRaw()
{
    return digitalRead(MVT_IR_PIN); // 0 ou 1
}