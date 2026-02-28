#ifndef COM_H
#define COM_H
#include "vpiv_dispatch.h"
#include "communication.h" // Pour sendInfo
/**
 * Module COM : Gestion des messages informatifs et de debug.
 *
 * Rôles :
 *   - Centraliser les messages non-critiques (info, debug, warn, error).
 *   - Utiliser CAT=I pour tous les messages (pas de réaction automatique).
 *   - Distinguer les types pour l'affichage (couleurs, filtrage).
 *
 * Messages VPIV :
 *   - $I:Com:info:A:message#
 *   - $I:Com:debug:A:message#
 *   - $I:Com:warn:A:message#
 *   - $I:Com:error:A:message#
 */

namespace Com
{
    // Fonction vide (inline dans le .h)
    inline void init()
    {
        // Rien à faire (pas de log, pas d'initialisation)
    }

    // Déclaration du dispatcher (à implémenter dans le même fichier)
    bool dispatch(const char *prop, const char *inst, const char *value);
}

// Implémentation du dispatcher (inline dans le .h si simple pas besoin de .cpp)
inline bool Com::dispatch(const char *prop, const char *inst, const char *value)
{
    if (!prop || !value)
        return false;

    // Gestion des messages (à étendre si nécessaire)
    if (strcmp(prop, "info") == 0)
    {
        sendInfo("Com", "info", inst, value);
        return true;
    }
    // Les autres propriétés (debug, warn, error) ne sont pas initialisées ici
    return false;
}

#endif