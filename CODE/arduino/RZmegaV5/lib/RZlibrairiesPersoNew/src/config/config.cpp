// ======================================================================
// FICHIER : src/configuration/config.cpp
// Objet   : Définition des constantes centralisées et initialisation
// Chemin  : src/config/config.cpp
// Rôle    : fournir tableaux pins / noms d'instances et initConfig()
// Remarques: centralise toutes les constantes modifiables et leurs
//            abréviations (utile pour Node-RED lors de la synchronisation
//            initiale).
// ======================================================================

#include "config.h"

// ----------------------------------------------------------------------
// LRING : noms d'instances (rg0..rg11)
// ----------------------------------------------------------------------
const char* lringNames[LRING_NUM_PIXELS] = {
    "rg0","rg1","rg2","rg3","rg4","rg5",
    "rg6","rg7","rg8","rg9","rg10","rg11"
};
int cfg_lring_brightness = 50;            // Valeur par défaut
int cfg_lring_red = 255;                 // Couleur rouge par défaut
int cfg_lring_green = 0;                 // Couleur verte par défaut
int cfg_lring_blue = 0;                  // Couleur bleue par défaut
unsigned long cfg_lring_timeout = 0;      // Timeout d'extinction (ms)

// ----------------------------------------------------------------------
// LRUB : noms d'instances (abréviations fournies)
// Indices 0..14
// ----------------------------------------------------------------------
const char* lrubNames[LRUB_NUM_PIXELS] = {
    "ar_gd0","ar_gd1","ar_gd2","ar_gd3","ar_gd4","ar_gd5","ar_gd6",
    "av_dg7","av_dg8","av_dg9","av_dg10","av_dg11", "cg12","cd13","cci14"
};


// Valeurs par défaut pour lrub (écrasées par Node-RED)
int cfg_lrub_brightness = 50;            // Valeur par défaut
unsigned long cfg_lrub_timeout = 5000;   // Valeur par défaut
int cfg_lrub_redRB = 255;                // Couleur rouge par défaut
int cfg_lrub_greenRB = 0;                // Couleur verte par défaut
int cfg_lrub_blueRB = 0;                 // Couleur bleue par défaut
// ----------------------------------------------------------------------
// SERVOMOTEURS (Srv) : noms d'instances
// ----------------------------------------------------------------------
const char* srvNames[] = {
    "TGD",  // tête gauche-droite (horizontale)
    "THB",  // tête haut-bas (verticale)
    "BASE"  // base
};
int cfg_srv_angles[SERVO_COUNT] = {SERVO_TGD_ZERO, SERVO_THB_ZERO, SERVO_BASE_ZERO};
int cfg_srv_speed[SERVO_COUNT] = {0, 0, 0};
bool cfg_srv_active[SERVO_COUNT] = {true, true, true};

// ----------------------------------------------------------------------
// ULTRASONS : pins trig/echo et noms d'instances (nouveau mapping confirmé)
// Index / Nom / TRIG / ECHO / Position
// 0  ArG  32/33  Arrière Gauche
// 1  ArC  34/35  Arrière Centre
// 2  ArD  36/37  Arrière Droit
// 3  AvD  38/39  Avant Droit
// 4  AvC  40/41  Avant Centre
// 5  AvG  42/43  Avant Gauche
// 6  TtG  26/27  Tête Gauche
// 7  TtD  28/29  Tête Droite
// 8  TtC  30/31  Tête Centre
// ----------------------------------------------------------------------

const uint8_t usTrigPins[US_NUM_SENSORS] = {
    32, 34, 36, 38, 40, 42, 26, 28, 30
};

const uint8_t usEchoPins[US_NUM_SENSORS] = {
    33, 35, 37, 39, 41, 43, 27, 29, 31
};

const char* usNames[US_NUM_SENSORS] = {
    "ArG","ArC","ArD",
    "AvD","AvC","AvG",
    "TtG","TtD","TtC"
};

// Variables centralisées du module US
bool cfg_us_active = true;
bool cfg_us_send_all = false;

unsigned long cfg_us_ping_cycle_ms = US_PING_CYCLE_MS_DEFAULT;
unsigned long cfg_us_suspect_timeout_ms = US_SUSPECT_TIMEOUT_MS_DEFAULT;

int cfg_us_delta_threshold = US_DELTA_THRESHOLD_DEFAULT;

// Danger + multiplicateur (valeurs choisies selon tes instructions)
// Arrière: 20/15/20 ; Avant: 15/10/15 ; Tête: 20/20/20
int cfg_us_danger_thresholds[US_NUM_SENSORS] = {
    20, 15, 20,   // ArG, ArC, ArD
    15, 10, 15,   // AvD, AvC, AvG
    20, 20, 20    // TtG, TtD, TtC
};

int cfg_us_alert_multiplier[US_NUM_SENSORS] = {
    1,1,1,1,1,1,1,1,1
};


// ----------------------------------------------------------------------
// MICROPHONES : noms d'instances et valeurs par défaut
// ----------------------------------------------------------------------
const char* micNames[MIC_COUNT] = {
    "micAv", "micG", "micD"
};

int cfg_mic_threshold[MIC_COUNT] = {500, 500, 500};
bool cfg_mic_active = true;
unsigned long cfg_mic_freq_ms = MIC_FREQ_MS_DEFAULT;
unsigned long cfg_mic_win_ms = MIC_WIN_MS_DEFAULT;

// ----------------------------------------------------------------------
// INFRA-ROUGE
// ----------------------------------------------------------------------
const char* mvtIrNames[] = { "*" }; // unique capteur -> instance "*"
/* Variables dynamiques (MvtIR) */
bool cfg_mvtir_active = true;
unsigned long cfg_mvtir_freq_ms = MVTIR_FREQ_DEFAULT;
int cfg_mvtir_threshold = MVTIR_THD_DEFAULT;
char cfg_mvtir_mode[16] = "monitor";

// ----------------------------------------------------------------------
// ODOM - valeurs par défaut et calculs initiaux
// ----------------------------------------------------------------------
const char* odomNames[] = { "encL", "encR" };

bool cfg_odom_active = true;
unsigned long cfg_odom_period_ms = 50UL;        // calcul interne (50ms)
unsigned long cfg_odom_report_period_ms = 200UL; // envoi de la position toutes les 200ms
bool cfg_odom_pendingReport = false;

// paramètres physiques (modifiables au runtime via Node-RED)
double cfg_odo_wheel_diameter_m = ODO_WHEEL_DIAMETER_M_DEFAULT;
double cfg_odo_track_m          = ODO_TRACK_M_DEFAULT;
int    cfg_odo_encoder_cpr     = ODO_ENCODER_CPR_DEFAULT; // ticks par tour (effectif)

// coefficients calculés (initialisés ci-dessous)
double cfg_odo_coeff_gLong = 0.0;
double cfg_odo_coeff_dLong = 0.0;
double cfg_odo_coeff_gAngl = 0.0;
double cfg_odo_coeff_dAngl = 0.0;

// externs de fusion (gyro/compass)
float cfg_odo_lastGyro_dps = 0.0f;
unsigned long cfg_odo_lastGyro_ts_ms = 0;
float cfg_odo_lastCompass_deg = 0.0f;
int cfg_odo_lastCompass_quality = 0;

// Calcul initial des coefficients (appelé à initConfig() ou odom_init())
static void _compute_default_odom_coeffs() {
    // distance parcourue par tick = circonférence / ticks_per_rev
    double wheel_circ = M_PI * cfg_odo_wheel_diameter_m; // m
    double dist_per_tick = wheel_circ / (double)cfg_odo_encoder_cpr; // m / tick

    cfg_odo_coeff_gLong = dist_per_tick;
    cfg_odo_coeff_dLong = dist_per_tick;

    // angle contribution per tick : (dist_right - dist_left) / track => dtheta
    // On garde la formulation dAng = distD * coeffDAngl - distG * coeffGAngl
    // donc coeffDAngl = coeffGAngl = 1 / track (rad per meter). Comme dist in meters, resultat en radians.
    if (cfg_odo_track_m != 0.0) {
        cfg_odo_coeff_dAngl = 1.0 / cfg_odo_track_m;
        cfg_odo_coeff_gAngl = 1.0 / cfg_odo_track_m;
    } else {
        cfg_odo_coeff_dAngl = 0.0;
        cfg_odo_coeff_gAngl = 0.0;
    }
}

// Appelle ce computeur pour initialiser correctement les coeffs (optionnel dans initConfig)
__attribute__((constructor)) static void _init_odom_coeffs_ctor() {
    _compute_default_odom_coeffs(); //use by odom_init() cf odom.cpp si node-red change cfg_odo_...
}


/// ----------------------------------------------------------------------
// FORCE SENSOR (FS) — Capteur HX711
// ----------------------------------------------------------------------

const char* fsNames[FS_NUM_SENSORS] = { "fs" };

bool cfg_fs_active = true;
int cfg_fs_threshold = FS_THRESHOLD_DEFAULT;
unsigned long cfg_fs_freq_ms = FS_FREQ_DEFAULT;
long cfg_fs_calibration = FS_CAL_DEFAULT;
long cfg_fs_offset = 0;


// MTR initial values
char cfg_mtr_acc = MOTOR_ACC_DEFAULT_CHAR;
char cfg_mtr_spd = MOTOR_SPD_DEFAULT_CHAR;
char cfg_mtr_dir = MOTOR_DIR_DEFAULT_CHAR;
bool cfg_mtr_active = MOTOR_AUTORISE_DEFAULT;

// mode modern/legacy
bool cfg_mtr_mode_modern = MTR_MODE_DEFAULT_MODERN;

// modern parameters
float cfg_mtr_kturn = MTR_DEFAULT_KTURN;
int cfg_mtr_inputScale = MTR_INPUT_SCALE;
int cfg_mtr_outputScale = MTR_OUTPUT_SCALE;

// runtime targets (modern raw)
int cfg_mtr_targetL = 0;
int cfg_mtr_targetR = 0;
int cfg_mtr_targetA = 1; // acceleration level 0..4


// ----------------------------------------------------------------------
// URGENCE
// ----------------------------------------------------------------------
bool cfg_urg_active = false;
uint8_t cfg_urg_code = URG_NONE;
unsigned long cfg_urg_timestamp = 0;

const char* cfg_urg_messages[] = {
    "OK",
    "OVERHEAT",
    "LOW_BAT",
    "MOTOR_STALL",
    "SENSOR_FAIL",
    "BUFFER_OVERFLOW",
    "PARSING_ERROR"
};


// ----------------------------------------------------------------------
// Fonction d'initialisation centrale
// ----------------------------------------------------------------------
void initConfig() {
    // Initialise les liaisons série (doit être appelé depuis setup()).
    // Important : on n'initialise pas ici des valeurs dynamiques venant
    //             de Node-RED. L'objectif est d'ouvrir les ports et de
    //             fournir les tableaux constants. Les valeurs "vivantes"
    //             (coeffs asservissement, seuils runtime, etc.) sont
    //             synchronisées par Node-RED via messages VPIV.
    MAIN_SERIAL.begin(MAIN_BAUD);

    // Initialise le port moteurs (UNO)
    MOTEURS_SERIAL.begin(MOTEURS_BAUD);

    // Petit délai pour permettre au PC/RPI de se connecter si nécessaire
    delay(50);

    // NOTE : si tu veux un "mode debug" qui affiche la config à chaque boot,
    // tu peux activer la section suivante (utile pour vérification).
#if 0
    MAIN_SERIAL.println(F("=== CONFIG D'INITIALISATION ==="));
    MAIN_SERIAL.print(F("MAIN_BAUD=")); MAIN_SERIAL.println(MAIN_BAUD);
    MAIN_SERIAL.print(F("MOTEURS_BAUD=")); MAIN_SERIAL.println(MOTEURS_BAUD);
    MAIN_SERIAL.print(F("US pins (trig) : "));
    for (int i=0;i<US_NUM_SENSORS;i++){ MAIN_SERIAL.print(usTrigPins[i]); MAIN_SERIAL.print(","); }
    MAIN_SERIAL.println();
    MAIN_SERIAL.print(F("US pins (echo) : "));
    for (int i=0;i<US_NUM_SENSORS;i++){ MAIN_SERIAL.print(usEchoPins[i]); MAIN_SERIAL.print(","); }
    MAIN_SERIAL.println();
    MAIN_SERIAL.println(F("================================"));
#endif
}

