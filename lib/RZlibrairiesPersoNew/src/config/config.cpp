// ======================================================================
// FICHIER : src/config/config.cpp
// Objet   : Définition des constantes centralisées et initialisation
// Rôle    :
//   - centralise toutes les variables de configuration globales
//   - fournit les tableaux de noms d'instances
//   - initialise les paramètres système au boot
// Remarque :
//   L'ordre des sections est ALPHABÉTIQUE par module afin de faciliter
//   la maintenance et la cohérence avec vpiv_dispatch.
// ======================================================================

#include "config.h"
#include "system/urg.h" // URG_NONE
#include <math.h>

// ======================================================================
// CFGS — CONFIGURATION SYSTÈME GLOBALE
// ======================================================================

uint8_t cfg_modeRz = 0;   // 0 par défaut 0 ARRET, 1 VEILLE , 2 FIXE , 3 DEPLACEMENT, 4 AUTONOME
uint8_t cfg_typePtge = 0; // 0 écran, 1 vocal, 2 manette, 3 laisse, 4 laisse+vocal

// ======================================================================
// CTRL_L — PILOTAGE PAR LAISSE (FOLLOW / LAISSE)
// ======================================================================

uint16_t cfg_laisse_target_dist_mm = 1000; // distance moyenne cible
int16_t cfg_laisse_max_speed = 40;         // vitesse max autorisée
int16_t cfg_laisse_max_turn = 30;          // différentiel max gauche/droite
uint8_t cfg_laisse_us_window = 5;          // fenêtre moyenne glissante US
uint16_t cfg_laisse_dead_zone_mm = 80;     // zone neutre autour de la cible

// ======================================================================
// FS — FORCE SENSOR (HX711)
// ======================================================================

const char *fsNames[FS_NUM_SENSORS] = {"fs"};

bool cfg_fs_active = true;
int cfg_fs_threshold = FS_THRESHOLD_DEFAULT;
unsigned long cfg_fs_freq_ms = FS_FREQ_DEFAULT;
long cfg_fs_calibration = FS_CAL_DEFAULT;
long cfg_fs_offset = 0;

// ======================================================================
// IRMVT — DÉTECTION MOUVEMENT INFRAROUGE
// ======================================================================

const char *mvtIrNames[] = {"*"};

bool cfg_mvtir_active = true;
unsigned long cfg_mvtir_freq_ms = MVTIR_FREQ_DEFAULT;
int cfg_mvtir_threshold = MVTIR_THD_DEFAULT;
char cfg_mvtir_mode[16] = "monitor";

// ======================================================================
// LRING — LED RING
// ======================================================================

const char *lringNames[LRING_NUM_PIXELS] = {
    "rg0", "rg1", "rg2", "rg3", "rg4", "rg5",
    "rg6", "rg7", "rg8", "rg9", "rg10", "rg11"};

int cfg_lring_brightness = 50;
int cfg_lring_red = 255;
int cfg_lring_green = 0;
int cfg_lring_blue = 0;
unsigned long cfg_lring_timeout = 0;

// ======================================================================
// LRUB — LED RUBAN
// ======================================================================

const char *lrubNames[LRUB_NUM_PIXELS] = {
    "ar_gd0", "ar_gd1", "ar_gd2", "ar_gd3", "ar_gd4", "ar_gd5", "ar_gd6",
    "av_dg7", "av_dg8", "av_dg9", "av_dg10", "av_dg11", "cg12", "cd13", "cci14"};

int cfg_lrub_brightness = 50;
unsigned long cfg_lrub_timeout = 5000;
int cfg_lrub_redRB = 255;
int cfg_lrub_greenRB = 0;
int cfg_lrub_blueRB = 0;

// ======================================================================
// MIC — MICROPHONES
// ======================================================================

const char *micNames[MIC_COUNT] = {"micAv", "micG", "micD"};

int cfg_mic_threshold[MIC_COUNT] = {500, 500, 500};
bool cfg_mic_active = true;
unsigned long cfg_mic_freq_ms = MIC_FREQ_MS_DEFAULT;
unsigned long cfg_mic_win_ms = MIC_WIN_MS_DEFAULT;

// ======================================================================
// MTR — MOTEURS
// ======================================================================

bool cfg_mtr_active = MOTOR_AUTORISE_DEFAULT;

float cfg_mtr_kturn = MTR_DEFAULT_KTURN;
int cfg_mtr_inputScale = MTR_INPUT_SCALE;
int cfg_mtr_outputScale = MTR_OUTPUT_SCALE;

int cfg_mtr_targetL = 0;
int cfg_mtr_targetR = 0;
int cfg_mtr_targetA = 2; // lissage moyen par défaut (0=direct, 4=très doux)

// ======================================================================
// ODOM — ODOMÉTRIE
// ======================================================================

const char *odomNames[] = {"encL", "encR"};

bool cfg_odom_active = true;
unsigned long cfg_odom_period_ms = 50UL;
unsigned long cfg_odom_report_period_ms = 200UL;
bool cfg_odom_pendingReport = false;

double cfg_odo_wheel_diameter_m = ODO_WHEEL_DIAMETER_M_DEFAULT;
double cfg_odo_track_m = ODO_TRACK_M_DEFAULT;
int cfg_odo_encoder_cpr = ODO_ENCODER_CPR_DEFAULT;

double cfg_odo_coeff_gLong = 0.0;
double cfg_odo_coeff_dLong = 0.0;
double cfg_odo_coeff_gAngl = 0.0;
double cfg_odo_coeff_dAngl = 0.0;

float cfg_odo_lastGyro_dps = 0.0f;
unsigned long cfg_odo_lastGyro_ts_ms = 0;
float cfg_odo_lastCompass_deg = 0.0f;
int cfg_odo_lastCompass_quality = 0;

static void _compute_default_odom_coeffs()
{
    double wheel_circ = M_PI * cfg_odo_wheel_diameter_m;
    double dist_per_tick = wheel_circ / (double)cfg_odo_encoder_cpr;

    cfg_odo_coeff_gLong = dist_per_tick;
    cfg_odo_coeff_dLong = dist_per_tick;

    if (cfg_odo_track_m != 0.0)
    {
        cfg_odo_coeff_dAngl = 1.0 / cfg_odo_track_m;
        cfg_odo_coeff_gAngl = 1.0 / cfg_odo_track_m;
    }
}

__attribute__((constructor)) static void _init_odom_coeffs_ctor()
{
    _compute_default_odom_coeffs();
}

// ======================================================================
// SRV — SERVOMOTEURS
// ======================================================================

const char *srvNames[] = {"TGD", "THB", "BASE"};

int cfg_srv_angles[SERVO_COUNT] = {SERVO_TGD_ZERO, SERVO_THB_ZERO, SERVO_BASE_ZERO};
int cfg_srv_speed[SERVO_COUNT] = {0, 0, 0};
bool cfg_srv_active[SERVO_COUNT] = {true, true, true};

// ======================================================================
// US — ULTRASONS
// ======================================================================

const uint8_t usTrigPins[US_NUM_SENSORS] = {32, 34, 36, 38, 40, 42, 26, 28, 30};
const uint8_t usEchoPins[US_NUM_SENSORS] = {33, 35, 37, 39, 41, 43, 27, 29, 31};

const char *usNames[US_NUM_SENSORS] = {
    "ArG", "ArC", "ArD",
    "AvD", "AvC", "AvG",
    "TtG", "TtD", "TtC"};

bool cfg_us_active = true;
bool cfg_us_send_all = false;
unsigned long cfg_us_ping_cycle_ms = US_PING_CYCLE_MS_DEFAULT;
unsigned long cfg_us_suspect_timeout_ms = US_SUSPECT_TIMEOUT_MS_DEFAULT;
int cfg_us_delta_threshold = US_DELTA_THRESHOLD_DEFAULT;

uint16_t cfg_us_danger_thresholds[US_NUM_SENSORS] = {
    20, 15, 20,
    15, 10, 15,
    20, 20, 20};

uint16_t cfg_us_alert_multiplier[US_NUM_SENSORS] = {1, 1, 1, 1, 1, 1, 1, 1, 1};

// ======================================================================
// URGENCE ( les indices ne doivent être utilisé qu'en interne pour URG - NewV)
// ======================================================================

bool cfg_urg_active = false;
uint8_t cfg_urg_code = URG_NONE;
unsigned long cfg_urg_timestamp = 0;

const char *cfg_urg_messages[] = {
    "OK",
    "OVERHEAT",
    "LOW_BAT",     // ZZ A faire monter chaine récup info.
    "MOTOR_STALL", // coupe moteur - attends clear
    "SENSOR_FAIL",
    "BUFFER_OVERFLOW",
    "PARSING_ERROR",
    "URG_LOOP_TOO_SLOW"};

// ======================================================================
// INITIALISATION CENTRALE
// ======================================================================

void initConfig()
{
    MAIN_SERIAL.begin(MAIN_BAUD);
    MOTEURS_SERIAL.begin(MOTEURS_BAUD);
    delay(50);
}