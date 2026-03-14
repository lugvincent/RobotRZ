#ifndef COM_H
#define COM_H

// =====================================================================
// FICHIER  : com.h
// CHEMIN   : lib/RZlibrairiesPersoNew/src/communication/com.h
// VERSION  : 1.1  —  Mars 2026
// AUTEUR   : Vincent Philippe
//
// RÔLE
// ----
//   Déclarations du module COM : messages informatifs et debug
//   publiés vers SP via VPIV (CAT=I).
//
// MESSAGES VPIV PRODUITS (A→SP)
// ------------------------------
//   $I:COM:info:A:<message>#    — information de fonctionnement (ACK, notifications)
//   $I:COM:debug:A:<message>#   — debug — ne pas activer en production
//   $I:COM:warn:A:<message>#    — avertissement non bloquant
//   $I:COM:error:A:<message>#   — erreur non critique (pas d'arrêt auto)
//
// ARTICULATION
// ------------
//   com.h (déclarations) → dispatch_com.cpp (implémentation complète)
//   vpiv_dispatch.cpp → Com::dispatch() → sendInfo()/sendError()
//
// CORRECTION v1.1
// ---------------
//   Suppression de l'implémentation inline de Com::dispatch présente dans v1.0.
//   Cette inline violait la règle ODR (One Definition Rule) : dispatch_com.cpp
//   définissait déjà Com::dispatch → double définition → comportement indéfini
//   à l'édition de liens (le linker choisissait l'une des deux, imprévisible).
//   De plus la version inline était INCOMPLÈTE (gérait seulement "info",
//   ignorait "debug", "warn", "error").
//   Désormais : seule la DÉCLARATION est ici. L'implémentation est dans
//   dispatch_com.cpp (info + debug + warn + error — complète).
//
// PRÉCAUTIONS
// -----------
//   Ne JAMAIS mettre d'implémentation non-inline dans un .h inclus dans
//   plusieurs .cpp → violations ODR immédiates en C++.
// =====================================================================

#include "vpiv_dispatch.h"
#include "communication.h" // Pour sendInfo, sendError

namespace Com
{
    // Initialisation — aucun matériel, aucun état interne
    inline void init()
    {
        // Rien à faire pour ce module
    }

    // Dispatcher VPIV — implémentation dans dispatch_com.cpp
    // Propriétés gérées : info | debug | warn | error
    bool dispatch(const char *prop, const char *inst, const char *value);
}

#endif // COM_H