// ======================================================================
// FICHIER : src/config/config.h
// Objet   : Déclaration centralisée de toutes les constantes globales,
//           pins Arduino, options systèmes, valeurs par défaut, et
//           déclarations d'API d'initialisation. Celles qui sont transmises
//           par Node-red écrasent une valeur par defaut defini dans config.cpp
//
//
// Chemin  : src/config/config.h
// Rôle    : centraliser les constantes et tableaux d'instances/pins pour
//           éviter leur dispersion dans l'ensemble du code.
// ======================================================================

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ----------------------------------------------------------------------
// GENERAL - paramètres systèmes
// ----------------------------------------------------------------------

// Longueur maximale d’un message VPIV (cohérent avec communication)
#define VPIV_MAX_MSG_LEN 128

// Délai max autorisé pour loop() avant warning
#define LOOP_MAX_TIME_MS 100

// Port série pour communication moteurs (Arduino UNO)
#define MOTEURS_SERIAL Serial3
#define MOTEURS_BAUD 115200

// Port série principal (MQTT / Node-RED / SE)
#define MAIN_SERIAL Serial
#define MAIN_BAUD 115200

// ----------------------------------------------------------------------
// MODULE : LED RING (lring)
// ----------------------------------------------------------------------
#define LRING_PIN 6
#define LRING_NUM_PIXELS 12
// noms d'instances (abréviations) : rg0 .. rg11
extern const char *lringNames[LRING_NUM_PIXELS];
extern int cfg_lring_brightness;        // Intensité lumineuse (0-255)
extern int cfg_lring_red;               // Couleur rouge par défaut
extern int cfg_lring_green;             // Couleur verte par défaut
extern int cfg_lring_blue;              // Couleur bleue par défaut
extern unsigned long cfg_lring_timeout; // Timeout d'extinction (ms)
// Le statut “actif / inactif / urgent / timeout” est stocké dans le module actuators,
//  pas dans config

// ----------------------------------------------------------------------
// MODULE : LED RUBAN (lrub)
// ----------------------------------------------------------------------
#define LRUB_PIN 10
#define LRUB_NUM_PIXELS 15
// noms d'instances (abréviations) : ar_gd0 .. cci14
extern const char *lrubNames[LRUB_NUM_PIXELS];
extern int cfg_lrub_brightness;        // Intensité lumineuse (0-255)
extern int cfg_lrub_redRB;             // Couleur rouge par défaut
extern int cfg_lrub_greenRB;           // Couleur verte par défaut
extern int cfg_lrub_blueRB;            // Couleur bleue par défaut
extern unsigned long cfg_lrub_timeout; // Timeout d'extinction (ms)
//-------------------------------
// MODULE : SERVOMOTEURS (Srv)
// ----------------------------------------------------------------------
#define SERVO_COUNT 3    // Nombre de servomoteurs utilisés (TGD, THB, BASE)
#define SERVO_TGD_PIN 4  // tête gauche/droite (horizontale)
#define SERVO_THB_PIN 5  // tête haut/bas (vertical)
#define SERVO_BASE_PIN 9 // base (rotation du corps)

#define SERVO_TGD_ZERO 30
#define SERVO_THB_ZERO 70
#define SERVO_BASE_ZERO 0

// Noms d'instances (srv) — exemples : "TGD","THB","BASE"
extern const char *srvNames[];
extern int cfg_srv_angles[SERVO_COUNT];  // Angles des servos
extern int cfg_srv_speed[SERVO_COUNT];   // Vitesse des servos
extern bool cfg_srv_active[SERVO_COUNT]; // Activation des servos
// ----------------------------------------------------------------------
// MODULE : MOTEURS (MTR)
// ----------------------------------------------------------------------
// Port série vers UNO (défini ici si nécessaire)
#ifndef MOTEURS_SERIAL
#define MOTEURS_SERIAL Serial3
#endif

// Valeurs par défaut (documentation)
#define MOTOR_ACC_DEFAULT_CHAR '1' // legacy : '0'..'9' (niveau d'accélération)
#define MOTOR_SPD_DEFAULT_CHAR '0'
#define MOTOR_DIR_DEFAULT_CHAR '0'
#define MOTOR_AUTORISE_DEFAULT false

// Mode par défaut modern vs legacy
#define MTR_MODE_DEFAULT_MODERN true // true = modern (<L,R,A>), false = legacy (<a,s,d>)

// Paramètres modern : échelle entrée (-100..100)
#define MTR_INPUT_SCALE 100  // input scale (v / omega range)
#define MTR_OUTPUT_SCALE 400 // sortie UNO attendue approx +/-400 (matching UNO mapping)

// Paramètres calcul diff drive
#define MTR_DEFAULT_KTURN 0.8f // coefficient de conversion omega->diff (ajustable via VPIV)

// Table d'accélération (correspond à l'UNO)  A=0..4
static const int MTR_ACCEL_STEP_FOR_A[5] = {400, 40, 24, 12, 6};

// Variables dynamiques exposées (déclarées dans config.cpp)
extern char cfg_mtr_acc;    // '0'..'9' (legacy)
extern char cfg_mtr_spd;    // '0'..'9' (legacy)
extern char cfg_mtr_dir;    // '0'..'9' (legacy)
extern bool cfg_mtr_active; // autorisation moteurs

// Mode runtime
extern bool cfg_mtr_mode_modern; // true = modern mode, false = legacy

// Param modern
extern float cfg_mtr_kturn;     // facteur k utilisé pour diff-drive
extern int cfg_mtr_inputScale;  // ex: 100
extern int cfg_mtr_outputScale; // ex: 400

// Caches / état runtime
extern int cfg_mtr_targetL; // target left (modern raw units -400..400)
extern int cfg_mtr_targetR; // target right
extern int cfg_mtr_targetA; // accel level 0..4 (modern)

// ----------------------------------------------------------------------
// MODULE : ULTRASONS (US)
// ----------------------------------------------------------------------
#define US_NUM_SENSORS 9
#define US_MAX_DISTANCE_CM 200
#define US_PING_CYCLE_MS_DEFAULT 200
#define US_DELTA_THRESHOLD_DEFAULT 5
#define US_SUSPECT_TIMEOUT_MS_DEFAULT 1000

// Pins TRIG & ECHO
extern const uint8_t usTrigPins[US_NUM_SENSORS];
extern const uint8_t usEchoPins[US_NUM_SENSORS];

// Noms d'instances (ex : "usAV", "usAR", "usG", "usD"...)
extern const char *usNames[US_NUM_SENSORS];

// Variables dynamiques (centralisées)
extern bool cfg_us_active;                      // module active ?
extern bool cfg_us_send_all;                    // envoyer tout à chaque cycle ?
extern unsigned long cfg_us_ping_cycle_ms;      // période cycle
extern unsigned long cfg_us_suspect_timeout_ms; // timeout suspect
extern int cfg_us_delta_threshold;              // delta minimal pour envoi

extern int cfg_us_danger_thresholds[US_NUM_SENSORS]; // seuils danger
extern int cfg_us_alert_multiplier[US_NUM_SENSORS];  // multiplicateur

// ----------------------------------------------------------------------
// MODULE : MICROPHONES (Mic)
// ----------------------------------------------------------------------
#define MIC_COUNT 3
#define MIC_AV_PIN A1
#define MIC_G_PIN A2
#define MIC_D_PIN A3
// valeurs par défaut de fenêtrage / fréquence (modifiable)
#define MIC_WIN_MS_DEFAULT 40
#define MIC_FREQ_MS_DEFAULT 200
// noms d'instances
extern const char *micNames[MIC_COUNT];
// Variables dynamiques pour les micros
extern int cfg_mic_threshold[MIC_COUNT];
extern bool cfg_mic_active;
extern unsigned long cfg_mic_freq_ms;
extern unsigned long cfg_mic_win_ms;

// ----------------------------------------------------------------------
// MODULE : SOUND TRACKER (ST) - fusion éventuelle MIC/ST
// ----------------------------------------------------------------------
#define TRACKER_SAMPLE_MS 40
#define TRACKER_DEFAULT_MODE 1

// ----------------------------------------------------------------------
// MODULE : INFRA-ROUGE (MvtIR)
// ----------------------------------------------------------------------
#ifndef MVT_IR_PIN
#define MVT_IR_PIN 8
#endif

// Valeurs par défaut manipulables
#define MVTIR_FREQ_DEFAULT 150 // ms
#define MVTIR_THD_DEFAULT 1    // nombre d'impulsions consécutives
#define MVTIR_MODE_DEFAULT "monitor"

// Nom d'instances (généralement "*")
extern const char *mvtIrNames[];

// Variables dynamiques exposées (centralisées pour sync Node-RED)
extern bool cfg_mvtir_active;
extern unsigned long cfg_mvtir_freq_ms;
extern int cfg_mvtir_threshold;
extern char cfg_mvtir_mode[16]; // "idle","monitor","surveillance",..

// ----------------------------------------------------------------------
// MODULE : ODOMÉTRIE (Odom)
// ----------------------------------------------------------------------
// Pins encodeurs (conformes à ta Mega — fournis pour clarté)
#define ODO_ENCODER_GA_PIN 2
#define ODO_ENCODER_GB_PIN 19
#define ODO_ENCODER_DA_PIN 18
#define ODO_ENCODER_DB_PIN 3

// Géométrie et matériel (valeurs par défaut, modifiables par Node-RED)
#define ODO_WHEEL_DIAMETER_M_DEFAULT 0.100 // diamètre roue en mètres (100 mm)
#define ODO_TRACK_M_DEFAULT 0.300          // entraxe (distance entre roues) en mètres (300 mm)
#define ODO_ENCODER_CPR_DEFAULT 256        // CPR effectif (ex: 64 CPR * 4 quadrature = 256 ticks)

// Coefficients calculés (m/tick et rad/tick) — exposés pour lecture / override
extern double cfg_odo_coeff_gLong; // distance gauche (m par tick)
extern double cfg_odo_coeff_dLong; // distance droite (m par tick)
extern double cfg_odo_coeff_gAngl; // coefficient angle appliqué aux ticks gauche (rad per tick)
extern double cfg_odo_coeff_dAngl; // coefficient angle appliqué aux ticks droite (rad per tick)

// Paramètres "physiques" modifiables
extern double cfg_odo_wheel_diameter_m; // diamètre roue (m)
extern double cfg_odo_track_m;          // entraxe (m)
extern int cfg_odo_encoder_cpr;         // ticks par révolution (effectifs)

/* Variables dynamiques (existantes) */
extern bool cfg_odom_active;
extern unsigned long cfg_odom_period_ms;
extern unsigned long cfg_odom_report_period_ms;
extern bool cfg_odom_pendingReport;
extern const char *odomNames[];

// ----------------------------------------------------------------------
// MODULE : FORCE SENSOR (FS) — HX711
// ----------------------------------------------------------------------
#define FS_NUM_SENSORS 1

// Pins HX711
#define FS_DOUT_PIN A4
#define FS_SCK_PIN A5

// Defaults
#define FS_THRESHOLD_DEFAULT 50
#define FS_FREQ_DEFAULT 100
#define FS_CAL_DEFAULT 1000  // facteur d'échelle integer
#define FS_WIN_MS_DEFAULT 20 // fenêtre pour angle/force

// Instances
extern const char *fsNames[FS_NUM_SENSORS];

// Runtime parameters
extern bool cfg_fs_active;
extern int cfg_fs_threshold;
extern unsigned long cfg_fs_freq_ms;
extern long cfg_fs_calibration;
extern long cfg_fs_offset;

// ----------------------------------------------------------------------
// MODULE : Urgence (Emergency / system)
// ----------------------------------------------------------------------
// #define EMG_FORCE_LIMIT      3000
// #define EMG_US_MIN_CM        8
// #define EMG_TIMEOUT_MS       200
/// ----------------------------------------------------------------------
// MODULE : URGENCE (Urg)
// ----------------------------------------------------------------------
extern bool cfg_urg_active;  // urgence en cours ?
extern uint8_t cfg_urg_code; // code urgence
extern unsigned long cfg_urg_timestamp;

extern const char *cfg_urg_messages[]; // textes associés

// ----------------------------------------------------------------------
// PROTOYPE D'INIT / UTILITAIRES
// ----------------------------------------------------------------------
/**
 * initConfig()
 *   - Initialise Serial(s).
 *   - Initialise les tableaux constants (pins, names).
 *   - Sert de point d'entrée pour charger des valeurs venant de Node-RED
 *     (synchronisation initiale) : la charge effective depuis Node-RED
 *     est faite par le mécanisme VPIV (envoi des $V:... par Node-RED).
 */
void initConfig();

#endif // CONFIG_H
