/************************************************************
 * FICHIER  : src/actuators/mtr.cpp
 * CHEMIN   : arduino/mega/src/actuators/mtr.cpp
 * VERSION  : 2.0  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Logique haute-niveau du module moteurs.
 *   Convertit les commandes (v, omega) en consignes roues (L, R)
 *   et délègue l'envoi à la couche matérielle (mtr_hardware).
 *
 * PRINCIPE DU CALCUL DIFFÉRENTIEL
 * ---------------------------------
 *   Les entrées v (translation) et omega (rotation) sont normalisées
 *   sur l'échelle [-inputScale .. +inputScale] (défaut : -100..+100).
 *
 *   Normalisation :
 *     vNorm   = v     / inputScale   (flottant -1.0 .. +1.0)
 *     omNorm  = omega / inputScale
 *
 *   Calcul des vitesses roues en unités de sortie [-outputScale..+outputScale]
 *   (défaut outputScale = 400 = MOTOR_MAX côté UNO) :
 *     L = (vNorm - omNorm × kturn) × outputScale
 *     R = (vNorm + omNorm × kturn) × outputScale
 *
 *   Exemple avec inputScale=100, outputScale=400, kturn=0.8 :
 *     v=80, omega=0   → L=320,  R=320  (avance droit)
 *     v=0,  omega=50  → L=-160, R=160  (tourne droite sur place)
 *     v=60, omega=30  → L=144,  R=336  (avance en virant droite)
 *
 * FACTEUR DE SCALING (mvt_safe)
 * -------------------------------
 *   mtr_globalScale (0.0..1.0) multiplie L et R avant envoi.
 *   Utilisé par mvt_safe.cpp pour ralentir le robot en zone dangereuse
 *   sans modifier les consignes cibles (qui restent intactes).
 *   Exemple : scale=0.5 → le robot avance à moitié de la vitesse demandée.
 *
 * OVERRIDE STOP (urgence logicielle)
 * ------------------------------------
 *   mtr_forcedStop = true → processPeriodic() envoie <0,0,0> en boucle
 *   et setTargetsSigned() ne fait rien (commandes ignorées).
 *   Levé uniquement par mtr_clearOverride().
 *   Distinct de urg_stopAllMotors() qui agit au niveau hardware.
 *
 * KEEPALIVE PÉRIODIQUE
 * ----------------------
 *   L'UNO possède un timeout safety (1000ms) : si la Mega n'envoie
 *   pas de trame pendant plus de 1s, l'UNO ralentit progressivement
 *   vers 0. mtr_processPeriodic() réémet les consignes courantes
 *   toutes les MTR_KEEPALIVE_MS (100ms) pour maintenir le mouvement.
 *
 * ARTICULATION
 * ------------
 *   dispatch_Mtr.cpp → mtr.cpp → mtr_hardware.cpp → UNO (Serial)
 *   mvt_safe.cpp     → mtr_scaleTargets(), mtr_overrideStop()
 *   ctrl_L.cpp       → mtr_setTargetsSigned()
 *
 * PRÉCAUTIONS
 * -----------
 *   - Ne pas utiliser String() — risque de fragmentation mémoire heap
 *   - cfg_mtr_inputScale et cfg_mtr_outputScale ne doivent pas être 0
 ************************************************************/

#include "mtr.h"
#include "hardware/mtr_hardware.h"
#include "config/config.h"
#include "communication/communication.h"

#include <Arduino.h>
#include <math.h>

/* ================================================================
 * CONSTANTES INTERNES
 * ================================================================ */

/* Intervalle de renvoi périodique des consignes vers l'UNO.
 * L'UNO a un timeout safety de 1000ms → on réémet toutes les 100ms.
 * Valeur choisie : 10× plus courte que le timeout = marge confortable. */
static const unsigned long MTR_KEEPALIVE_MS = 100UL;

/* ================================================================
 * ÉTAT INTERNE
 * ================================================================ */
static int currentL = 0; // dernières valeurs L effectivement envoyées
static int currentR = 0;
static unsigned long lastSendTs = 0; // horodatage du dernier envoi

/* Facteur de réduction global (0.0=arrêt total, 1.0=vitesse normale).
 * Modifié par mtr_scaleTargets() depuis mvt_safe.cpp. */
static float mtr_globalScale = 1.0f;

/* Arrêt forcé logiciel — true = ignore setTargetsSigned, envoie 0,0,0.
 * Distinct du hook matériel urg_stopAllMotors(). */
static bool mtr_forcedStop = false;

/* ================================================================
 * UTILITAIRES INTERNES
 * ================================================================ */

/* Clamp entier dans [lo, hi] */
static inline int clampInt(int v, int lo, int hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

/* Calcul différentiel : (v, omega) → (L, R) en unités de sortie.
 *
 * Entrées normalisées sur [-inputScale..+inputScale].
 * Sorties clampées sur [-outputScale..+outputScale].
 *
 * Le signe de omega définit le sens de rotation :
 *   omega > 0 → R accélère, L ralentit → tourne à droite
 *   omega < 0 → L accélère, R ralentit → tourne à gauche */
static void computeDiffDrive(int vin, int omegaIn, int *outL, int *outR)
{
    // Normalisation sur [-1.0 .. +1.0]
    float vNorm = (float)vin / (float)cfg_mtr_inputScale;
    float omNorm = (float)omegaIn / (float)cfg_mtr_inputScale;

    float scale = (float)cfg_mtr_outputScale;

    // Formule différentielle avec coefficient de rotation kturn
    float Lf = (vNorm - omNorm * cfg_mtr_kturn) * scale;
    float Rf = (vNorm + omNorm * cfg_mtr_kturn) * scale;

    // Arrondi et clamp dans la plage de sortie
    int maxOut = cfg_mtr_outputScale;
    *outL = clampInt((int)round(Lf), -maxOut, maxOut);
    *outR = clampInt((int)round(Rf), -maxOut, maxOut);
}

/* Envoie effectivement les consignes à l'UNO (avec application du scaling).
 * Centralise l'envoi hardware pour éviter la duplication de code. */
static void sendToHardware(int L, int R, int A)
{
    // Application du facteur de réduction global
    int sendL = (int)round(L * mtr_globalScale);
    int sendR = (int)round(R * mtr_globalScale);

    mtr_hw_updateTrame(sendL, sendR, A);
    mtr_hw_send();

    currentL = sendL;
    currentR = sendR;
    lastSendTs = millis();
}

/* ================================================================
 * INITIALISATION — appeler depuis setup()
 * ================================================================ */
void mtr_init()
{
    mtr_hw_init();

    // Remise à zéro de l'état interne
    currentL = 0;
    currentR = 0;
    mtr_globalScale = 1.0f;
    mtr_forcedStop = false;
    lastSendTs = millis();

    // Initialisation des consignes config
    cfg_mtr_targetL = 0;
    cfg_mtr_targetR = 0;
    cfg_mtr_targetA = 2; // lissage moyen par défaut
}

/* ================================================================
 * COMMANDE PRINCIPALE — pilotage différentiel
 *
 *   Calcule L,R depuis v et omega, stocke dans cfg, envoie à l'UNO.
 *   Ignoré si override stop actif ou si module inactif.
 * ================================================================ */
void mtr_setTargetsSigned(int v, int omega, int accelLevel)
{
    if (mtr_forcedStop)
        return; // override actif → commande ignorée

    // Clamp des entrées dans les plages autorisées
    v = clampInt(v, -cfg_mtr_inputScale, cfg_mtr_inputScale);
    omega = clampInt(omega, -cfg_mtr_inputScale, cfg_mtr_inputScale);
    accelLevel = clampInt(accelLevel, 0, 4);

    // Calcul différentiel → L, R en unités de sortie
    int L, R;
    computeDiffDrive(v, omega, &L, &R);

    // Stockage des consignes cibles (utilisées par processPeriodic et mvt_safe)
    cfg_mtr_targetL = L;
    cfg_mtr_targetR = R;
    cfg_mtr_targetA = accelLevel;

    if (!cfg_mtr_active)
        return; // module désactivé → calcul fait, mais pas d'envoi hardware

    // Envoi immédiat à l'UNO
    sendToHardware(L, R, accelLevel);

    // Confirmation VPIV
    char buf[32];
    snprintf(buf, sizeof(buf), "%d,%d,%d", L, R, accelLevel);
    sendInfo("Mtr", "targets", "*", buf); // $I:Mtr:targets:*:L,R,A#
}

/* ================================================================
 * ARRÊT PROPRE
 *   Remet toutes les consignes à 0 et envoie une seule trame <0,0,0>.
 * ================================================================ */
void mtr_stopAll()
{
    cfg_mtr_targetL = 0;
    cfg_mtr_targetR = 0;
    cfg_mtr_targetA = 0; // changement direct → arrêt immédiat sans lissage

    // Envoi d'une seule trame (pas de duplication legacy/modern)
    mtr_hw_updateTrame(0, 0, 0);
    mtr_hw_send();
    currentL = currentR = 0;
    lastSendTs = millis();

    sendInfo("Mtr", "stop", "*", "OK");
}

/* ================================================================
 * SÉCURITÉS
 * ================================================================ */

/* Arrêt forcé immédiat — bloque toutes les commandes suivantes.
 * Les commandes setTargetsSigned() seront ignorées jusqu'à clearOverride(). */
void mtr_overrideStop()
{
    mtr_forcedStop = true;

    mtr_hw_updateTrame(0, 0, 0);
    mtr_hw_send();
    currentL = currentR = 0;
    lastSendTs = millis();

    sendInfo("Mtr", "override", "*", "stop");
}

/* Lève l'arrêt forcé — les commandes sont à nouveau acceptées.
 * Note : les consignes cibles (cfg_mtr_targetL/R) sont toujours en mémoire
 * → le robot reprend sur les dernières consignes reçues avant l'override.
 * Si ce comportement n'est pas souhaité, appeler mtr_stopAll() avant clearOverride(). */
void mtr_clearOverride()
{
    mtr_forcedStop = false;
    sendInfo("Mtr", "override", "*", "cleared");
}

/* mtr_scaleTargets — applique le facteur de réduction des vitesses.
 *
 * Paramètres :
 *   s      (float) : facteur 0.0..1.0 — utilisé UNIQUEMENT pour le calcul interne.
 *                    Jamais publié dans le VPIV (règle pas-de-float).
 *   s_int  (int)   : valeur ×1000 telle que reçue depuis le VPIV (0..1000).
 *                    Publiée telle quelle dans l'ACK VPIV.
 *                    Exemples : s=0.5→s_int=500, s=0.75→s_int=750, s=1.0→s_int=1000.
 *
 * Effet de bord :
 *   - mtr_globalScale mis à jour → appliqué dans sendToHardware() à chaque envoi.
 *   - ACK $I:Mtr:scale:*:<s_int># publié (entier ×1000, pas de float). */
void mtr_scaleTargets(float s, int s_int)
{
    // Clamp float interne (défense en profondeur — dispatcher a déjà clampé)
    mtr_globalScale = clampInt((int)(s * 1000), 0, 1000) / 1000.0f;

    // ACK VPIV en entier ×1000, conforme règle pas-de-float-dans-VPIV
    // Ex : s=0.75 → s_int=750 → "$I:Mtr:scale:*:750#"
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", s_int);
    sendInfo("Mtr", "scale", "*", buf);
}

/* mtr_setKturn — règle le coefficient de rotation du calcul différentiel.
 *
 * Paramètres :
 *   k      (float) : coefficient 0.0..2.0 — stocké dans cfg_mtr_kturn, utilisé
 *                    dans computeDiffDrive(). Jamais publié dans le VPIV.
 *   k_int  (int)   : valeur ×1000 telle que reçue depuis le VPIV (0..2000).
 *                    Publiée telle quelle dans l'ACK VPIV.
 *
 * Effet de bord :
 *   - cfg_mtr_kturn mis à jour → effet immédiat sur le prochain
 *     appel de mtr_setTargetsSigned() via computeDiffDrive().
 *   - ACK $I:Mtr:kturn:*:<k_int># publié (entier ×1000, pas de float).
 *
 * Valeurs VPIV typiques :
 *   500=rotation douce  800=standard confort  1000=standard  1500=rotation franche */
void mtr_setKturn(float k, int k_int)
{
    // Clamp float interne (défense en profondeur — dispatcher a déjà clampé)
    if (k < 0.0f)
        k = 0.0f;
    if (k > 2.0f)
        k = 2.0f;

    cfg_mtr_kturn = k; // utilisé par computeDiffDrive()

    // ACK VPIV en entier ×1000, conforme règle pas-de-float-dans-VPIV
    // Ex : k=0.8 → k_int=800 → "$I:Mtr:kturn:*:800#"
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", k_int);
    sendInfo("Mtr", "kturn", "*", buf);
}

/* ================================================================
 * BOUCLE PÉRIODIQUE — appeler depuis loop() à chaque tour
 *
 *   Deux rôles :
 *     1. KEEPALIVE : réémet les consignes courantes toutes les
 *        MTR_KEEPALIVE_MS (100ms) pour éviter le timeout UNO (1000ms)
 *     2. OVERRIDE : si mtr_forcedStop est actif, maintient l'envoi
 *        de <0,0,0> au même rythme (keepalive de l'arrêt)
 * ================================================================ */
void mtr_processPeriodic()
{
    if (!cfg_mtr_active)
        return;

    unsigned long now = millis();
    if (now - lastSendTs < MTR_KEEPALIVE_MS)
        return; // pas encore le moment de réémettre

    if (mtr_forcedStop)
    {
        // Override actif : maintient <0,0,0> pour que l'UNO ne sorte pas du timeout
        mtr_hw_updateTrame(0, 0, 0);
        mtr_hw_send();
        lastSendTs = now;
        return;
    }

    // Renvoi des consignes cibles courantes (keepalive normal)
    sendToHardware(cfg_mtr_targetL, cfg_mtr_targetR, cfg_mtr_targetA);
}