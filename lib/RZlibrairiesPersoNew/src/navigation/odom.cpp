/************************************************************
 * FICHIER  : src/navigation/odom.cpp
 * CHEMIN   : arduino/mega/lib/RZlibrairiesPersoNew/src/navigation/odom.cpp
 * VERSION  : 3.0 — 2026-03-13
 * AUTEUR   : Vincent Philippe
 *
 * OBJECTIF : Calculer la position et l'orientation du robot à partir
 *            des encodeurs de roues. Émettre ces données en VPIV.
 *
 * ===================================================================
 * PRINCIPE DE L'ODOMÉTRIE DIFFÉRENTIELLE (differential drive odometry)
 * ===================================================================
 *
 *  Le robot est mû par deux roues indépendantes [G] et [D].
 *  Chaque roue est équipée d'un encodeur incrémental qui génère des
 *  "ticks" (impulsions) proportionnels à sa rotation.
 *
 *  Repère utilisé (convention robotique standard) :
 *
 *          ↑ y
 *          |
 *          |    [G]───────[D]   ← vue de dessus
 *          |       robot
 *          O────────────────→ x
 *
 *  - theta = 0    → robot orienté vers +x (vers la droite)
 *  - theta = +π/2 → robot orienté vers +y (vers le haut)
 *  - theta croît dans le sens trigonométrique (anti-horaire / CCW)
 *
 *  À chaque cycle de calcul, on effectue :
 *
 *  1. Lecture des deltas ticks depuis le dernier cycle :
 *       dG = ticks_gauche_new − ticks_gauche_old
 *       dD = ticks_droit_new  − ticks_droit_old
 *
 *  2. Conversion en distances parcourues par chaque roue (en mètres) :
 *       distG = dG × coeffGLong    (m, distance roue gauche)
 *       distD = dD × coeffDLong    (m, distance roue droite)
 *
 *  3. Distance et rotation du centre du robot :
 *       dDist = (distG + distD) / 2   (m, avance du centre)
 *       dAng  = (distD − distG) / entraxe   (rad, rotation)
 *            ← formule du modèle différentiel
 *            ← dans le code : distD×coeffDAngl − distG×coeffGAngl
 *               avec coeffGAngl = coeffDAngl = 1/entraxe
 *
 *  4. Mise à jour de la pose avec l'approximation "mid-point" :
 *       midTheta = theta + dAng/2      (angle au milieu du déplacement)
 *       x       += dDist × cos(midTheta)
 *       y       += dDist × sin(midTheta)
 *       theta   += dAng
 *
 *  5. Normalisation de theta dans [−π, +π]
 *
 * ===================================================================
 * COEFFICIENTS DE CONVERSION (rappel des formules)
 * ===================================================================
 *
 *  circumférence = π × D             (D = diamètre roue en m)
 *  dist_par_tick = circumférence / CPR
 *  coeffGLong    = dist_par_tick × gainG   (m/tick)
 *  coeffDLong    = dist_par_tick × gainD   (m/tick)
 *  coeffGAngl    = 1 / entraxe             (rad/m — positif !)
 *  coeffDAngl    = 1 / entraxe             (rad/m — positif !)
 *
 *  ⚠ Les deux coefficients angulaires sont POSITIFS.
 *    C'est la SOUSTRACTION dans dAng = distD×dAngl − distG×gAngl
 *    qui réalise le modèle différentiel.
 *
 * ===================================================================
 * SORTIES VPIV — ENTIERS UNIQUEMENT (pas de float sur le fil)
 * ===================================================================
 *
 *  Envoi périodique automatique :
 *    $I:Odom:pos:*:x_mm,y_mm,theta_mrad#
 *      x_mm       = x      × 1000   (millimètres)
 *      y_mm       = y      × 1000   (millimètres)
 *      theta_mrad = theta  × 1000   (milliradians)
 *
 *    $I:Odom:vel:*:v_mm_s,omega_mrad_s#
 *      v_mm_s      = v     × 1000   (mm/s)
 *      omega_mrad_s= omega × 1000   (mrad/s)
 *
 *  Rapport combiné one-shot (sur demande) :
 *    $I:Odom:report:*:pos:x_mm,y_mm,theta_mrad;vel:v_mm_s,omega_mrad_s#
 *
 * ===================================================================
 * ARTICULATION
 * ===================================================================
 *   ← RZ_initAll()       appelle odom_init()
 *   ← loop()             appelle odom_processPeriodic()
 *   ← dispatch_Odom.cpp  appelle les fonctions publiques (paraOdom, reset…)
 *   ← odom_hardware.cpp  fournit les ticks encodeurs
 *   ← config.cpp         fournit les paramètres physiques cfg_odo_*
 *   → communication.h    sendInfo / sendFlux pour trames VPIV sortantes
 *
 * PRÉCAUTIONS :
 *   - Aucun float dans les trames VPIV sortantes (règle protocole VPIV)
 *   - Pas de String() Arduino (fragmentation mémoire sur le tas / heap)
 *   - Les coefficients sont recalculés via _recompute_coeffs_from_physical()
 *     appelée depuis odom_init() et odom_setPhysical()
 *
 ************************************************************/

#include "odom.h"
#include "hardware/odom_hardware.h"
#include "config/config.h"
#include "communication/communication.h"
#include <Arduino.h>
#include <math.h>

/* ================================================================== */
/* ÉTAT INTERNE DU MODULE                                              */
/* (variables statiques = invisibles hors de ce fichier)              */
/* ================================================================== */

/* --- Pose courante du robot dans le repère monde (world frame) --- */
static double xR = 0.0;          /* abscisse X en mètres                          */
static double yR = 0.0;          /* ordonnée Y en mètres                          */
static double orientation = 0.0; /* cap (heading) en radians, normalisé [−π, +π] */

/* --- Vitesses estimées au dernier cycle --- */
static double last_v = 0.0;     /* vitesse linéaire  (linear velocity)  en m/s   */
static double last_omega = 0.0; /* vitesse angulaire (angular velocity) en rad/s */

/* --- Horodatages (timestamps) --- */
static unsigned long lastComputeTs = 0;   /* dernier calcul odométrique    (ms) */
static unsigned long lastPosReportTs = 0; /* dernier envoi trame pos       (ms) */
static unsigned long lastVelReportTs = 0; /* dernier envoi trame vel       (ms) */

/* ================================================================== */
/* COEFFICIENTS DE CONVERSION TICKS → DISTANCES                       */
/* (copies locales des valeurs de config — recalculées à chaque init) */
/* ================================================================== */

/*
 * coeffGLong / coeffDLong : distance en mètres par tick (m/tick)
 *   → multiplient les ticks bruts pour obtenir distG et distD en mètres
 *   → incluent le gain correctif gauche/droite (calibration asymétrie)
 */
static double coeffGLong = 0.0; /* m/tick, roue gauche (left  longitudinal coefficient) */
static double coeffDLong = 0.0; /* m/tick, roue droite (right longitudinal coefficient) */

/*
 * coeffGAngl / coeffDAngl : contribution angulaire par mètre (rad/m)
 *   → s'appliquent sur les DISTANCES en mètres (pas sur les ticks)
 *   → valeur = 1 / entraxe  (rad/m)
 *
 *   Usage dans processPeriodic :
 *     dAng = distD × coeffDAngl − distG × coeffGAngl
 *          = distD/entraxe − distG/entraxe
 *          = (distD − distG) / entraxe    ← modèle différentiel correct
 *
 *   ⚠ Ces deux valeurs sont POSITIVES.
 *     La soustraction est DANS la formule, pas dans les coefficients.
 */
static double coeffGAngl = 0.0; /* rad/m, contribution gauche (left  angular coefficient) */
static double coeffDAngl = 0.0; /* rad/m, contribution droite (right angular coefficient) */

/* ================================================================== */
/* CALCUL DES COEFFICIENTS À PARTIR DES PARAMÈTRES PHYSIQUES          */
/* ================================================================== */

/**
 * _recompute_coeffs_from_physical()
 *
 * Recalcule les 4 coefficients à partir de :
 *   cfg_odo_wheel_diameter_m   (diamètre roue en m)
 *   cfg_odo_track_m            (entraxe en m)
 *   cfg_odo_encoder_cpr        (ticks par tour, counts per revolution)
 *   cfg_odo_left_gain_x1000    (correctif gauche ×1000)
 *   cfg_odo_right_gain_x1000   (correctif droite ×1000)
 *
 * Formules appliquées :
 *   circumférence   = π × D
 *   dist_par_tick   = circumférence / CPR
 *   coeffGLong      = dist_par_tick × gainG
 *   coeffDLong      = dist_par_tick × gainD
 *   coeffGAngl      = 1 / entraxe   (POSITIF — voir note signe plus haut)
 *   coeffDAngl      = 1 / entraxe   (POSITIF)
 *
 * Sécurité : si un paramètre est invalide (≤0), on garde les coefficients
 * existants pour ne pas bloquer le robot.
 */
static void _recompute_coeffs_from_physical()
{
    double D = cfg_odo_wheel_diameter_m; /* diamètre roue (wheel diameter) en m */
    double track = cfg_odo_track_m;      /* entraxe (wheelbase / track)     en m */
    int cpr = cfg_odo_encoder_cpr;       /* ticks par tour (counts/revolution)   */

    /* Sécurité : paramètres invalides → on garde les coefficients en place */
    if (D <= 0.0 || track <= 0.0 || cpr <= 0)
    {
        /* récupère les dernières valeurs valides depuis config */
        coeffGLong = cfg_odo_coeff_gLong;
        coeffDLong = cfg_odo_coeff_dLong;
        coeffGAngl = cfg_odo_coeff_gAngl;
        coeffDAngl = cfg_odo_coeff_dAngl;
        return;
    }

    /* Gains correctifs (valeur 1000 = neutre, pas de correction) */
    double gainG = (double)cfg_odo_left_gain_x1000 / 1000.0;
    double gainD = (double)cfg_odo_right_gain_x1000 / 1000.0;

    /* Distance parcourue par UNE roue pour 1 tick */
    double circumference = M_PI * D;                    /* tour complet (m) */
    double dist_per_tick = circumference / (double)cpr; /* m/tick       */

    /* Coefficients longitudinaux (intègrent le correctif de calibration) */
    coeffGLong = dist_per_tick * gainG; /* m/tick, roue gauche */
    coeffDLong = dist_per_tick * gainD; /* m/tick, roue droite */

    /* Coefficients angulaires — POSITIFS (la soustraction est dans processPeriodic)
     * dAng = distD × (1/track) − distG × (1/track) = (distD−distG)/track     */
    coeffDAngl = 1.0 / track; /* rad/m — contribution de distD à la rotation */
    coeffGAngl = 1.0 / track; /* rad/m — contribution de distG à la rotation */

    /* Répercussion dans config (pour lecture externe si besoin) */
    cfg_odo_coeff_gLong = coeffGLong;
    cfg_odo_coeff_dLong = coeffDLong;
    cfg_odo_coeff_gAngl = coeffGAngl;
    cfg_odo_coeff_dAngl = coeffDAngl;
}

/* ================================================================== */
/* INITIALISATION                                                      */
/* ================================================================== */

void odom_init()
{
    /* 1. Initialiser le hardware (encodeurs / interruptions) */
    odom_hw_init();

    /* 2. Calculer les coefficients à partir des paramètres physiques
     *    définis dans config.cpp (valeurs par défaut ou déjà modifiées) */
    _recompute_coeffs_from_physical();

    /* 3. Remettre la pose à zéro */
    xR = 0.0;
    yR = 0.0;
    orientation = 0.0;
    last_v = 0.0;
    last_omega = 0.0;

    /* 4. Initialiser les horloges internes */
    lastComputeTs = millis();
    lastPosReportTs = millis();
    lastVelReportTs = millis();
}

/* ================================================================== */
/* RESET POSE                                                          */
/* ================================================================== */

void odom_resetPose()
{
    /* Remet la position et l'orientation à l'origine */
    xR = 0.0;
    yR = 0.0;
    orientation = 0.0;

    /* Remet les compteurs hardware à zéro pour repartir proprement */
    odom_hw_forceClear();

    /* Réinitialise les horloges */
    lastComputeTs = millis();
    lastPosReportTs = millis();
    lastVelReportTs = millis();

    /* Force l'envoi d'un rapport combiné au prochain cycle */
    cfg_odom_pendingReport = true;
}

/* ================================================================== */
/* BOUCLE PÉRIODIQUE — CŒUR DU MODULE                                 */
/* ================================================================== */

void odom_processPeriodic()
{
    /* Rien à faire si le module est désactivé */
    if (!cfg_odom_active)
        return;

    unsigned long now = millis();

    /* ----------------------------------------------------------------
     * ÉTAPE 1 : CALCUL ODOMÉTRIQUE
     * Cadence définie par cfg_odom_period_ms (ex: 50ms = 20Hz)
     * ---------------------------------------------------------------- */
    if (now - lastComputeTs >= cfg_odom_period_ms)
    {
        /* Lire les deltas ticks depuis le dernier cycle
         * (la fonction remet les compteurs à zéro après lecture)
         * dG > 0 : roue gauche tourne en avant ; < 0 : en arrière   */
        long dG = odom_hw_getAndClearLeftTicks();  /* delta ticks gauche */
        long dD = odom_hw_getAndClearRightTicks(); /* delta ticks droite */

        if (dG != 0 || dD != 0)
        {
            /* --- Conversion ticks → distances en mètres --- */
            double distG = (double)dG * coeffGLong; /* distance parcourue roue gauche (m) */
            double distD = (double)dD * coeffDLong; /* distance parcourue roue droite (m) */

            /* --- Avance du centre et rotation (modèle différentiel) ---
             * dDist = avance du milieu du robot (translation moyenne)
             * dAng  = rotation du robot (positif = virage à droite CCW)
             *
             * Formule angulaire : dAng = (distD − distG) / entraxe
             * Codée comme       : distD×coeffDAngl − distG×coeffGAngl
             *                     avec coeffDAngl = coeffGAngl = 1/entraxe */
            double dDist = (distG + distD) * 0.5;                  /* m   */
            double dAng = distD * coeffDAngl - distG * coeffGAngl; /* rad */

            /* --- Mise à jour de la pose ("mid-point integration") ---
             * On utilise l'angle à mi-chemin pour une meilleure précision
             * que si on utilisait l'angle de début ou de fin.            */
            double midOrient = orientation + dAng * 0.5;
            xR += dDist * cos(midOrient); /* nouveau x (m) */
            yR += dDist * sin(midOrient); /* nouveau y (m) */
            orientation += dAng;          /* nouvel angle (rad) */

            /* --- Normaliser theta dans [−π, +π] ---
             * Évite que l'angle ne dérive vers des valeurs très grandes
             * après de nombreux tours (ex: après 3 tours = 6π → ramené à 0) */
            if (orientation > M_PI)
                orientation -= 2.0 * M_PI;
            if (orientation < -M_PI)
                orientation += 2.0 * M_PI;

            /* --- Estimation des vitesses instantanées ---
             * dt = temps écoulé depuis le dernier calcul (en secondes)
             * v     = vitesse linéaire  en m/s
             * omega = vitesse angulaire en rad/s                          */
            double dt = (double)(now - lastComputeTs) / 1000.0;
            if (dt < 1e-6)
                dt = 1e-6;          /* sécurité division par zéro */
            last_v = dDist / dt;    /* m/s   */
            last_omega = dAng / dt; /* rad/s */
        }
        else
        {
            /* Aucun déplacement : les vitesses tombent à 0 */
            last_v = 0.0;
            last_omega = 0.0;
        }

        lastComputeTs = now;
    }

    /* ----------------------------------------------------------------
     * ÉTAPE 2 : RAPPORT ONE-SHOT (demandé explicitement)
     * Format combiné pos+vel — envoyé une seule fois
     * ---------------------------------------------------------------- */
    if (cfg_odom_pendingReport)
    {
        /* Conversion en entiers pour le protocole VPIV (pas de float !) */
        long x_mm = (long)(xR * 1000.0);                 /* mm       */
        long y_mm = (long)(yR * 1000.0);                 /* mm       */
        long theta_mrad = (long)(orientation * 1000.0);  /* mrad     */
        long v_mm_s = (long)(last_v * 1000.0);           /* mm/s     */
        long omega_mrad_s = (long)(last_omega * 1000.0); /* mrad/s   */

        /* FORMAT VPIV rapport combiné :
         * $I:Odom:report:*:pos:x_mm,y_mm,theta_mrad;vel:v_mm_s,omega_mrad_s#
         * Exemple : $I:Odom:report:*:pos:1234,567,1570;vel:250,12#          */
        char buf[96];
        snprintf(buf, sizeof(buf),
                 "pos:%ld,%ld,%ld;vel:%ld,%ld",
                 x_mm, y_mm, theta_mrad, v_mm_s, omega_mrad_s);
        sendInfo("Odom", "report", "*", buf);

        cfg_odom_pendingReport = false;
        lastPosReportTs = now; /* évite un double envoi position juste après */
        lastVelReportTs = now; /* évite un double envoi vitesse juste après  */
    }

    /* ----------------------------------------------------------------
     * ÉTAPE 3 : ENVOI PÉRIODIQUE POSITION
     * Cadence : cfg_odom_freq_pos_ms (ex: 200ms = 5Hz)
     * FORMAT : $I:Odom:pos:*:x_mm,y_mm,theta_mrad#
     * ---------------------------------------------------------------- */
    if (cfg_odom_freq_pos_ms > 0 && (now - lastPosReportTs) >= cfg_odom_freq_pos_ms)
    {
        long x_mm = (long)(xR * 1000.0);                /* m  → mm    */
        long y_mm = (long)(yR * 1000.0);                /* m  → mm    */
        long theta_mrad = (long)(orientation * 1000.0); /* rad → mrad */

        char buf[48];
        snprintf(buf, sizeof(buf), "%ld,%ld,%ld", x_mm, y_mm, theta_mrad);
        sendInfo("Odom", "pos", "*", buf);

        lastPosReportTs = now;
    }

    /* ----------------------------------------------------------------
     * ÉTAPE 4 : ENVOI PÉRIODIQUE VITESSE
     * Cadence : cfg_odom_freq_vel_ms (ex: 40ms = 25Hz)
     * FORMAT : $I:Odom:vel:*:v_mm_s,omega_mrad_s#
     * ---------------------------------------------------------------- */
    if (cfg_odom_freq_vel_ms > 0 && (now - lastVelReportTs) >= cfg_odom_freq_vel_ms)
    {
        long v_mm_s = (long)(last_v * 1000.0);           /* m/s   → mm/s   */
        long omega_mrad_s = (long)(last_omega * 1000.0); /* rad/s → mrad/s */

        char buf[32];
        snprintf(buf, sizeof(buf), "%ld,%ld", v_mm_s, omega_mrad_s);
        sendInfo("Odom", "vel", "*", buf);

        lastVelReportTs = now;
    }
}

/* ================================================================== */
/* ACCÈS À LA POSE (lecture externe)                                  */
/* ================================================================== */

void odom_getPose(double *out_x, double *out_y, double *out_theta)
{
    if (out_x)
        *out_x = xR;
    if (out_y)
        *out_y = yR;
    if (out_theta)
        *out_theta = orientation;
}

/* ================================================================== */
/* CONTRÔLE ACTIVATION                                                */
/* ================================================================== */

void odom_setActive(bool on)
{
    cfg_odom_active = on;
}

/* ================================================================== */
/* RAPPORT ONE-SHOT (demande externe)                                 */
/* ================================================================== */

void odom_requestReport()
{
    cfg_odom_pendingReport = true;
}

/* ================================================================== */
/* CONFIGURATION PHYSIQUE COMPLÈTE — paraOdom                         */
/* ================================================================== */

/**
 * odom_setPhysical()
 * Voir déclaration dans odom.h pour description complète.
 *
 * Séquence recommandée depuis SP pour calibration :
 *   $V:Odom:*:act:0#
 *   $V:Odom:*:paraOdom:wheel_mm,track_mm,cpr,gainG,gainD,fCalc,fPos,fVel#
 *   $V:Odom:*:reset:1#
 *   $V:Odom:*:act:1#
 */
void odom_setPhysical(int wheel_mm, int track_mm, int cpr,
                      int gainG_x1000, int gainD_x1000,
                      unsigned long freqCalc_ms,
                      unsigned long freqPos_ms,
                      unsigned long freqVel_ms)
{
    /* Mise à jour des paramètres physiques (0 = garder la valeur actuelle) */
    if (wheel_mm > 0)
        cfg_odo_wheel_diameter_m = wheel_mm / 1000.0;
    if (track_mm > 0)
        cfg_odo_track_m = track_mm / 1000.0;
    if (cpr > 0)
        cfg_odo_encoder_cpr = cpr;
    if (gainG_x1000 > 0)
        cfg_odo_left_gain_x1000 = gainG_x1000;
    if (gainD_x1000 > 0)
        cfg_odo_right_gain_x1000 = gainD_x1000;

    /* Mise à jour des fréquences (0 = garder la valeur actuelle) */
    if (freqCalc_ms > 0)
        cfg_odom_period_ms = freqCalc_ms;
    if (freqPos_ms > 0)
        cfg_odom_freq_pos_ms = freqPos_ms;
    if (freqVel_ms > 0)
        cfg_odom_freq_vel_ms = freqVel_ms;

    /* Recalcul des coefficients sans toucher à la pose courante */
    _recompute_coeffs_from_physical();

    /* Envoi VPIV de confirmation avec TOUTES les valeurs effectives
     * (y compris celles non modifiées, pour que SP ait l'état réel) */
    char buf[80];
    snprintf(buf, sizeof(buf), "%d,%d,%d,%d,%d,%lu,%lu,%lu",
             (int)(cfg_odo_wheel_diameter_m * 1000.0), /* wheel_mm  */
             (int)(cfg_odo_track_m * 1000.0),          /* track_mm  */
             cfg_odo_encoder_cpr,                      /* cpr       */
             cfg_odo_left_gain_x1000,                  /* gainG     */
             cfg_odo_right_gain_x1000,                 /* gainD     */
             cfg_odom_period_ms,                       /* fCalc_ms  */
             cfg_odom_freq_pos_ms,                     /* fPos_ms   */
             cfg_odom_freq_vel_ms);                    /* fVel_ms   */
    sendInfo("Odom", "paraOdom", "*", buf);
}

/* ================================================================== */
/* SURCHARGE DIRECTE DES COEFFICIENTS (mode expert)                   */
/* ================================================================== */

void odom_setCoefficients(double gLong_m_per_tick, double dLong_m_per_tick,
                          double gAngl_rad_per_m, double dAngl_rad_per_m)
{
    coeffGLong = gLong_m_per_tick;
    coeffDLong = dLong_m_per_tick;
    coeffGAngl = gAngl_rad_per_m;
    coeffDAngl = dAngl_rad_per_m;

    /* Répercussion dans config pour cohérence */
    cfg_odo_coeff_gLong = coeffGLong;
    cfg_odo_coeff_dLong = coeffDLong;
    cfg_odo_coeff_gAngl = coeffGAngl;
    cfg_odo_coeff_dAngl = coeffDAngl;
}

/* ================================================================== */
/* RÉCEPTION DONNÉES CAPTEURS EXTERNES (fusion future)                */
/* ================================================================== */

void odom_receiveGyro(float gyro_z_dps, unsigned long ts_ms)
{
    /* Mémorise la vitesse angulaire du gyroscope SE (deg/s)
     * → sera utilisée dans une version future pour corriger la dérive
     *   angulaire de l'odométrie (fusion encodeurs + gyro)             */
    cfg_odo_lastGyro_dps = gyro_z_dps;
    cfg_odo_lastGyro_ts_ms = ts_ms;
}

void odom_receiveCompass(float heading_deg, int quality)
{
    /* Mémorise le cap boussole SE (0–360°, 0=Nord)
     * → sera utilisée pour corriger la dérive long terme de l'orientation */
    cfg_odo_lastCompass_deg = heading_deg;
    cfg_odo_lastCompass_quality = quality;
}