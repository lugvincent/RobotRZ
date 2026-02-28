/************************************************************
 * FICHIER : src/hardware/fs_hardware.cpp
 * ROLE    : Lecture HX711 non-bloquante, tout entier
 ************************************************************/

#include "fs_hardware.h"

// HX711 timing
static unsigned long lastConv = 0;

// INIT
void fs_hw_init() {
    pinMode(FS_SCK_PIN, OUTPUT);
    pinMode(FS_DOUT_PIN, INPUT);
    digitalWrite(FS_SCK_PIN, LOW);
    lastConv = micros();
}

// Lire raw (24 bits signé), non bloquant
bool fs_hw_readRaw(long &outValue) {

    if (digitalRead(FS_DOUT_PIN) == HIGH)
        return false;   // pas prêt

    long value = 0;

    // lecture des 24 bits
    for (int i = 0; i < 24; i++) {
        digitalWrite(FS_SCK_PIN, HIGH);
        delayMicroseconds(1);
        value = (value << 1) | (digitalRead(FS_DOUT_PIN) & 1);
        digitalWrite(FS_SCK_PIN, LOW);
        delayMicroseconds(1);
    }

    // Gain 128 pulse
    digitalWrite(FS_SCK_PIN, HIGH);
    delayMicroseconds(1);
    digitalWrite(FS_SCK_PIN, LOW);

    // signe
    if (value & 0x800000)
        value |= ~0xFFFFFF;

    outValue = value;
    return true;
}

void fs_hw_tare() {
    long r;
    if (fs_hw_readRaw(r))
        cfg_fs_offset = r;
}

void fs_hw_setCal(long cal) {
    cfg_fs_calibration = cal;
}

long fs_hw_readForce() {
    long r;
    if (!fs_hw_readRaw(r))
        return 0;

    long v = r - cfg_fs_offset;
    return (v / cfg_fs_calibration);
}
