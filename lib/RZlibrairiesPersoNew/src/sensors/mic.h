/************************************************************
 * FICHIER  : src/sensors/mic.h
 * CHEMIN   : arduino/mega/src/sensors/mic.h
 * VERSION  : 2.1  —  Mars 2026
 * AUTEUR   : Vincent Philippe
 *
 * RÔLE
 * ----
 *   Interface publique du module microphone (Mic).
 *   Gère l'acquisition sonore non-bloquante sur 3 micros
 *   (Avant=0, Gauche=1, Droite=2) et publie des informations
 *   vers SP selon deux familles de modes configurables.
 *
 * ============================================================
 * DEUX FAMILLES DE MODES INDÉPENDANTS
 * ============================================================
 *
 * modeMicsF — MODE FLUX (un seul actif à la fois) : CAT=F
 * ---------------------------------------------------------
 *   OFF   : aucune publication flux
 *   MOY   : niveau sonore moyen (RMS des 3 canaux) → micMoy
 *   ANGLE : angle d'origine du son (-90..+90°)     → micAngle
 *   PIC   : valeur crête parmi les 3 canaux        → micPIC
 *
 *   Anti-bruit flux : publication seulement si variation
 *   depuis la dernière valeur publiée ≥ deltaSeuil (MOY/PIC)
 *   ou ≥ deltaAngle (ANGLE). Économise les échanges MQTT.
 *
 * modeMicsI — MODE ÉVÉNEMENT (exclusif) : CAT=I
 * -----------------------------------------------
 *   OFF            : aucune détection événement
 *
 *   InfoNuisce     : surveillance du niveau sonore par catégorie
 *     Logique de décision sur le niveau mesuré (RMS moyen) :
 *       < seuil0Parasite              → ignoré (bruit matériel)
 *       seuil0Parasite ≤ niv < seuil1Bruit → espace mort logique (silencieux)
 *       seuil1Bruit ≤ niv < seuil2Bruit   → publie bruitNiv1 si soutenu
 *       niv ≥ seuil2Bruit                 → publie bruitNiv2 si soutenu
 *     "soutenu" = signal reste dans la zone pendant dureeNuisce ms.
 *
 *   DetectPresence : détection de présence tout-ou-rien
 *     Logique de décision :
 *       < seuil0Parasite              → ignoré
 *       seuil0Parasite ≤ niv < seuilPresence → espace mort logique
 *       niv ≥ seuilPresence                → publie detectPresenceSon si soutenu
 *     "soutenu" = signal reste au-dessus de seuilPresence pendant dureePresence ms.
 *
 * ============================================================
 * NOTION DE COOLDOWN (délai de re-signalement)
 * ============================================================
 *   Après un signalement (bruitNiv1, bruitNiv2, detectPresenceSon),
 *   un délai minimum (cooldown ms) est imposé avant de pouvoir
 *   re-publier le même événement. Ce délai évite les rafales de
 *   messages si le signal reste durablement au-dessus du seuil.
 *   Exemple : cooldown=2000ms → au maximum 1 signalement toutes les 2s.
 *   Le cooldown est le même pour tous les types d'événements.
 *
 * ============================================================
 * NOTION DE DÉTECTION SOUTENUE (durée soutenue)
 * ============================================================
 *   Pour éviter les faux positifs (claquement isolé, choc bref),
 *   le signalement n'est émis que si le signal reste dans la zone
 *   de déclenchement de façon CONTINUE pendant dureeNuisce ms ou
 *   dureePresence ms. Si le signal repasse sous le seuil, le
 *   compteur repart de zéro. Coût CPU : une comparaison de millis().
 *
 * ============================================================
 * PARAMÉTRAGE (FORMAT_VALUE objet CSV, voir dispatch_Mic.cpp)
 * ============================================================
 *   paraMicsF :
 *     "freqMoy,freqPic,winMoy,winPic,deltaSeuil,deltaAngle"
 *
 *   paraMicsI :
 *     "seuil0Parasite,seuil1Bruit,seuil2Bruit,seuilPresence,
 *      dureeNuisce,dureePresence,cooldown"
 *
 *     seuil0Parasite  : plancher commun (parasites matériels HW)
 *     seuil1Bruit     : déclenchement bruitNiv1 (InfoNuisce)
 *     seuil2Bruit     : déclenchement bruitNiv2 (InfoNuisce)
 *     seuilPresence   : déclenchement detectPresenceSon
 *     dureeNuisce(ms) : durée soutenue requise pour InfoNuisce
 *     dureePresence(ms): durée soutenue requise pour DetectPresence
 *     cooldown  (ms)  : délai minimum entre deux signalements
 *
 * ============================================================
 * PROPRIÉTÉS VPIV PUBLIÉES (A->SP)
 * ============================================================
 *   micMoy             (F) : niveau moyen RMS — si modeMicsF=MOY
 *   micAngle           (F) : angle source sonore — si modeMicsF=ANGLE
 *   micPIC             (F) : valeur crête — si modeMicsF=PIC
 *   bruitNiv1          (I) : bruit fort détecté (InfoNuisce)
 *   bruitNiv2          (I) : bruit très fort détecté (InfoNuisce)
 *   detectPresenceSon  (I) : présence sonore détectée (DetectPresence)
 *
 * ============================================================
 * ARTICULATION AVEC LES AUTRES MODULES
 * ============================================================
 *   dispatch_Mic.cpp → mic.h / mic.cpp
 *   mic.cpp          → mic_hardware.h / mic_hardware.cpp
 *   mic_hardware     : échantillonnage ADC non-bloquant,
 *                      calcul RMS / PIC / RAW par fenêtre temporelle.
 *                      Variables publiques : mic_lastRMS[], mic_lastPeak[]
 *
 * PRÉCAUTIONS
 * -----------
 *   - mic_processPeriodic() DOIT être appelé à chaque tour de loop()
 *   - Envoyer paraMicsI AVANT d'activer modeMicsI (seuils cohérents)
 *   - seuil0Parasite < seuil1Bruit < seuil2Bruit (InfoNuisce)
 *   - seuil0Parasite < seuilPresence (DetectPresence)
 ************************************************************/

#ifndef MIC_H
#define MIC_H

#include <Arduino.h>

/* ----------------------------------------------------------------
 * Constantes — modeMicsF (mode flux, un seul actif à la fois)
 * ---------------------------------------------------------------- */
#define MIC_MODE_F_OFF 0   // aucune publication flux
#define MIC_MODE_F_MOY 1   // niveau moyen RMS → micMoy
#define MIC_MODE_F_ANGLE 2 // angle source sonore → micAngle
#define MIC_MODE_F_PIC 3   // valeur crête → micPIC

/* ----------------------------------------------------------------
 * Constantes — modeMicsI (mode événement, exclusif)
 * ---------------------------------------------------------------- */
#define MIC_MODE_I_OFF 0      // aucune détection
#define MIC_MODE_I_NUISCE 1   // InfoNuisce : surveillance niveau sonore
#define MIC_MODE_I_PRESENCE 2 // DetectPresence : présence sonore

/* ================================================================
 * INITIALISATION — appeler depuis setup()
 * ================================================================ */
void mic_init();

/* ================================================================
 * BOUCLE PÉRIODIQUE — appeler à CHAQUE tour de loop()
 *   Non-bloquant. Échantillonnage + publications VPIV.
 * ================================================================ */
void mic_processPeriodic();

/* ================================================================
 * CONFIGURATION DES MODES (appelé depuis dispatch_Mic.cpp)
 * ================================================================ */

/* Mode flux : MIC_MODE_F_OFF | MOY | ANGLE | PIC
 * Met à jour automatiquement la fenêtre hardware selon le mode */
void mic_setModeF(uint8_t mode);

/* Mode événement : MIC_MODE_I_OFF | NUISCE | PRESENCE (exclusif) */
void mic_setModeI(uint8_t mode);

/* ================================================================
 * PARAMÉTRAGE FLUX — paraMicsF
 *   freqMoy     (ms) : cadence publication micMoy et micAngle
 *   freqPic     (ms) : cadence publication micPIC
 *   winMoy      (ms) : fenêtre hardware pour MOY/ANGLE
 *   winPic      (ms) : fenêtre hardware pour PIC
 *   deltaSeuil       : variation min pour publier MOY ou PIC (unités ADC)
 *   deltaAngle  (°)  : variation min d'angle pour publier micAngle
 * ================================================================ */
void mic_setParaF(unsigned long freqMoy, unsigned long freqPic,
                  unsigned long winMoy, unsigned long winPic,
                  int deltaSeuil, int deltaAngle);

/* ================================================================
 * PARAMÉTRAGE ÉVÉNEMENTS — paraMicsI
 *
 *   seuil0Parasite   : plancher commun matériel — en dessous : signal ignoré
 *   seuil1Bruit      : déclenchement bruitNiv1 (InfoNuisce)
 *   seuil2Bruit      : déclenchement bruitNiv2 (InfoNuisce)
 *   seuilPresence    : déclenchement detectPresenceSon (DetectPresence)
 *   dureeNuisce (ms) : durée soutenue requise avant signalement InfoNuisce
 *   dureePresence(ms): durée soutenue requise avant signalement DetectPresence
 *   cooldown    (ms) : délai minimum entre deux signalements du même type
 *                      (voir définition complète dans l'entête du fichier)
 * ================================================================ */
void mic_setParaI(int seuil0Parasite,
                  int seuil1Bruit, int seuil2Bruit,
                  int seuilPresence,
                  unsigned long dureeNuisce,
                  unsigned long dureePresence,
                  unsigned long cooldown);

#endif // MIC_H