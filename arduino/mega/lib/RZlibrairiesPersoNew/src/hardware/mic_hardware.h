/************************************************************
 * FICHIER : src/hardware/mic_hardware.h
 * ROLE    : Interface matérielle pour les microphones (3 canaux)
 *
 * OBJECTIF :
 *   - Fournir une API HARDWARE, non bloquante, pour les microphones
 *   - Centraliser l'accès ADC et le calcul RAW / PEAK / RMS
 *   - Être utilisé par le module haut-niveau sensors/mic.cpp
 *
 * ARCHITECTURE :
 *   mic.cpp
 *      ↓
 *   mic_hardware.h / mic_hardware.cpp
 *      ↓
 *   analogRead()
 *
 * IMPORTANT :
 *   - Ce module N'EXPOSE AUCUNE logique VPIV
 *   - Il ne contient PAS de temporisation bloquante
 *   - Il est appelé très fréquemment depuis loop()
 *
 ************************************************************/

#ifndef MIC_HARDWARE_H
#define MIC_HARDWARE_H

#include <Arduino.h>
#include "config/config.h"

/************************************************************
 * CONFIGURATION GLOBALE
 ************************************************************/

// Nombre de microphones physiques
// Fixe et connu à la compilation
#define MIC_COUNT 3

/************************************************************
 * MAPPING HARDWARE
 ************************************************************/

// Tableau des pins ADC pour chaque micro
// Défini dans mic_hardware.cpp
//
// Index :
//   0 = Avant
//   1 = Gauche
//   2 = Droite
//
extern const int MIC_PINS[MIC_COUNT];

/************************************************************
 * VARIABLES GLOBALES PUBLIQUES (HARDWARE-LEVEL)
 *
 * Ces variables représentent les DERNIÈRES VALEURS
 * calculées sur une fenêtre complète.
 *
 * Elles sont mises à jour automatiquement par
 * mic_sampler_pollOne() / mic_sampler_pollAll()
 *
 * ATTENTION :
 *   - Ce sont des "snapshots" récents
 *   - Elles peuvent être lues à tout moment
 *   - Elles NE déclenchent AUCUNE acquisition par elles-mêmes
 ************************************************************/

// Valeur moyenne brute (RAW) sur la dernière fenêtre
// mic_lastRMS[]
// ----------------
// RMS (Root Mean Square) ENTIER du signal micro, calculé sur une fenêtre temporelle.
// - unité : valeur ADC (relative)
// - pas de float volontairement (performance Arduino Mega)
// - utilisé pour :
//     * détection de seuil sonore
//     * comparaison gauche / droite / avant (orientation)
extern int mic_lastRaw[MIC_COUNT];

// Valeur RMS (Root Mean Square) ENTIER
// Utilisée pour détection direction / niveau sonore
extern int mic_lastRMS[MIC_COUNT];

// Valeur PEAK max détectée sur la fenêtre
extern int mic_lastPeak[MIC_COUNT];

/************************************************************
 * API HARDWARE PUBLIQUE
 ************************************************************/

/**
 * mic_initHardware()
 *
 * Rôle :
 *   - Initialise l'état interne des samplers
 *   - Effectue une première lecture ADC pour chaque micro
 *   - Prépare les fenêtres temporelles
 *
 * Appel :
 *   - UNE SEULE FOIS au démarrage
 *   - Depuis mic_init() (module haut niveau)
 */
void mic_initHardware();

/**
 * mic_sampler_pollOne(idx)
 *
 * Rôle :
 *   - Effectue UNE lecture ADC pour le micro idx
 *   - Met à jour les accumulateurs internes
 *   - Vérifie si la fenêtre temporelle est terminée
 *   - Si oui :
 *       - calcule RAW / RMS / PEAK
 *       - met à jour mic_lastRaw / mic_lastRMS / mic_lastPeak
 *
 * IMPORTANT :
 *   - NON BLOQUANT
 *   - Très rapide
 *   - Peut être appelé à CHAQUE tour de loop()
 *
 * Paramètre :
 *   idx : index du micro (0 .. MIC_COUNT-1)
 */
void mic_sampler_pollOne(int idx);

/**
 * mic_sampler_pollAll()
 *
 * Rôle :
 *   - Appelle mic_sampler_pollOne() pour TOUS les micros
 *   - Effectue exactement UN sample par micro
 *
 * Usage typique :
 *   - Appelé depuis mic_processPeriodic()
 *   - Ou directement depuis loop()
 *
 * Avantage :
 *   - Distribution équitable des échantillons
 *   - Charge CPU maîtrisée
 */
void mic_sampler_pollAll();

#endif // MIC_HARDWARE_H
