// ======================================================================
// FICHIER : src/config/config.h
// Objet   : Déclaration centralisée de toutes les constantes, variables
//           globales configurables et paramètres système.
// Rôle    : Référence unique des variables synchronisées avec Node-RED
//           via VPIV. Toute variable ici peut être écrasée au runtime.
// ======================================================================
#include <stdint.h>
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

/* ===================================================================== */
/* GENERAL SYSTEM                                                         */
/* ===================================================================== */

#define VPIV_MAX_MSG_LEN 128
#define LOOP_MAX_TIME_MS 100

#define MAIN_SERIAL Serial
#define MAIN_BAUD 115200

#define MOTEURS_SERIAL Serial3
#define MOTEURS_BAUD 115200

/* ===================================================================== */
/* CFGS — MODE GLOBAL ROBOT                                               */
/* ===================================================================== */

extern uint8_t cfg_modeRz;
extern uint8_t cfg_typePtge;

/* ===================================================================== */
/* CTRL_L — LAISSE / SUIVI UTILISATEUR                                    */
/* ===================================================================== */

extern uint16_t cfg_laisse_target_dist_mm;
extern int16_t cfg_laisse_max_speed;
extern int16_t cfg_laisse_max_turn;
extern uint8_t cfg_laisse_us_window;
extern uint16_t cfg_laisse_dead_zone_mm;

/* ===================================================================== */
/* FS — FORCE SENSOR (HX711)                                              */
/* ===================================================================== */

#define FS_NUM_SENSORS 1
#define FS_DOUT_PIN A4 // Ligne DATA du HX711
#define FS_SCK_PIN A5  // Ligne CLOCK du HX711

// Valeurs par défaut (limites, seuils, etc.)
#define FS_THRESHOLD_DEFAULT 50 // Seuil d’alerte valeur corrigée
#define FS_FREQ_DEFAULT 100     // Fréquence monitoring (ms)
#define FS_CAL_DEFAULT 1000     // Gain (x1000) — 1000 = x1.0

extern const char *fsNames[FS_NUM_SENSORS];

extern bool cfg_fs_active;
extern int cfg_fs_threshold;
extern unsigned long cfg_fs_freq_ms;
extern long cfg_fs_calibration;
extern long cfg_fs_offset;

/* ===================================================================== */
/* IR — INFRA-ROUGE MOUVEMENT                                             */
/* ===================================================================== */

#define MVT_IR_PIN 8

#define MVTIR_FREQ_DEFAULT 150
#define MVTIR_THD_DEFAULT 1
#define MVTIR_MODE_DEFAULT "monitor"

extern const char *mvtIrNames[];

extern bool cfg_mvtir_active;
extern unsigned long cfg_mvtir_freq_ms;
extern int cfg_mvtir_threshold;
extern char cfg_mvtir_mode[16];

/* ===================================================================== */
/* LRING — LED RING                                                       */
/* ===================================================================== */

#define LRING_PIN 6
#define LRING_NUM_PIXELS 12

extern const char *lringNames[LRING_NUM_PIXELS];

extern int cfg_lring_brightness;
extern int cfg_lring_red;
extern int cfg_lring_green;
extern int cfg_lring_blue;
extern unsigned long cfg_lring_timeout;

/* ===================================================================== */
/* LRUB — LED RUBAN                                                       */
/* ===================================================================== */

#define LRUB_PIN 10
#define LRUB_NUM_PIXELS 15

extern const char *lrubNames[LRUB_NUM_PIXELS];

extern int cfg_lrub_brightness;
extern int cfg_lrub_redRB;
extern int cfg_lrub_greenRB;
extern int cfg_lrub_blueRB;
extern unsigned long cfg_lrub_timeout;

/* ===================================================================== */
/* MIC — MICROPHONES                                                      */
/* ===================================================================== */

#define MIC_COUNT 3

#define MIC_AV_PIN A1
#define MIC_G_PIN A2
#define MIC_D_PIN A3

#define MIC_WIN_MS_DEFAULT 40
#define MIC_FREQ_MS_DEFAULT 200

extern const char *micNames[MIC_COUNT];

extern int cfg_mic_threshold[MIC_COUNT];
extern bool cfg_mic_active;
extern unsigned long cfg_mic_freq_ms;
extern unsigned long cfg_mic_win_ms;

/* ===================================================================== */
/* MTR — MOTEURS                                                          */
/* ===================================================================== */

#define MOTOR_ACC_DEFAULT_CHAR '1'
#define MOTOR_SPD_DEFAULT_CHAR '0'
#define MOTOR_DIR_DEFAULT_CHAR '0'
#define MOTOR_AUTORISE_DEFAULT false

#define MTR_MODE_DEFAULT_MODERN true

#define MTR_INPUT_SCALE 100
#define MTR_OUTPUT_SCALE 400
#define MTR_DEFAULT_KTURN 0.8f

static const int MTR_ACCEL_STEP_FOR_A[5] = {400, 40, 24, 12, 6};

extern char cfg_mtr_acc;
extern char cfg_mtr_spd;
extern char cfg_mtr_dir;
extern bool cfg_mtr_active;

extern bool cfg_mtr_mode_modern;
extern float cfg_mtr_kturn;
extern int cfg_mtr_inputScale;
extern int cfg_mtr_outputScale;

extern int cfg_mtr_targetL;
extern int cfg_mtr_targetR;
extern int cfg_mtr_targetA;

/* ===================================================================== */
/* ODOM — ODOMETRIE                                                       */
/* ===================================================================== */

#define ODO_ENCODER_GA_PIN 2
#define ODO_ENCODER_GB_PIN 19
#define ODO_ENCODER_DA_PIN 18
#define ODO_ENCODER_DB_PIN 3

#define ODO_WHEEL_DIAMETER_M_DEFAULT 0.100
#define ODO_TRACK_M_DEFAULT 0.300
#define ODO_ENCODER_CPR_DEFAULT 256

extern const char *odomNames[];

extern bool cfg_odom_active;
extern unsigned long cfg_odom_period_ms;
extern unsigned long cfg_odom_report_period_ms;
extern bool cfg_odom_pendingReport;

extern double cfg_odo_wheel_diameter_m;
extern double cfg_odo_track_m;
extern int cfg_odo_encoder_cpr;

extern double cfg_odo_coeff_gLong;
extern double cfg_odo_coeff_dLong;
extern double cfg_odo_coeff_gAngl;
extern double cfg_odo_coeff_dAngl;

extern float cfg_odo_lastGyro_dps;
extern unsigned long cfg_odo_lastGyro_ts_ms;
extern float cfg_odo_lastCompass_deg;
extern int cfg_odo_lastCompass_quality;

/* ===================================================================== */
/* SRV — SERVOMOTEURS                                                     */
/* ===================================================================== */

#define SERVO_COUNT 3

#define SERVO_TGD_PIN 4
#define SERVO_THB_PIN 5
#define SERVO_BASE_PIN 9

#define SERVO_TGD_ZERO 30
#define SERVO_THB_ZERO 70
#define SERVO_BASE_ZERO 0

extern const char *srvNames[];

extern int cfg_srv_angles[SERVO_COUNT];
extern int cfg_srv_speed[SERVO_COUNT];
extern bool cfg_srv_active[SERVO_COUNT];

/* ===================================================================== */
/* US — ULTRASONS                                                         */
/* ===================================================================== */

#define US_NUM_SENSORS 9

#define US_PING_CYCLE_MS_DEFAULT 200
#define US_DELTA_THRESHOLD_DEFAULT 5
#define US_SUSPECT_TIMEOUT_MS_DEFAULT 1000
#define US_MAX_DISTANCE_CM 400
extern const uint8_t usTrigPins[US_NUM_SENSORS];
extern const uint8_t usEchoPins[US_NUM_SENSORS];
extern const char *usNames[US_NUM_SENSORS];

extern bool cfg_us_active;
extern bool cfg_us_send_all;
extern unsigned long cfg_us_ping_cycle_ms;
extern unsigned long cfg_us_suspect_timeout_ms;
extern int cfg_us_delta_threshold;

extern uint16_t cfg_us_danger_thresholds[US_NUM_SENSORS];
extern uint16_t cfg_us_alert_multiplier[US_NUM_SENSORS];

/* ===================================================================== */
/* URG — URGENCE                                                          */
/* ===================================================================== */

extern bool cfg_urg_active;
extern uint8_t cfg_urg_code;
extern unsigned long cfg_urg_timestamp;

extern const char *cfg_urg_messages[];

/* ===================================================================== */
/* INIT                                                                   */
/* ===================================================================== */

void initConfig();

#endif // CONFIG_H
