/************************************************************
 * FICHIER : src/sensors/us.cpp
 * ROLE    : Logique haute-niveau pour les sonars (module Us)
 ************************************************************/
#include "us.h"
#include "hardware/us_hardware.h"
#include "system/urg.h"
#include "communication/communication.h"
#include "communication/vpiv_utils.h"

// valeurs internes
static int lastReported[US_NUM_SENSORS];
// Curseur de la passe echelonnee : quel capteur « exploration » vient ensuite.
static uint8_t prochainExplore = 0;
static int currValue[US_NUM_SENSORS];

static unsigned long suspectSince[US_NUM_SENSORS];
static bool sensorDisabled[US_NUM_SENSORS];

static unsigned long lastCycle = 0;

// ----------------------------------------------------------
// Envoi d’une valeur unique : $I:Us:one:<idx>:<val>
// ----------------------------------------------------------
static void send_one(int i, int v)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", v);

    char inst[4];
    snprintf(inst, sizeof(inst), "%d", i);

    sendInfo("US", "one", inst, buf);
}

// ----------------------------------------------------------
// Envoi groupé : $I:Us:val:*:v1,v2,v3…
// ----------------------------------------------------------
static void send_all()
{
    char buf[128];
    buf[0] = 0;

    for (int i = 0; i < US_NUM_SENSORS; i++)
    {
        char t[8];
        snprintf(t, sizeof(t), "%d", sensorDisabled[i] ? -1 : currValue[i]);
        strcat(buf, t);
        if (i < US_NUM_SENSORS - 1)
            strcat(buf, ",");
    }

    sendInfo("US", "val", "*", buf);
}

// ----------------------------------------------------------
// Init
// ----------------------------------------------------------
void us_init()
{
    us_initHardware();

    for (int i = 0; i < US_NUM_SENSORS; i++)
    {
        lastReported[i] = US_UNREPORTED;
        currValue[i] = US_SILENCE;
        suspectSince[i] = 0;
        sensorDisabled[i] = false;
    }

    lastCycle = millis();
}

// ----------------------------------------------------------
// Boucle périodique sonar
// ----------------------------------------------------------
void us_processPeriodic()
{
    if (!cfg_us_active)
        return;

    if (urg_isActive())
        return;

    unsigned long now = millis();
    if (now - lastCycle < cfg_us_ping_cycle_ms)
        return;
    lastCycle = now;

    // ------------------------------------------------------------------
    //  PASSE ECHELONNEE AVEC PRIORITE AU SENS DE MARCHE  (24/10/2026)
    //
    //  AVANT : les neuf capteurs etaient mesures dans la meme passe. Chaque
    //  capteur qui ne voit rien coute le delai complet de pulseIn, soit
    //  9 x 30 ms = 270 ms pour un chien de garde regle a 80 ms. L'urgence se
    //  redeclenchait a chaque passe, et le banc de test ne mesurait plus que
    //  le filtre d'urgence.
    //
    //  MAINTENANT, par passe :
    //    - les TROIS capteurs du sens de marche, delai sécurité (1 m, 6 ms)
    //      Les seuils de danger sont a 10 et 20 cm : un metre est cinq fois
    //      le seuil d'alerte, largement suffisant, et vingt fois moins cher.
    //    - UN SEUL des six autres, a tour de role, portee complete (30 ms)
    //      pour la navigation, qui n'a pas besoin de temps reel.
    //  Cout au pire : 3 x 6 + 30 = 48 ms. Sous le seuil, avec de la marge.
    //
    //  CE QUE CELA NE CHANGE PAS : currValue[] est statique et persistant,
    //  donc les neuf valeurs restent disponibles en aval a tout instant.
    //  Echelonner change la fraicheur, jamais la presence.
    //
    //  Raisonnement complet : docs/decision_US_detection_panne.md
    // ------------------------------------------------------------------
    static const uint8_t US_AVANT[3] = {3, 4, 5};   // AvD, AvC, AvG
    static const uint8_t US_ARRIERE[3] = {0, 1, 2}; // ArG, ArC, ArD

    // Marche arriere : les DEUX consignes negatives. Une rotation sur place
    // (signes opposes) n'est pas une marche arriere — on garde l'avant.
    const bool enArriere = (cfg_mtr_targetL < 0 && cfg_mtr_targetR < 0);
    const uint8_t *prioritaires = enArriere ? US_ARRIERE : US_AVANT;

    // Les indices a mesurer cette passe : les trois prioritaires, plus un
    // seul des six autres, choisi a tour de role.
    uint8_t aMesurer[4];
    unsigned long delais[4];
    uint8_t n = 0;
    for (uint8_t k = 0; k < 3; k++)
    {
        aMesurer[n] = prioritaires[k];
        delais[n++] = US_TIMEOUT_SECURITE_US;
    }
    for (uint8_t essai = 0; essai < US_NUM_SENSORS; essai++)
    {
        uint8_t cand = prochainExplore;
        prochainExplore = (prochainExplore + 1) % US_NUM_SENSORS;
        bool estPrioritaire = (cand == prioritaires[0] ||
                               cand == prioritaires[1] ||
                               cand == prioritaires[2]);
        if (!estPrioritaire)
        {
            aMesurer[n] = cand;
            delais[n++] = US_TIMEOUT_EXPLORATION_US;
            break;
        }
    }

    for (uint8_t k = 0; k < n; k++)
    {
        const int i = aMesurer[k];

        if (sensorDisabled[i])
        {
            currValue[i] = US_DISABLED;
            continue;
        }

        int d = us_measureOnce(i, delais[k]);

        // ----- gestion du mode suspect -----
        //  🔴 CORRIGE le 24/10/2026. La condition etait « if (d < 0) », qui
        //  n'est JAMAIS vraie : us_measureOnce rend un unsigned int, et son
        //  entete documente « 0 si timeout ». La branche entiere — suspicion,
        //  desactivation, US_DISABLED — n'avait donc jamais ete executee.
        //  Consequences avant correction :
        //    - la detection de panne n'existait que sur le papier ;
        //    - US_DISABLED etait inatteignable, donc le blanc de l'ecran
        //      Node-RED ne pouvait pas s'afficher ;
        //    - un timeout tombait en « mesure valide » a 0 cm, donc sous le
        //      seuil de danger : chaque capteur muet emettait une alerte de
        //      danger a chaque passe.
        //  us_handleReadList(), plus bas, testait deja « v == 0 » correctement.
        if (d == US_SILENCE)
        {
            if (suspectSince[i] == 0)
                suspectSince[i] = now;

            if (now - suspectSince[i] >= cfg_us_suspect_timeout_ms)
            {
                sensorDisabled[i] = true;
                currValue[i] = US_DISABLED;
                send_one(i, US_DISABLED);
                lastReported[i] = US_DISABLED;
                continue;
            }

            currValue[i] = US_SILENCE;
            continue;
        }

        // mesure valide
        suspectSince[i] = 0;
        currValue[i] = d;

        // ----- système d’alerte danger -----
        // Conversion complète en uint32_t pour éviter warning signed/unsigned
        uint32_t d_u = static_cast<uint32_t>(d);
        uint32_t dangerThreshold = static_cast<uint32_t>(cfg_us_danger_thresholds[i]) *
                                   static_cast<uint32_t>(cfg_us_alert_multiplier[i]);

        if (d_u <= dangerThreshold)
        {
            int lvl = max(1, cfg_us_danger_thresholds[i] / max(1, d));
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", lvl);
            sendInfo("US", "alert", String(i).c_str(), buf);
        }
    }

    // envoi groupé
    if (cfg_us_send_all)
    {
        send_all();
        for (int i = 0; i < US_NUM_SENSORS; i++)
            lastReported[i] = currValue[i];
        return;
    }

    // envoi des variations significatives
    for (int i = 0; i < US_NUM_SENSORS; i++)
    {
        int c = currValue[i];
        int p = lastReported[i];

        if (p == US_UNREPORTED)
        {
            send_one(i, c);
            lastReported[i] = c;
            continue;
        }

        if (c == US_DISABLED && p != US_DISABLED)
        {
            send_one(i, c);
            lastReported[i] = c;
            continue;
        }

        if (c == US_SILENCE)
            continue;

        if (abs(c - p) >= cfg_us_delta_threshold)
        {
            send_one(i, c);
            lastReported[i] = c;
        }
    }
}

// ----------------------------------------------------------
// Lecture forcée de certaines instances
// ----------------------------------------------------------
void us_handleReadList(const int *idxs, int n)
{
    // ⚠ CAS CONNU, NON CORRIGE le 24/10/2026 — a trancher
    //   « $V:US:read:*:1# » mesure les NEUF capteurs a portee complete dans
    //   la meme passe : 9 x 30 = 270 ms, soit plus de trois fois le seuil du
    //   chien de garde (LOOP_MAX_MS = 80). Cette commande declenche donc une
    //   urgence a elle seule quand les capteurs ne voient rien.
    //   Contrairement a us_processPeriodic(), on ne l'echelonne PAS : « read »
    //   est une demande explicite de mesures fraiches, pas une veille. Soit
    //   on accepte le cout, soit on refuse « * » et on impose une liste.
    //   Voir §7 de docs/decision_US_detection_panne.md
    if (n == 0) // "*" -> tous
    {
        for (int i = 0; i < US_NUM_SENSORS; i++)
        {
            int v = us_measureOnce(i);
            if (sensorDisabled[i])
                v = US_DISABLED;
            else if (v == 0)
                v = US_SILENCE;
            send_one(i, v);
        }
        return;
    }

    // liste explicite
    for (int k = 0; k < n; k++)
    {
        int i = idxs[k];
        if (i < 0 || static_cast<uint32_t>(i) >= US_NUM_SENSORS)
            continue;

        int v = us_measureOnce(i);
        if (sensorDisabled[i])
            v = US_DISABLED;
        else if (v == 0)
            v = US_SILENCE;

        send_one(i, v);
    }
}

// ----------------------------------------------------------
// Lecture d’une valeur courante
// ----------------------------------------------------------
int us_peekCurrValue(int idx)
{
    if (idx < 0 || static_cast<uint32_t>(idx) >= US_NUM_SENSORS)
        return -1;

    int v = currValue[idx];
    if (sensorDisabled[idx])
        return -1;

    return v;
}