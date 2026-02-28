/************************************************************
 * FICHIER : src/actuators/lring.cpp
 * ROLE    : Implémentation du module LED Ring (Lring)
 *
 * Le module gère :
 *   - Les expressions internes (voyant, neutre, sourire…)
 *   - Les commandes RGB externes (mode EXTERN)
 *   - Le retour automatique à "neutre" après timeout
 *   - La gestion visuelle de l’urgence (mode EXPR_FORCED)
 *
 * Modes internes :
 *   - OFF : pas d’effet particulier, LEDs sous contrôle EXPR
 *   - EXPR : expression interne choisie par Node-RED
 *   - EXTERN : contrôle direct (rgb/clr)
 *   - EXPR_FORCED : urgence — override complet
 *
 * RÈGLES :
 *   - Toute commande expr => bascule en EXPR
 *   - Toute commande rgb/clr => bascule en EXTERN
 *   - urg_trigger => bascule EXPR_FORCED (priorité absolue)
 *   - L’urgence neutralise toute commande interne ou externe
 ************************************************************/

#include "lring.h"
#include "config/config.h"
#include "hardware/lring_hardware.h"
#include "communication/communication.h"
#include "communication/vpiv_utils.h"

#include <string.h>

/* ==========================================================
 * VARIABLES INTERNES AU MODULE
 * ========================================================== */

// Expression courante
static LringExpression curExpr = LRING_EXPR_NEUTRE;

// Mode EXTERN actif ?
static bool modeExtern = false;

// Expression forcée par l’urgence ?
static bool exprForcedByUrg = false;

// Timeout d’expression (0 = aucune expiration)
static unsigned long exprTimeout = 0;
static unsigned long exprExpireAt = 0;

// Gestion du clignotement
static bool blinkState = false;
static unsigned long lastBlinkToggle = 0;

/* ==========================================================
 * MAPPEURS (nom <-> enum)
 * ========================================================== */

static LringExpression _exprFromName(const char *s)
{
    if (!s)
        return LRING_EXPR_NEUTRE;
    if (strcasecmp(s, "neutre") == 0)
        return LRING_EXPR_NEUTRE;
    if (strcasecmp(s, "voyant") == 0)
        return LRING_EXPR_VOYANT;
    if (strcasecmp(s, "eclairage") == 0)
        return LRING_EXPR_ECLAIRAGE;
    if (strcasecmp(s, "sourire") == 0)
        return LRING_EXPR_SOURIRE;
    if (strcasecmp(s, "triste") == 0)
        return LRING_EXPR_TRISTE;
    if (strcasecmp(s, "alerte") == 0 || strcasecmp(s, "urgence") == 0)
        return LRING_EXPR_URGENCE;
    return LRING_EXPR_NEUTRE;
}

static const char *_nameFromExpr(LringExpression e)
{
    switch (e)
    {
    case LRING_EXPR_NEUTRE:
        return "neutre";
    case LRING_EXPR_VOYANT:
        return "voyant";
    case LRING_EXPR_ECLAIRAGE:
        return "eclairage";
    case LRING_EXPR_SOURIRE:
        return "sourire";
    case LRING_EXPR_TRISTE:
        return "triste";
    case LRING_EXPR_URGENCE:
        return "alerte";
    default:
        return "neutre";
    }
}

/* ==========================================================
 * APPLICATION D’ÉTATS PRÉDÉFINIS
 * ========================================================== */

static void _applyNeutral()
{
    lringhw_clear();
    lringhw_setPixel(5, 0, 160, 0);
    lringhw_setPixel(6, 0, 160, 0);
    lringhw_setPixel(7, 0, 160, 0);
    lringhw_show();
}

static void _applySourire()
{
    lringhw_clear();
    int idx[] = {4, 5, 6, 7, 8};
    for (int i = 0; i < 5; i++)
        lringhw_setPixel(idx[i], 0, 160, 0);
    lringhw_show();
}

static void _applyTriste()
{
    lringhw_clear();
    int idx[] = {10, 11, 0, 1, 2};
    for (int i = 0; i < 5; i++)
        lringhw_setPixel(idx[i], 200, 100, 0);
    lringhw_show();
}

static void _applyVoyant_on()
{
    lringhw_clear();
    lringhw_setPixel(6, 0, 160, 0);
    lringhw_show();
}

static void _applyVoyant_off()
{
    lringhw_setPixel(6, 0, 0, 0);
    lringhw_show();
}

static void _applyEclairage()
{
    lringhw_fill(255, 255, 255);
    lringhw_show();
}

static void _applyUrgence_on_full()
{
    lringhw_fill(255, 0, 0);
    lringhw_show();
}

static void _applyUrgence_off_full()
{
    lringhw_fill(0, 0, 0);
    lringhw_show();
}

static void _applyUrgence_on_single()
{
    lringhw_clear();
    lringhw_setPixel(0, 255, 0, 0);
    lringhw_show();
}

static void _applyUrgence_off_single()
{
    lringhw_setPixel(0, 0, 0, 0);
    lringhw_show();
}

/* ==========================================================
 * API : INITIALISATION
 * ========================================================== */

void lring_init()
{
    lringhw_init();

    curExpr = LRING_EXPR_NEUTRE;
    modeExtern = false;
    exprForcedByUrg = false;

    exprTimeout = 0;
    exprExpireAt = 0;

    blinkState = false;
    lastBlinkToggle = millis();

    _applyNeutral();
}

/* ==========================================================
 * API : MODE EXTERN
 * ========================================================== */

void lring_setExternMode(bool enable)
{
    if (exprForcedByUrg)
        return; // urgence interdit tout changement
    modeExtern = enable;
}

bool lring_isExternMode()
{
    return modeExtern;
}

/* ==========================================================
 * API : APPLY EXPRESSION (non urgence)
 * ========================================================== */

void lring_applyExpression(const char *exprName, unsigned long timeoutMs)
{

    if (exprForcedByUrg)
        return; // urgence = override total

    modeExtern = false; // retour mode interne

    curExpr = _exprFromName(exprName);
    exprTimeout = timeoutMs;
    exprExpireAt = (timeoutMs ? millis() + timeoutMs : 0);

    blinkState = true;
    lastBlinkToggle = millis();

    switch (curExpr)
    {
    case LRING_EXPR_NEUTRE:
        _applyNeutral();
        break;
    case LRING_EXPR_ECLAIRAGE:
        _applyEclairage();
        break;
    case LRING_EXPR_SOURIRE:
        _applySourire();
        break;
    case LRING_EXPR_TRISTE:
        _applyTriste();
        break;
    case LRING_EXPR_VOYANT:
        _applyVoyant_on();
        break;
    case LRING_EXPR_URGENCE:
        _applyUrgence_on_full();
        break;
    }

    sendInfo("Lring", "expr", "*", exprName);
}

void lring_setExpression(LringExpression expr)
{
    const char *n = _nameFromExpr(expr);
    lring_applyExpression(n, 0);
}

/* ==========================================================
 * API : URGENCE — override complet
 *résidu de l’ancienne logique d’urgence,
 * à revoir pour intégrer les codes d’urgence et la logique de longue urgence
 * ========================================================== */

void lring_emergencyTrigger(uint8_t urgCode)
{
    exprForcedByUrg = true;
    curExpr = LRING_EXPR_URGENCE;

    blinkState = true;
    lastBlinkToggle = millis();

    _applyUrgence_on_full();

    sendInfo("Lring", "urg", "*", String((int)urgCode).c_str());
}

/* ==========================================================
 * INFORMATION
 * ========================================================== */

const char *lring_currentExpression()
{
    return _nameFromExpr(curExpr);
}

/* ==========================================================
 * PROCESS CYCLIQUE — NON BLOQUANT
 * ========================================================== */

void lring_processPeriodic()
{

    unsigned long now = millis();

    /* ----- Gestion expiration / retour neutre ----- */
    if (!exprForcedByUrg && exprExpireAt != 0 && now >= exprExpireAt)
    {
        lring_applyExpression("neutre", 0);
    }

    /* ----- Mode urgence ----- */
    if (exprForcedByUrg || curExpr == LRING_EXPR_URGENCE)
    {

        if (now - lastBlinkToggle >= 500)
        {

            blinkState = !blinkState;
            lastBlinkToggle = now;

            bool longueUrg = (cfg_urg_active &&
                              (now - cfg_urg_timestamp) > 10000UL);

            if (blinkState)
            {
                if (longueUrg)
                    _applyUrgence_on_single();
                else
                    _applyUrgence_on_full();
            }
            else
            {
                if (longueUrg)
                    _applyUrgence_off_single();
                else
                    _applyUrgence_off_full();
            }
        }
        return;
    }

    /* ----- Mode EXTERN ----- */
    if (modeExtern)
        return;

    /* ----- Expressions à clignotement ----- */
    if (curExpr == LRING_EXPR_VOYANT)
    {
        if (now - lastBlinkToggle >= 1000)
        {
            blinkState = !blinkState;
            lastBlinkToggle = now;
            if (blinkState)
                _applyVoyant_on();
            else
                _applyVoyant_off();
        }
    }
}
