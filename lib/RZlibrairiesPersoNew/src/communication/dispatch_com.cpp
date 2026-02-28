// chemin : lib/RZlibrairiesPersoNew/src/communication/dispatch_com.cpp
// Fichier : dispatch_com.cpp
// Description : Implémentation de la fonction de dispatch pour les propriétés de communication
// Auteur : Phvincent
// Date : 2024-06
#include "vpiv_dispatch.h"
#include "communication/communication.h"

namespace Com
{
    bool dispatch(const char *prop, const char *inst, const char *value)
    {
        if (!prop || !value)
            return false;

        // Propriété 'info' (CAT=I)
        if (strcmp(prop, "info") == 0)
        {
            sendInfo("Com", "info", inst, value); // CAT=I
            return true;
        }

        // Propriété 'debug' (CAT=I)
        if (strcmp(prop, "debug") == 0)
        {
            sendInfo("Com", "debug", inst, value); // CAT=I
            return true;
        }

        // Propriété 'warn' (CAT=I)
        if (strcmp(prop, "warn") == 0)
        {
            sendInfo("Com", "warn", inst, value); // CAT=I
            return true;
        }

        // Propriété 'error' (CAT=I)
        if (strcmp(prop, "error") == 0)
        {
            sendInfo("Com", "error", inst, value); // CAT=I (pas de traitement automatique)
            return true;
        }

        return false;
    }
}
