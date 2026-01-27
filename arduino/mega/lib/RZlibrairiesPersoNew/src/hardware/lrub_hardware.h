/******#ifndef LRUB_HARDWARE_H*****/
#define LRUB_HARDWARE_H

#include <Adafruit_NeoPixel.h>

/*************** PRIMITIVES MATÉRIELLES ***************/

void lrubhw_init();
void lrubhw_setActive(bool on);
void lrubhw_setPixel(int idx, int r, int g, int b);
void lrubhw_fill(int r, int g, int b);
void lrubhw_clear();
void lrubhw_show();
void lrubhw_setBrightness(int intensity);
bool lrubhw_isActive();
