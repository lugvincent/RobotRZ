# Patch config.h / config.cpp — suppression variables legacy Mtr
# Version : Mars 2026
# À appliquer manuellement dans config.h et config.cpp

## Dans config.h — SUPPRIMER ces lignes

```cpp
// --- SUPPRIMER (legacy motor chars, plus utilisés) ---
#define MOTOR_ACC_DEFAULT_CHAR '1'
#define MOTOR_SPD_DEFAULT_CHAR '0'
#define MOTOR_DIR_DEFAULT_CHAR '0'

// --- SUPPRIMER (mode boolean, toujours modern désormais) ---
#define MTR_MODE_DEFAULT_MODERN true

// --- SUPPRIMER (variables legacy) ---
extern char cfg_mtr_acc;
extern char cfg_mtr_spd;
extern char cfg_mtr_dir;
extern bool cfg_mtr_mode_modern;
```

## Dans config.h — CONSERVER (toujours utilisés)

```cpp
#define MOTOR_AUTORISE_DEFAULT false  // ← conserver
#define MTR_INPUT_SCALE  100          // ← conserver
#define MTR_OUTPUT_SCALE 400          // ← conserver
#define MTR_DEFAULT_KTURN 0.8f        // ← conserver
static const int MTR_ACCEL_STEP_FOR_A[5] = {400, 40, 24, 12, 6}; // ← conserver (UNO)

extern bool  cfg_mtr_active;          // ← conserver
extern float cfg_mtr_kturn;           // ← conserver
extern int   cfg_mtr_inputScale;      // ← conserver
extern int   cfg_mtr_outputScale;     // ← conserver
extern int   cfg_mtr_targetL;         // ← conserver
extern int   cfg_mtr_targetR;         // ← conserver
extern int   cfg_mtr_targetA;         // ← conserver
```

## Dans config.cpp — SUPPRIMER ces lignes

```cpp
char cfg_mtr_acc = MOTOR_ACC_DEFAULT_CHAR;   // ← supprimer
char cfg_mtr_spd = MOTOR_SPD_DEFAULT_CHAR;   // ← supprimer
char cfg_mtr_dir = MOTOR_DIR_DEFAULT_CHAR;   // ← supprimer
bool cfg_mtr_mode_modern = MTR_MODE_DEFAULT_MODERN; // ← supprimer
```

## Dans config.cpp — CONSERVER

```cpp
bool  cfg_mtr_active      = MOTOR_AUTORISE_DEFAULT;  // ← conserver
float cfg_mtr_kturn        = MTR_DEFAULT_KTURN;       // ← conserver
int   cfg_mtr_inputScale   = MTR_INPUT_SCALE;         // ← conserver
int   cfg_mtr_outputScale  = MTR_OUTPUT_SCALE;        // ← conserver
int   cfg_mtr_targetL      = 0;                       // ← conserver
int   cfg_mtr_targetR      = 0;                       // ← conserver
int   cfg_mtr_targetA      = 2;                       // ← conserver (était 1, passer à 2=lissage moyen)
```

## Remarque sur MTR_ACCEL_STEP_FOR_A

Ce tableau n'est plus utilisé côté Mega (le lissage est géré entièrement
par l'UNO). Il peut rester dans config.h comme documentation de référence
(les valeurs correspondent à ce que l'UNO applique), ou être déplacé dans
un commentaire de mtr_hardware.h.
