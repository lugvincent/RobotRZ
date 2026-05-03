/************************************************************
 * FICHIER : src/hardware/mic_hardware.h
 * ROLE    : Interface matérielle pour les microphones (3 canaux)
 *
 * Description :
 *   - Fournit l’accès hardware aux 3 microphones analogiques
 *   - Effectue les mesures fenêtrées : RAW, PEAK, RMS
 *   - Ces mesures sont ensuite exploitées en haut niveau pour la
 *     détection d’alertes et l’estimation de direction.
 *
 * Instances :
 *   0 = AV (avant)
 *   1 = G  (gauche)
 *   2 = D  (droite)
 *
 * Notes techniques :
 *   - La détermination de direction du son repose sur les RMS/Peak
 *     mesurés de manière cohérente dans la même fenêtre temporelle.
 *   - Chaque mesure utilise une boucle d’échantillonnage compressée
 *     dans un laps de temps court (fenêtre configurable).
 *
 ************************************************************/

#ifndef MIC_HARDWARE_H
#define MIC_HARDWARE_H

#include <Arduino.h>
#include "../configuration/config.h"

// Nombre de micros (fixe)
#define MIC_COUNT 3

// Tableau de pins ADC pour les 3 micros (défini dans mic_hardware.cpp)
extern const int MIC_PINS[MIC_COUNT];

/* --------------------------------------------------------------------
 * Variables globales hardware (dernières valeurs mesurées)
 * Stockées ici car *hardware-level*, utilisées par sensors/mic.cpp
 * ------------------------------------------------------------------*/

// Dernière valeur brute moyenne sur fenêtre
extern int   mic_lastRaw[MIC_COUNT];

// Dernière valeur RMS calculée sur fenêtre
extern float mic_lastRMS[MIC_COUNT];

// Dernière valeur PEAK détectée sur fenêtre
extern int   mic_lastPeak[MIC_COUNT];

/* --------------------------------------------------------------------
 * FONCTIONS HARDWARE
 * ------------------------------------------------------------------*/

/**
 * mic_initHardware()
 *  - Initialise les micros (lecture initiale)
 *  - Ne configure pas les pins (analogRead ne nécessite pas pinMode)
 */
void mic_initHardware();

/**
 * mic_sampleWindow(idx, windowMs, peakOut, rmsOut)
 *
 * Rôle :
 *   - Lit un microphone pendant une fenêtre temporelle fixe
 *   - Retourne :
 *       raw   : moyenne brute dans la fenêtre
 *       peak  : maximum instantané
 *       rms   : RMS ± amortie autour de la moyenne
 *
 * Paramètres :
 *   idx       : index micro (0=AV, 1=G, 2=D)
 *   windowMs  : largeur fenêtre en ms
 *   peakOut   : (out) valeur peak mesurée
 *   rmsOut    : (out) RMS calculée
 *
 * Méthode :
 *   - boucle rapide analogRead()
 *   - accumulateur somme et somme des carrés
 *   - calcul RMS = sqrt(meanSq - mean²)
 *
 * REMARQUE :
 *   Cette fonction est synchrone et *bloquante* pendant la durée windowMs.
 *   Elle est conçue pour mic_handleReadList() ET mic_processPeriodic().
 */
int mic_sampleWindow(int idx, unsigned long windowMs, int &peakOut, float &rmsOut);

#endif
