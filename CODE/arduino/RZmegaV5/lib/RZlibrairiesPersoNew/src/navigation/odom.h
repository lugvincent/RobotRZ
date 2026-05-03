/************************************************************
 * FICHIER : src/sensors/odom.h
 * ROLE    : Interface publique du module odométrie
 *
 * Chemin  : src/sensors/odom.h
 *
 * Fonctions exportées :
 *  - odom_init()
 *  - odom_processPeriodic()  // à appeler dans loop()
 *  - odom_resetPose()
 *  - odom_getPose(&x,&y,&theta)
 *  - odom_setActive(bool)
 *  - odom_requestReport()    // force envoi VPIV (utilisable par dispatch)
 *
 ************************************************************/
#ifndef ODOM_H
#define ODOM_H

#include <Arduino.h>

/* Initialisation + périodique */
void odom_init();
void odom_processPeriodic();

/* Accès état (pose en mètres, theta en radians) */
void odom_resetPose();
void odom_getPose(double* out_x, double* out_y, double* out_theta);

/* Contrôle */
void odom_setActive(bool on);
void odom_requestReport(); // force envoi VPIV immédiat
void odom_setCoefficients(double gLong_m_per_tick, double dLong_m_per_tick,
                          double gAngl_rad_per_tick, double dAngl_rad_per_tick);

/* Fusion capteurs externes */
void odom_receiveGyro(float gyro_z_dps, unsigned long ts_ms);
void odom_receiveCompass(float heading_deg, int quality);

#endif // ODOM_H