// RZlibrairiesPersoNew.ccp - init de tout les modules
#include "RZlibrairiesPersoNew.h"

void RZ_initAll() {

    // Série principale + UNO
    initConfig();         // ouvre Serial, initialise pins & valeurs par défaut
    communication_init(); // buffers VPIV
    urg_init();           // sécurité

    // Actionneurs
    lring_init();
    lrub_init();
    srv_init();
    mtr_init();

    // Capteurs
    mic_init();
    us_init();
    fs_init();
    mvt_init();

    // Navigation
    odom_init();

    // Sécurité avancée
    mvtsafe_init();   // si tu veux une init dédiée

    // Message d'information
    sendInfo("System","boot","*","OK");
}
