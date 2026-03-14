/************************************************************
 * FICHIER  : src/navigation/odom.h
 * CHEMIN   : arduino/mega/lib/RZlibrairiesPersoNew/src/navigation/odom.h
 * VERSION  : 3.0 — 2026-03-13
 * AUTEUR   : Vincent Philippe
 *
 * OBJECTIF : Interface publique du module odométrie différentielle.
 *            (differential drive odometry)
 *
 * PRINCIPE :
 *   Ce fichier déclare les fonctions appelables depuis l'extérieur.
 *   L'implémentation complète est dans odom.cpp.
 *
 * UTILISATION TYPIQUE :
 *   setup()  → odom_init()         (via RZ_initAll)
 *   loop()   → odom_processPeriodic()
 *   VPIV     → dispatch_Odom.cpp   → odom_setPhysical() / odom_resetPose()...
 *
 * ARTICULATION :
 *   ← config.h          (paramètres physiques cfg_odo_*)
 *   ← odom_hardware.h   (lecture ticks encodeurs)
 *   → communication.h   (sendInfo pour trames VPIV sortantes)
 *   ← dispatch_Odom.cpp (commandes VPIV entrantes)
 *
 ************************************************************/
#ifndef ODOM_H
#define ODOM_H

#include <Arduino.h>

/* ------------------------------------------------------------------ */
/* CYCLE DE VIE DU MODULE                                              */
/* ------------------------------------------------------------------ */

/**
 * odom_init()
 * Initialise le module : hardware encodeurs + calcul des coefficients
 * à partir des paramètres physiques de config.h/config.cpp.
 * Remet la pose (position + orientation) à zéro.
 * À appeler une seule fois dans RZ_initAll().
 */
void odom_init();

/**
 * odom_processPeriodic()
 * À appeler à chaque itération de loop().
 * Lit les ticks encodeurs, calcule la nouvelle pose (x, y, theta),
 * estime les vitesses, et envoie les trames VPIV selon les fréquences
 * configurées (pos toutes les cfg_odom_freq_pos_ms,
 *              vel toutes les cfg_odom_freq_vel_ms).
 */
void odom_processPeriodic();

/* ------------------------------------------------------------------ */
/* CONTRÔLE DE LA POSE (position + orientation)                        */
/* ------------------------------------------------------------------ */

/**
 * odom_resetPose()
 * Remet x=0, y=0, theta=0 et efface les compteurs encodeurs.
 * Déclenche immédiatement un envoi VPIV pos+vel (rapport one-shot).
 */
void odom_resetPose();

/**
 * odom_getPose(out_x, out_y, out_theta)
 * Lit la pose courante (lecture instantanée, sans modifier l'état).
 *   out_x, out_y : position en mètres
 *   out_theta    : orientation en radians (−π … +π, 0 = face à +x)
 */
void odom_getPose(double *out_x, double *out_y, double *out_theta);

/* ------------------------------------------------------------------ */
/* CONTRÔLE DU MODULE                                                  */
/* ------------------------------------------------------------------ */

/**
 * odom_setActive(on)
 * Active (true) ou met en pause (false) le calcul odométrique.
 * En pause, les ticks s'accumulent dans le hardware mais ne sont
 * pas traités (la pose reste gelée).
 */
void odom_setActive(bool on);

/**
 * odom_requestReport()
 * Force l'envoi d'un rapport combiné pos+vel au prochain cycle.
 * Le rapport a le format :
 *   $I:Odom:report:*:pos:x_mm,y_mm,theta_mrad;vel:v_mm_s,omega_mrad_s#
 * Appelé par le dispatcher pour les demandes one-shot (report:0).
 */
void odom_requestReport();

/* ------------------------------------------------------------------ */
/* CONFIGURATION DES PARAMÈTRES PHYSIQUES                              */
/* ------------------------------------------------------------------ */

/**
 * odom_setPhysical(wheel_mm, track_mm, cpr,
 *                  gainG_x1000, gainD_x1000,
 *                  freqCalc_ms, freqPos_ms, freqVel_ms)
 *
 * Met à jour les paramètres physiques du robot ET recalcule
 * automatiquement les coefficients de conversion ticks → distances.
 * NE remet PAS la pose à zéro (appeler odom_resetPose() séparément
 * si nécessaire).
 *
 * Appelée par dispatch_Odom.cpp lors d'une trame paraOdom :
 *   $V:Odom:*:paraOdom:wheel_mm,track_mm,cpr[,gainG,gainD,fCalc,fPos,fVel]#
 * Les 3 premiers paramètres sont obligatoires, les 5 suivants optionnels.
 * Passer 0 pour un paramètre optionnel = garder la valeur actuelle.
 *
 * Paramètres :
 *   wheel_mm    : diamètre de roue en mm (ex: 65 ou 100)
 *   track_mm    : entraxe entre les 2 roues en mm (ex: 200 ou 300)
 *   cpr         : ticks par tour encodeur (counts per revolution, ex: 256)
 *   gainG_x1000 : correctif roue gauche ×1000 (1000=neutre, 1020=+2%)
 *   gainD_x1000 : correctif roue droite  ×1000
 *   freqCalc_ms : période calcul odométrique (ms), ex: 50
 *   freqPos_ms  : période envoi automatique position (ms), ex: 200
 *   freqVel_ms  : période envoi automatique vitesse  (ms), ex: 40
 *
 * Retourne via VPIV :
 *   $I:Odom:paraOdom:*:wheel,track,cpr,gainG,gainD,fCalc,fPos,fVel#
 */
void odom_setPhysical(int wheel_mm, int track_mm, int cpr,
                      int gainG_x1000, int gainD_x1000,
                      unsigned long freqCalc_ms,
                      unsigned long freqPos_ms,
                      unsigned long freqVel_ms);

/**
 * odom_setCoefficients(gLong, dLong, gAngl, dAngl)
 * Surcharge directe des 4 coefficients (mode expert / test).
 * Préférer odom_setPhysical() pour la configuration normale via paraOdom.
 *
 *   gLong, dLong : distance en mètres par tick (m/tick) — ex: 0.001227
 *   gAngl, dAngl : contribution angulaire par mètre (rad/m) — POSITIFS
 *                  La soustraction dAng = distD×dAngl − distG×gAngl
 *                  est dans odom_processPeriodic() (modèle différentiel)
 */
void odom_setCoefficients(double gLong_m_per_tick, double dLong_m_per_tick,
                          double gAngl_rad_per_m, double dAngl_rad_per_m);

/* ------------------------------------------------------------------ */
/* FUSION CAPTEURS EXTERNES (données venant de SE via SP)              */
/* ------------------------------------------------------------------ */

/**
 * odom_receiveGyro(gyro_z_dps, ts_ms)
 * Mémorise la vitesse angulaire mesurée par le gyroscope du smartphone SE.
 *   gyro_z_dps : vitesse angulaire axe Z en degrés/seconde (deg/s)
 *   ts_ms      : horodatage de la mesure en ms (millis côté SE)
 * Utilisation future : fusion gyro+encodeurs pour corriger la dérive angulaire.
 */
void odom_receiveGyro(float gyro_z_dps, unsigned long ts_ms);

/**
 * odom_receiveCompass(heading_deg, quality)
 * Mémorise le cap magnétique mesuré par la boussole (magnétomètre) de SE.
 *   heading_deg : cap en degrés (0–360, 0 = Nord magnétique)
 *   quality     : indice de fiabilité de la mesure (0=mauvais, 100=excellent)
 * Utilisation future : correction de la dérive long terme de l'orientation.
 */
void odom_receiveCompass(float heading_deg, int quality);

#endif // ODOM_H