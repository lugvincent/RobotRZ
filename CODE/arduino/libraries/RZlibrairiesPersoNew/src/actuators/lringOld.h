/************************************************************
 * FICHIER : src/actuators/lring.h
 * ROLE    : API haut-niveau du module LED Ring (Lring)
 *
 * Cette interface fournit :
 *   - Initialisation du ring
 *   - Gestion des expressions visuelles (voyant, sourire…)
 *   - Gestion automatique des timeouts
 *   - Process cyclique non bloquant à appeler dans loop()
 *   - Réaction automatique à l’urgence (alerte visuelle)
 *
 * Le dispatcher VPIV NE SE TROUVE PAS ICI :
 *    → il est dans : src/communication/dispatch_Lring.cpp
 ************************************************************/

#ifndef ACTUATORS_LRING_H
#define ACTUATORS_LRING_H

#include <Arduino.h>

// ----------------------------------------------------------
// INITIALISATION
// ----------------------------------------------------------
void lring_init();  
// Appelée une seule fois dans setup()

// ----------------------------------------------------------
// EXPRESSIONS VISUELLES
// ----------------------------------------------------------
// Applique une expression définie :
//   "voyant", "eclairage", "alerte", "neutre", "sourire", "triste"
// timeoutMs = 0 => effet persistant
void lring_applyExpression(const char* expr, unsigned long timeoutMs = 0);

// ----------------------------------------------------------
// PROCESS CYCLIQUE
// ----------------------------------------------------------
// À appeler régulièrement dans loop() (toutes les 20–30ms)
void lring_processPeriodic();

// ----------------------------------------------------------
// URGENCE
// ----------------------------------------------------------
// Appelé automatiquement par urg_handle()
// pour déclencher l’effet "alerte"
void lring_emergencyTrigger(uint8_t urgCode);

// ----------------------------------------------------------
// LECTURE
// ----------------------------------------------------------
const char* lring_currentExpression();

#endif // ACTUATORS_LRING_H
