/************************************************************
 * UNO : Contrôleur moteurs (Dual VNH5019)
 * Reçoit via Serial1 (depuis la Mega) des trames :
 *    <L,R,A>    -> mode moderne (L,R signés, A = 0..4)
 * OU <acc,spd,dir> -> mode legacy (valeurs 0..9)
 *
 * Exemple trames :
 *   <120,-80,2>
 *   <1,3,2>    (legacy)
 *
 * L,R : vitesses signées entières (bornées à MOTOR_MAX)
 * A   : intensité accélération (0 .. 4) (0 = changement direct)
 *
 * Le UNO applique une interpolation non bloquante pour atteindre
 * progressivement targetL/targetR en respectant A.
 *
 * Baud recommandé (Mega <-> Uno) : 115200 (doit matcher la Mega)
 *
 * Auteur : adapté pour RZ-BOT - commentaires FR
 ************************************************************/

#include <Arduino.h>
#include "DualVNH5019MotorShield.h" // Pololu Dual VNH5019 library

// ======================= CONFIG ===========================
const unsigned long SERIAL_BAUD = 115200UL; // doit matcher la Mega
const unsigned long TIMEOUT_MS   = 1000UL;  // arrêt/ralenti si pas de trame
const int MOTOR_MAX = 400;                  // plage autorisée en sortie (-MOTOR_MAX..+MOTOR_MAX)

// Paramètres de lissage par niveau A
// Chaque niveau donne un pas maximal par cycle (unités moteur) — plus grand = plus agressif
// Ces valeurs ont été calibrées pour ton robot ~4kg / roues 10cm.
const int accelStepForA[5] = { 400, 40, 24, 12, 6 };
// A=0 -> pas de lissage (changement direct), A=1..4 -> de plus en plus doux

// ======================= ETAT GLOBAL ======================
DualVNH5019MotorShield md;            // objet pilote Pololu

// Consignes cibles (écrites par l'ISR série / parse)
volatile int targetL = 0;             // consigne gauche
volatile int targetR = 0;             // consigne droite
volatile int accelA  = 2;             // intensité accélération (0..4)

// Etats courants (ceux réellement envoyés au driver)
int curL = 0;
int curR = 0;

// Historique legacy (si on reçoit format ancien)
int legacy_acc = 1;
int legacy_spd = 0;
int legacy_dir = 0;

// Timestamp dernier reception utile
unsigned long lastRxMillis = 0;

// Buffer de réception (non bloquant, lecture par byte)
static char lineBuf[64];
static size_t bufIdx = 0;

// ======================= UTILITAIRES ======================

// Clamp valeur moteur dans la plage autorisée
static int clampMotor(int v) {
    if (v > MOTOR_MAX) return MOTOR_MAX;
    if (v < -MOTOR_MAX) return -MOTOR_MAX;
    return v;
}

// Applique la consigne aux moteurs via la librairie Pololu.
// left/right doivent être clampés avant appel.
static void applyMotorOutputs(int left, int right) {
    // la librairie attend des entiers, on lui passe tels quels
    md.setSpeeds(left, right);
}

// Retourne vrai si on a reçu une commande récemment
static bool hasRecentCmd() {
    return (millis() - lastRxMillis) < TIMEOUT_MS;
}

// Avance cur vers tgt avec un pas maximal 'step'
static int advanceTowards(int cur, int tgt, int step) {
    if (cur == tgt) return cur;
    int diff = tgt - cur;
    int sign = (diff > 0) ? 1 : -1;
    int absdiff = abs(diff);
    int delta = (absdiff <= step) ? absdiff : step;
    return cur + sign * delta;
}

// ======================= PARSING ==========================
//
// Format attendu dans le buffer (sans les chevrons):
//  - modern :  "<L,R,A>"   (ex: 120,-80,2)
//  - legacy :  "<acc,spd,dir>" (ex: 1,3,2)
//
// La fonction parseBuffer() analyse lineBuf (sans chevrons) et
// met à jour targetL, targetR, accelA (atomiquement).
//
static bool parseBufferAndStore(const char* s) {
    if (!s || !s[0]) return false;

    // Copier dans un buffer modifiable
    char buf[64];
    strncpy(buf, s, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';

    // compter virgules
    int comma1 = -1, comma2 = -1;
    for (int i = 0; buf[i] != '\0'; ++i) {
        if (buf[i] == ',') {
            if (comma1 == -1) comma1 = i;
            else if (comma2 == -1) comma2 = i;
            else break;
        }
    }
    if (comma1 == -1 || comma2 == -1) return false;

    // Séparer en 3 champs (termine par \0)
    char *p1 = buf;
    char *p2 = buf + comma1 + 1;
    char *p3 = buf + comma2 + 1;
    buf[comma1] = '\0';
    buf[comma2] = '\0';

    // Détecter s'il s'agit du mode moderne (premier champ signé ou un nombre >9)
    bool isModern = false;
    // skip leading spaces
    while (*p1 && isspace((unsigned char)*p1)) p1++;
    if (*p1 == '-' || isdigit((unsigned char)*p1) ) {
        // if number contains '-' or length >1 or value > 9 => modern
        if (strchr(p1,'-') || strlen(p1) > 1 || atoi(p1) > 9) isModern = true;
    }

    if (isModern) {
        // Parseer signé L,R et A
        int L = atoi(p1);
        int R = atoi(p2);
        int A = atoi(p3);
        // clamp
        L = clampMotor(L);
        R = clampMotor(R);
        if (A < 0) A = 0;
        if (A > 4) A = 4;

        // stocker atomiquement
        noInterrupts();
        targetL = L;
        targetR = R;
        accelA  = A;
        lastRxMillis = millis();
        interrupts();
        return true;
    } else {
        // Legacy parsing (acc,spd,dir) (0..9)
        int a = atoi(p1);
        int s = atoi(p2);
        int d = atoi(p3);
        a = constrain(a, 0, 9);
        s = constrain(s, 0, 9);
        d = constrain(d, 0, 9);

        // Conversion simple legacy -> L,R
        // (On reprend la logique proposée : v = spd * 40 ; w = (dir - 5) * 80)
        int v = s * 40;                  // base vitesse
        int w = (d - 5) * 80;            // écart / rotation
        int L = v - w;
        int R = v + w;
        L = clampMotor(L);
        R = clampMotor(R);
        int A = a; // utiliser a (0..9) mais on la mappe sur 0..4 si nécessaire
        // map legacy acc 0..9 -> 0..4 (approx)
        int mappedA = constrain( (A * 4) / 9, 0, 4 );

        noInterrupts();
        targetL = L;
        targetR = R;
        accelA  = mappedA;
        lastRxMillis = millis();
        interrupts();
        return true;
    }
}

// ======================= SETUP =============================
void setup() {
    // Serial pour debug PC (optional)
    Serial.begin(SERIAL_BAUD);

    // Serial1 : lien Mega -> UNO
    Serial1.begin(SERIAL_BAUD);

    // Initialisation driver Pololu
    md.init();
    md.setSpeeds(0, 0);

    // Etat initial
    curL = curR = 0;
    targetL = targetR = 0;
    accelA = 2;
    lastRxMillis = millis();

    // message de boot pour debug
    Serial.println(F("UNO moteurs boot OK"));
}

// ======================= MAIN LOOP =========================
void loop() {
    // 1) Lecture série non-bloquante depuis Serial1 (Mega)
    while (Serial1.available() > 0) {
        char c = (char)Serial1.read();

        if (c == '<') {
            // début de trame
            bufIdx = 0;
            // on ne stocke pas le '<'
            continue;
        }
        if (c == '>') {
            // fin de trame -> traiter si contenu
            if (bufIdx > 0) {
                // s'assurer terminaison
                lineBuf[bufIdx] = '\0';
                // parse et stocke atomiquement
                parseBufferAndStore(lineBuf);
            }
            bufIdx = 0;
            continue;
        }

        // si on est au milieu d'une trame on accumule (protège overflow)
        if (bufIdx < sizeof(lineBuf) - 1) {
            lineBuf[bufIdx++] = c;
        } else {
            // overflow -> reset pour sécurité
            bufIdx = 0;
        }
    }

    // 2) Timeout safety : si pas de commandes récentes, ralentir progressivement vers 0
    if (!hasRecentCmd()) {
        // freine vers 0 avec pas fixe (sécurité)
        int step = 20;
        if (curL > 0) curL = max(0, curL - step);
        else if (curL < 0) curL = min(0, curL + step);

        if (curR > 0) curR = max(0, curR - step);
        else if (curR < 0) curR = min(0, curR + step);

        applyMotorOutputs(curL, curR);
        // on continue la loop (ne pas bloquer)
        return;
    }

    // 3) Interpolation (lissage) non bloquante
    // copie stable des cibles
    int tgtL, tgtR, A;
    noInterrupts();
    tgtL = targetL; tgtR = targetR; A = accelA;
    interrupts();

    if (A == 0) {
        // mode réactif : changement direct
        curL = tgtL;
        curR = tgtR;
    } else {
        int step = accelStepForA[ constrain(A, 0, 4) ];
        curL = advanceTowards(curL, tgtL, step);
        curR = advanceTowards(curR, tgtR, step);
    }

    // 4) Appliquer consignes calculées
    applyMotorOutputs(curL, curR);

    // Boucle courte ; pas de delay long pour conserver réactivité
}

// ======================= FIN ===============================
