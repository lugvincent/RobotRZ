/************************************************************
 * FICHIER : src/actuators/lrub.cpp
 * ROLE    : Implémentation haut-niveau du ruban LED (LRUB) - SIMPLE
 *
 * Utilise les primitives bas-niveau définies dans:
 *   src/hardware/lrub_hardware.h  (lrubhw_*)
 *
 * Comportement :
 *  - mode STATIC : couleur appliquée (sur tout ou indices)
 *  - mode EXTERN  : idem (on considère que pixel-by-pixel = EXTERN)
 *  - act 0/1 pour on/off
 *  - effet "fuite" simple
 *  - timeout possible pour extinction automatique
 ************************************************************/

#include "lrub.h"
#include "hardware/lrub_hardware.h"
#include "config/config.h"
#include "communication/communication.h"

#include <string.h>

/* États internes */
static bool lrub_isActive = false;
static bool lrub_modeExtern = false;    // false = STATIC, true = EXTERN (séman­tique seulement)
static unsigned long lrub_expireAt = 0; // 0 = pas de timeout

/* Initialisation */
void lrub_init()
{
    lrubhw_init();
    lrub_isActive = true;
    lrub_modeExtern = false;
    lrub_expireAt = 0;

    // Appliquer couleur par défaut centralisée
    lrubhw_setBrightness(cfg_lrub_brightness);
    lrubhw_fill(cfg_lrub_redRB, cfg_lrub_greenRB, cfg_lrub_blueRB);
    lrubhw_show();
}

/* Appliquer couleur à tout le ruban */
void lrub_applyColorAll(int r, int g, int b)
{
    lrubhw_fill(r, g, b);
    lrubhw_show();
    // mémoriser couleur centrale (utile si mode STATIC)
    cfg_lrub_redRB = r;
    cfg_lrub_greenRB = g;
    cfg_lrub_blueRB = b;
    lrub_modeExtern = false;
}

/* Appliquer couleur à une liste d'indices (nIndices == 0 => tous) */
void lrub_applyColorIndices(const int *indices, int nIndices, int r, int g, int b)
{
    if (!indices || nIndices == 0)
    {
        lrub_applyColorAll(r, g, b);
        return;
    }
    for (int i = 0; i < nIndices; i++)
    {
        int idx = indices[i];
        if (idx < 0 || idx >= LRUB_NUM_PIXELS)
            continue;
        lrubhw_setPixel(idx, r, g, b);
    }
    lrubhw_show();
    // si application ciblée, on considère que c'est EXTERN
    lrub_modeExtern = true;
}

/* Intensité */
void lrub_setIntensity(int intensity)
{
    if (intensity < 0)
        intensity = 0;
    if (intensity > 255)
        intensity = 255;
    cfg_lrub_brightness = intensity;
    lrubhw_setBrightness(intensity);
}

/* Activation on/off */
void lrub_setActive(bool on)
{
    lrubhw_setActive(on);
    lrub_isActive = on;
    if (!on)
    {
        lrub_expireAt = 0;
    }
}

/* Timeout absolu (millis). Passe 0 pour désactiver */
void lrub_setTimeout(unsigned long expireAtMs)
{
    lrub_expireAt = expireAtMs;
}

/* Effet fuite simple : alterne pairs/impairs */
void lrub_effectFuite()
{
    // utilisation de la couleur centrale si présente
    int r = cfg_lrub_redRB;
    int g = cfg_lrub_greenRB;
    int b = cfg_lrub_blueRB;
    for (int i = 0; i < LRUB_NUM_PIXELS; i++)
    {
        if (i % 2 == 0)
            lrubhw_setPixel(i, r, g, b);
        else
            lrubhw_setPixel(i, 0, 0, 0);
    }
    lrubhw_show();
    // on considère effet comme STATIC (non persistant si un timeout est planifié)
    lrub_modeExtern = false;
}

/* Process timeout : appeler régulièrement (par ex lrub_processTimeout dans loop) */
void lrub_processTimeout()
{
    if (!lrub_isActive)
        return;
    if (lrub_expireAt == 0)
        return;
    unsigned long now = millis();
    if (now >= lrub_expireAt)
    {
        // extinction demandée
        lrubhw_clear();
        lrubhw_show();
        lrub_setActive(false);
        lrub_expireAt = 0;
        sendInfo("Lrub", "timeout", "*", "OK");
    }
}

/* Dispatcher helper (ouvre la possibilité d'appeler directement depuis le module VPIV) */
bool lrub_dispatchVPIV(const char *prop, const char *inst, const char *value)
{
    // Ceci est redondant avec dispatch_Lrub.cpp mais pratique pour tests.
    (void)inst; // inst traitée au niveau du dispatcher
    if (!prop)
        return false;

    if (strcmp(prop, "col") == 0)
    {
        // value = "R,G,B"
        if (!value)
            return false;
        int rgb[3];
        if (!parseRGB(value, rgb))
        {
            sendError("Lrub", "col", "*", "bad_rgb");
            return false;
        }
        lrub_applyColorAll(rgb[0], rgb[1], rgb[2]);
        sendInfo("Lrub", "col", "*", value);
        return true;
    }

    if (strcmp(prop, "int") == 0)
    {
        int v = atoi(value);
        lrub_setIntensity(v);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", v);
        sendInfo("Lrub", "int", "*", buf);
        return true;
    }

    if (strcmp(prop, "act") == 0)
    {
        bool on = (value && (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0));
        lrub_setActive(on);
        sendInfo("Lrub", "act", "*", on ? "1" : "0");
        return true;
    }

    if (strcmp(prop, "f") == 0)
    { // effet fuite
        bool on = (value && (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0));
        if (on)
            lrub_effectFuite();
        else
            lrub_applyColorAll(cfg_lrub_redRB, cfg_lrub_greenRB, cfg_lrub_blueRB);
        sendInfo("Lrub", "f", "*", on ? "1" : "0");
        return true;
    }

    if (strcmp(prop, "init") == 0)
    {
        lrub_init();
        sendInfo("Lrub", "init", "*", "OK");
        return true;
    }

    return false;
}
