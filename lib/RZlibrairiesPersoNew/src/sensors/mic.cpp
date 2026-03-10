/************************************************************
 * FICHIER  : src/sensors/mic.cpp
 * CHEMIN   : arduino/mega/src/sensors/mic.cpp
 * VERSION  : 2.1  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Logique haute-niveau du module microphone (3 canaux).
 *   S'appuie sur mic_hardware pour l'échantillonnage ADC non-bloquant.
 *
 * RÉSUMÉ DU FONCTIONNEMENT
 * ------------------------
 *   mic_processPeriodic() est appelé à chaque tour de loop().
 *   Elle réalise 3 étapes dans l'ordre :
 *     1. Délègue l'échantillonnage ADC à mic_hardware (toujours actif)
 *     2. Publication flux selon modeMicsF (si actif)
 *     3. Détection événements selon modeMicsI (si actif)
 *
 * ============================================================
 * VALEURS CALCULÉES PAR mic_hardware
 * ============================================================
 *   mic_lastRMS[i]  : RMS du canal i sur la dernière fenêtre temporelle
 *                     = indicateur d'énergie sonore (niveau moyen)
 *   mic_lastPeak[i] : valeur crête (maximum ADC) sur la fenêtre
 *                     = valeur instantanée la plus haute observée
 *   (i=0:Avant, i=1:Gauche, i=2:Droite)
 *
 * ============================================================
 * CALCUL DU NIVEAU DE RÉFÉRENCE (niveauGlobal)
 * ============================================================
 *   niveauGlobal = (mic_lastRMS[0] + mic_lastRMS[1] + mic_lastRMS[2]) / 3
 *   Moyenne arithmétique des 3 canaux.
 *   Utilisé à la fois pour les publications flux MOY
 *   et pour toutes les comparaisons de seuils (modeMicsI).
 *   Ainsi, les décisions seuils sont basées sur le même calcul
 *   que ce qui est publié en micMoy → cohérence garantie.
 *
 * ============================================================
 * DÉTECTION SOUTENUE — DÉFINITION
 * ============================================================
 *   Un signalement n'est émis que si le signal reste dans sa zone
 *   de déclenchement de façon CONTINUE pendant durée ms.
 *   Si le signal repasse sous le seuil (ou change de zone), le
 *   compteur de durée repart de zéro.
 *   Exemple pour InfoNuisce, zone bruitNiv1 :
 *     t=0ms   → niveau passe dans zone Niv1 → debut compteur
 *     t=200ms → niveau repasse sous seuil1  → RESET compteur
 *     t=300ms → niveau repasse en zone Niv1 → nouveau début
 *     t=800ms → durée soutenue = 500ms ≥ dureeNuisce → signalement !
 *
 * ============================================================
 * COOLDOWN — DÉFINITION
 * ============================================================
 *   Après un signalement (bruitNiv1, bruitNiv2, detectPresenceSon),
 *   un délai minimum cooldown (ms) est imposé avant de re-publier
 *   le MÊME type d'événement. But : éviter une rafale de messages
 *   si le signal reste durablement dans la zone de déclenchement.
 *   Exemple : cooldown=2000ms → au maximum 1 bruitNiv1 par 2 secondes.
 *   Note : bruitNiv1 et bruitNiv2 ont chacun leur propre compteur cooldown.
 *
 * ============================================================
 * LOGIQUE DÉTAILLÉE — modeMicsI InfoNuisce
 * ============================================================
 *   niveauGlobal < seuil0Parasite
 *     → ignoré : parasite matériel (bruit de fond du circuit ADC)
 *
 *   seuil0Parasite ≤ niveauGlobal < seuil1Bruit
 *     → espace mort logique : signal significatif mais en dessous
 *       du seuil de nuisance → rien à signaler
 *
 *   seuil1Bruit ≤ niveauGlobal < seuil2Bruit
 *     → zone bruitNiv1 : commence (ou continue) la durée soutenue
 *       si durée ≥ dureeNuisce ET cooldown écoulé → publie bruitNiv1
 *
 *   niveauGlobal ≥ seuil2Bruit
 *     → zone bruitNiv2 : commence (ou continue) la durée soutenue
 *       si durée ≥ dureeNuisce ET cooldown écoulé → publie bruitNiv2
 *
 *   Changement de zone (ex: niv1→niv2 ou niv2→niv1) :
 *     → RESET du compteur soutenu (on repart à zéro dans la nouvelle zone)
 *
 * ============================================================
 * LOGIQUE DÉTAILLÉE — modeMicsI DetectPresence
 * ============================================================
 *   niveauGlobal < seuil0Parasite
 *     → ignoré
 *
 *   seuil0Parasite ≤ niveauGlobal < seuilPresence
 *     → espace mort logique : présence incertaine, non signalée
 *
 *   niveauGlobal ≥ seuilPresence
 *     → si durée soutenue ≥ dureePresence ET cooldown écoulé
 *       → publie detectPresenceSon
 *
 * ============================================================
 * PARAMÉTRAGE PAR DÉFAUT (valeurs de départ conservatives)
 * ============================================================
 *   modeMicsF = OFF, modeMicsI = OFF
 *   SP doit configurer via paraMicsF et paraMicsI AVANT d'activer les modes.
 *
 * ARTICULATION
 * ------------
 *   dispatch_Mic.cpp → mic.cpp → mic_hardware.cpp → analogRead()
 ************************************************************/

#include "mic.h"
#include "hardware/mic_hardware.h"
#include "config/config.h"
#include "communication/communication.h"
#include <Arduino.h>
#include <string.h>

/* ================================================================
 * ÉTAT DES MODES
 * ================================================================ */
static uint8_t modeF = MIC_MODE_F_OFF; // mode flux actif
static uint8_t modeI = MIC_MODE_I_OFF; // mode événement actif

/* ================================================================
 * PARAMÈTRES FLUX — paraMicsF
 * ================================================================ */
static unsigned long freqMoy_ms = 200; // cadence publication MOY/ANGLE (ms)
static unsigned long freqPic_ms = 100; // cadence publication PIC (ms)
static unsigned long winMoy_ms = 200;  // fenêtre hardware MOY/ANGLE (ms)
static unsigned long winPic_ms = 100;  // fenêtre hardware PIC (ms)
static int deltaSeuil = 5;             // variation min pour publier MOY/PIC
static int deltaAngle = 10;            // variation min d'angle pour publier ANGLE (°)

/* Dernières valeurs publiées — calcul du delta anti-bruit */
static int lastPubMoy = -999;
static int lastPubPic = -999;
static int lastPubAngle = -999;

/* Timers de cadence flux */
static unsigned long tFluxMoy = 0;
static unsigned long tFluxPic = 0;

/* ================================================================
 * PARAMÈTRES ÉVÉNEMENTS — paraMicsI
 * ================================================================ */
static int seuil0Parasite = 30;              // plancher matériel (commun)
static int seuil1Bruit = 300;                // déclenchement bruitNiv1
static int seuil2Bruit = 600;                // déclenchement bruitNiv2
static int seuilPresence = 100;              // déclenchement detectPresenceSon
static unsigned long dureeNuisce_ms = 500;   // durée soutenue InfoNuisce (ms)
static unsigned long dureePresence_ms = 300; // durée soutenue DetectPresence (ms)
static unsigned long cooldown_ms = 2000;     // délai min entre deux signalements (ms)

/* ================================================================
 * VARIABLES D'ÉTAT — détection soutenue InfoNuisce
 *
 *   zoneCourante : zone dans laquelle se trouve le signal actuellement
 *     0 = sous seuil0 ou espace mort (non pertinent)
 *     1 = zone bruitNiv1 (entre seuil1 et seuil2)
 *     2 = zone bruitNiv2 (au-dessus de seuil2)
 *   tDebutZone   : millis() au moment où le signal est entré dans la zone courante
 *   tDernierNiv1 : millis() du dernier signalement bruitNiv1 (cooldown)
 *   tDernierNiv2 : millis() du dernier signalement bruitNiv2 (cooldown)
 * ================================================================ */
static uint8_t zoneCourante = 0;
static unsigned long tDebutZone = 0;
static unsigned long tDernierNiv1 = 0;
static unsigned long tDernierNiv2 = 0;

/* ================================================================
 * VARIABLES D'ÉTAT — détection soutenue DetectPresence
 *
 *   presenceEnCours : true = signal actuellement ≥ seuilPresence
 *   tDebutPresence  : millis() début de la période soutenue
 *   tDernierPresence: millis() du dernier signalement detectPresenceSon (cooldown)
 * ================================================================ */
static bool presenceEnCours = false;
static unsigned long tDebutPresence = 0;
static unsigned long tDernierPresence = 0;

/* ================================================================
 * LISSAGE ANGLE (filtre du 1er ordre)
 * ================================================================ */
static int angleLisse = 0;

/* ================================================================
 * FONCTIONS DE CALCUL INTERNES
 * ================================================================ */

/* Niveau global = moyenne RMS des 3 canaux.
 * Base de toutes les comparaisons seuils et de la publication micMoy.
 * Garantit cohérence entre ce qui est publié et ce qui est comparé. */
static int niveauGlobal()
{
    return (mic_lastRMS[0] + mic_lastRMS[1] + mic_lastRMS[2]) / 3;
}

/* Valeur crête = maximum des pics des 3 canaux */
static int picGlobal()
{
    int p = mic_lastPeak[0];
    if (mic_lastPeak[1] > p)
        p = mic_lastPeak[1];
    if (mic_lastPeak[2] > p)
        p = mic_lastPeak[2];
    return p;
}

/* Angle d'origine sonore (-90..+90°)
 * Méthode barycentrique : angle = Σ(poids_i × angle_i) / Σ(poids_i)
 * Canal 0=Avant(0°), Canal 1=Gauche(-90°), Canal 2=Droite(+90°)
 * +1 sur chaque canal : biais de stabilité si tous les niveaux sont nuls
 * Lissage 1er ordre : 75% historique + 25% instantané → fluidité */
static int angleGlobal()
{
    int r0 = mic_lastRMS[0] + 1;
    int r1 = mic_lastRMS[1] + 1;
    int r2 = mic_lastRMS[2] + 1;

    long num = (long)r0 * 0 + (long)r1 * (-90) + (long)r2 * 90;
    long den = r0 + r1 + r2;
    if (den == 0)
        return angleLisse;

    int raw = (int)(num / den);
    angleLisse = (angleLisse * 3 + raw) / 4; // filtre 1er ordre
    return angleLisse;
}

/* ================================================================
 * INITIALISATION
 * ================================================================ */
void mic_init()
{
    mic_initHardware();
    // Modes OFF par défaut : SP configure via VPIV avant d'activer
}

/* ================================================================
 * CONFIGURATION DES MODES
 * ================================================================ */

/* Mode flux : adapte automatiquement la fenêtre hardware au mode */
void mic_setModeF(uint8_t mode)
{
    modeF = mode;

    // Fenêtre hardware adaptée au mode actif :
    //   PIC   → fenêtre courte pour ne pas rater les transitoires
    //   MOY/ANGLE → fenêtre longue pour un niveau stable et lissé
    switch (mode)
    {
    case MIC_MODE_F_PIC:
        cfg_mic_win_ms = winPic_ms;
        break;
    default: // MOY, ANGLE, OFF
        cfg_mic_win_ms = winMoy_ms;
        break;
    }

    // Réinitialise les deltas pour éviter une suppression au démarrage du mode
    lastPubMoy = -999;
    lastPubPic = -999;
    lastPubAngle = -999;
}

/* Mode événement : exclusif (pas de combinaison) — réinitialise l'état */
void mic_setModeI(uint8_t mode)
{
    modeI = mode;
    // Reset de tous les traqueurs de détection soutenue
    zoneCourante = 0;
    presenceEnCours = false;
}

/* Paramètres flux */
void mic_setParaF(unsigned long freqMoy, unsigned long freqPic,
                  unsigned long winMoy, unsigned long winPic,
                  int delta, int deltaAng)
{
    freqMoy_ms = freqMoy;
    freqPic_ms = freqPic;
    winMoy_ms = winMoy;
    winPic_ms = winPic;
    deltaSeuil = delta;
    deltaAngle = deltaAng;

    // Recalcule la fenêtre hardware avec les nouvelles valeurs
    mic_setModeF(modeF);
}

/* Paramètres événements */
void mic_setParaI(int s0, int s1, int s2, int sP,
                  unsigned long dNuisce, unsigned long dPresence,
                  unsigned long cooldown)
{
    seuil0Parasite = s0;
    seuil1Bruit = s1;
    seuil2Bruit = s2;
    seuilPresence = sP;
    dureeNuisce_ms = dNuisce;
    dureePresence_ms = dPresence;
    cooldown_ms = cooldown;
}

/* ================================================================
 * BOUCLE PÉRIODIQUE — appeler depuis loop() à chaque tour
 * ================================================================ */
void mic_processPeriodic()
{
    unsigned long now = millis();

    /* ----------------------------------------------------------
     * ÉTAPE 1 — Échantillonnage hardware
     *   Toujours actif, quel que soit le mode.
     *   Coût minimal : 1 analogRead par micro par appel.
     *   Les tableaux mic_lastRMS[] et mic_lastPeak[] sont mis à
     *   jour à chaque fin de fenêtre temporelle.
     * ---------------------------------------------------------- */
    mic_sampler_pollAll();

    /* ----------------------------------------------------------
     * ÉTAPE 2 — Publications flux (modeMicsF)
     * ---------------------------------------------------------- */

    /* --- Mode MOY : niveau sonore moyen --- */
    if (modeF == MIC_MODE_F_MOY && (now - tFluxMoy >= freqMoy_ms))
    {
        tFluxMoy = now;
        int val = niveauGlobal();

        // Anti-bruit : publie seulement si variation ≥ deltaSeuil
        if (abs(val - lastPubMoy) >= deltaSeuil)
        {
            lastPubMoy = val;
            char buf[12];
            snprintf(buf, sizeof(buf), "%d", val);
            sendInfo("Mic", "micMoy", "*", buf); // CAT=F
        }
    }

    /* --- Mode ANGLE : direction de la source sonore --- */
    if (modeF == MIC_MODE_F_ANGLE && (now - tFluxMoy >= freqMoy_ms))
    {
        tFluxMoy = now;
        int val = angleGlobal();

        // Anti-bruit : publie seulement si variation ≥ deltaAngle
        if (abs(val - lastPubAngle) >= deltaAngle)
        {
            lastPubAngle = val;
            char buf[12];
            snprintf(buf, sizeof(buf), "%d", val);
            sendInfo("Mic", "micAngle", "*", buf); // CAT=F
        }
    }

    /* --- Mode PIC : valeur crête --- */
    if (modeF == MIC_MODE_F_PIC && (now - tFluxPic >= freqPic_ms))
    {
        tFluxPic = now;
        int val = picGlobal();

        // Anti-bruit : publie seulement si variation ≥ deltaSeuil
        if (abs(val - lastPubPic) >= deltaSeuil)
        {
            lastPubPic = val;
            char buf[12];
            snprintf(buf, sizeof(buf), "%d", val);
            sendInfo("Mic", "micPIC", "*", buf); // CAT=F
        }
    }

    /* ----------------------------------------------------------
     * ÉTAPE 3 — Détection événements (modeMicsI)
     *   Niveau de référence : niveauGlobal() — même calcul que micMoy
     *   pour garantir la cohérence entre flux et seuils.
     * ---------------------------------------------------------- */
    if (modeI == MIC_MODE_I_OFF)
        return; // mode événement inactif → fin de traitement

    int niv = niveauGlobal(); // niveau instantané de référence
    char buf[12];
    snprintf(buf, sizeof(buf), "%d", niv);

    /* ==========================================================
     * Mode InfoNuisce
     * ==========================================================
     * Détermine la zone courante du signal, compare à la durée
     * soutenue, et publie bruitNiv1 ou bruitNiv2 si les conditions
     * sont réunies (durée soutenue + cooldown écoulé).
     * ========================================================== */
    if (modeI == MIC_MODE_I_NUISCE)
    {
        /* Détermination de la zone courante ---
         * zone 0 : parasite ou espace mort → rien à faire
         * zone 1 : bruitNiv1 (entre seuil1 et seuil2)
         * zone 2 : bruitNiv2 (au-dessus de seuil2) */
        uint8_t zone;
        if (niv < seuil0Parasite || niv < seuil1Bruit)
            zone = 0; // parasite ou espace mort logique
        else if (niv < seuil2Bruit)
            zone = 1; // zone bruit fort
        else
            zone = 2; // zone bruit très fort

        if (zone == 0)
        {
            // Signal sous les seuils significatifs : reset compteur
            zoneCourante = 0;
        }
        else if (zone != zoneCourante)
        {
            // Changement de zone : on repart de zéro dans la nouvelle zone
            zoneCourante = zone;
            tDebutZone = now;
        }
        else
        {
            // Même zone maintenue : vérifier si durée soutenue atteinte
            unsigned long duree = now - tDebutZone;

            if (duree >= dureeNuisce_ms)
            {
                if (zone == 1 && (now - tDernierNiv1 >= cooldown_ms))
                {
                    // Bruit fort soutenu et cooldown écoulé → signalement
                    tDernierNiv1 = now;
                    sendInfo("Mic", "bruitNiv1", "*", buf); // CAT=I bruit fort

                    // Reset pour éviter signalement répété sans nouveau cycle
                    zoneCourante = 0;
                }
                else if (zone == 2 && (now - tDernierNiv2 >= cooldown_ms))
                {
                    // Bruit très fort soutenu et cooldown écoulé → signalement
                    tDernierNiv2 = now;
                    sendInfo("Mic", "bruitNiv2", "*", buf); // CAT=I bruit très fort

                    zoneCourante = 0;
                }
            }
        }
    }

    /* ==========================================================
     * Mode DetectPresence
     * ==========================================================
     * Décision tout-ou-rien : signal ≥ seuilPresence (et > seuil0Parasite)
     * Si soutenu ≥ dureePresence ET cooldown écoulé → detectPresenceSon
     * ========================================================== */
    if (modeI == MIC_MODE_I_PRESENCE)
    {
        // Filtre parasite + comparaison seuil présence
        bool audessus = (niv >= seuil0Parasite) && (niv >= seuilPresence);

        if (!audessus)
        {
            // Signal sous le seuil : reset du compteur de présence
            presenceEnCours = false;
        }
        else if (!presenceEnCours)
        {
            // Premier dépassement : début de la période soutenue
            presenceEnCours = true;
            tDebutPresence = now;
        }
        else
        {
            // Signal maintenu au-dessus : vérifier durée et cooldown
            if ((now - tDebutPresence >= dureePresence_ms) &&
                (now - tDernierPresence >= cooldown_ms))
            {
                tDernierPresence = now;
                sendInfo("Mic", "detectPresenceSon", "*", buf); // CAT=I présence

                // Reset pour attendre un nouveau cycle
                presenceEnCours = false;
            }
        }
    }
}