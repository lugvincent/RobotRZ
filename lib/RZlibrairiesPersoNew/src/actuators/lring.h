/************************************************************
 * FICHIER : src/actuators/lring.h
 * ROLE    : API haut-niveau du module LED Ring (Lring)
 *
 * Modes définis :
 *   - OFF          : LEDs éteintes
 *   - EXPR         : expression interne (voyant, neutre…)
 *   - EXTERN       : couleurs reçues via VPIV (rgb, clr…)
 *   - EXPR_FORCED  : mode urgence (blocs extern)
 ************************************************************/

#ifndef ACTUATORS_LRING_H
#define ACTUATORS_LRING_H

#include <stdint.h>

/* ==========================================================
 * ENUM DES EXPRESSIONS INTERNES
 * ========================================================== */
enum LringExpression {
    LRING_EXPR_NEUTRE = 0,
    LRING_EXPR_VOYANT,
    LRING_EXPR_ECLAIRAGE,
    LRING_EXPR_SOURIRE,
    LRING_EXPR_TRISTE,
    LRING_EXPR_URGENCE
};

/* ==========================================================
 * INITIALISATION
 * ========================================================== */
void lring_init();

/* ==========================================================
 * MODE DE FONCTIONNEMENT
 * ========================================================== */
void lring_setExternMode(bool enable);
bool lring_isExternMode();

/* ==========================================================
 * EXPRESSIONS INTERNES (MODE EXPR)
 * ========================================================== */
void lring_applyExpression(const char* exprName, unsigned long timeoutMs = 0);
void lring_setExpression(LringExpression expr);

/* ==========================================================
 * PROCESS CYCLIQUE
 * ========================================================== */
void lring_processPeriodic();

/* ==========================================================
 * URGENCE (MODE EXPR_FORCED)
 * ========================================================== */
void lring_emergencyTrigger(uint8_t urgCode);

/* ==========================================================
 * INFORMATIONS
 * ========================================================== */
const char* lring_currentExpression();

#endif
