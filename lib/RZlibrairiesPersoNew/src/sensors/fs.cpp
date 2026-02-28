/************************************************************
 * FICHIER : src/sensors/fs.cpp
 * ROLE    : Logique haute-niveau du capteur de force (module FS)
 *
 * Principes :
 *  - Lecture périodique selon cfg_fs_freq_ms non blocante
 *  - Conversion : value_int = round( (raw - offset) * calib )
 *    -> résultat entier; en mode test on renvoie un entier 3-chiffres possible
 *  - Angle : pour FS_NUM_SENSORS == 1 renvoie 0 (placeholder)
 *  - Anti-spam : envoi VPIV de l'angle uniquement si écart >= FS_ANGLE_DELTA_MIN
 *
 * VPIV :
 *  - sendInfo("FS","value","*", "<value_int>")
 *  - sendInfo("FS","raw","*", "<raw>")
 *  - sendInfo("FS","angle","*", "<angle>")
 *  - sendInfo("FS","alert","*", "<value>") si dépasse cfg_fs_threshold
 ************************************************************/
/************************************************************
 * FICHIER : src/sensors/fs.cpp
 * ROLE    : Force Sensor : integer, non-blocking
 ************************************************************/

#include "fs.h"
#include "hardware/fs_hardware.h"
#include "communication/communication.h"
#include "config/config.h"

static long lastForce = 0;
static int lastAngleDeg = 0;

// orientation simple (si futur tu veux 2 capteurs → étendre)
static int computeAngle(long f)
{
    if (f > cfg_fs_threshold)
        return 0;
    return 0;
}

void fs_init()
{
    fs_hw_init();
    cfg_fs_active = true;
}

long fs_lastForce() { return lastForce; }
int fs_lastAngle() { return lastAngleDeg; }

// reading commands
void fs_doRead(const char *type)
{
    long r;
    if (!fs_hw_readRaw(r))
        return;

    long f = (r - cfg_fs_offset) / cfg_fs_calibration;
    lastForce = f;

    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", f);
    sendInfo("FS", "value", "*", buf);
}

void fs_doTare() { fs_hw_tare(); }
void fs_doCal(long f) { fs_hw_setCal(f); }

void fs_setActive(bool on) { cfg_fs_active = on; }
void fs_setFreq(unsigned long ms) { cfg_fs_freq_ms = ms; }
void fs_setThreshold(int th) { cfg_fs_threshold = th; }

// periodic
void fs_processPeriodic()
{
    static unsigned long lastT = 0;
    if (!cfg_fs_active)
        return;

    unsigned long now = millis();
    if (now - lastT < cfg_fs_freq_ms)
        return;
    lastT = now;

    long f = fs_hw_readForce();
    lastForce = f;

    // SEND force
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", f);
    sendInfo("FS", "value", "*", buf);

    // orientation
    lastAngleDeg = computeAngle(f);

    char b2[16];
    snprintf(b2, sizeof(b2), "%d", lastAngleDeg);
    sendInfo("FS", "angle", "*", b2);

    // alert
    if (f > cfg_fs_threshold)
    {
        sendInfo("FS", "alert", "*", buf);
    }
}
