// chemin : arduino/mega/lib/RZlibrairiesPersoNew/src/RZlibrairiesPersoNew.cpp
// role init de tout les modules de la librairie RZ, appelé dans setup() du .ino
// RZlibrairiesPersoNew.ccp - init de tout les modules

#include "RZlibrairiesPersoNew.h"

void RZ_initAll()
{

    // Série principale + UNO
    initConfig();         // ouvre Serial, initialise pins & valeurs par défaut
    cfg_modeRz = 0;       // ARRET par défaut (sécurité au boot)
    communication_init(); // buffers VPIV
    urg_init();           // sécurité
    ctrl_L_init();        // enclenche le ctrl par laisse si demandé

    // Actionneurs
    lring_init();
    lrub_init();
    srv_init();
    mtr_init();

    // Capteurs
    mic_init();
    us_init();
    fs_init();
    mvt_ir_init();

    // Navigation
    odom_init();

    // Sécurité avancée
    mvtsafe_init(); // si tu veux une init dédiée

    // Message d'information
    sendInfo("System", "boot", "*", "OK");
}
