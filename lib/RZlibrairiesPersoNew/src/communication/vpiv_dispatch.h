// ======================================================================
// FICHIER : src/communication/vpiv_dispatch.h
// Rôle : déclaration des dispatchers VPIV pour chaque module + routeur central
// Version : 1.00
// Date : 2024-06-01
// Auteur : Vincent Philippe
// Remarques : - Les dispatchers sont déclarés dans des namespaces correspondant aux modules
//             - Le routeur central dispatchVPIV() est appelé par communication_processInput()
//               après parsing du VPIV, et redirige vers le dispatcher du module ciblé
//             - Chaque dispatcher de module retourne un booléen indiquant si le VPIV a
// Routeur VPIV : déclaration des dispatchers
// ======================================================================

#ifndef VPIV_DISPATCH_H
#define VPIV_DISPATCH_H

#include <stdbool.h>

// URGENCE
#include "system/urg.h"

// ======================================================================
// NAMESPACES DES MODULES
// (Ils doivent correspondre aux noms des fichiers dispatch_XXX.cpp)
// VPIV structure A ACTUALISE
// ======================================================================
namespace Ctrl_L
{
    bool dispatch(const char *, const char *, const char *);
}

namespace Lrub
{
    bool dispatch(const char *, const char *, const char *);
}
namespace Lring
{
    bool dispatch(const char *, const char *, const char *);
}

namespace Srv
{
    bool dispatch(const char *, const char *, const char *);
}
namespace Mtr
{
    bool dispatch(const char *, const char *, const char *);
}
namespace US
{
    bool dispatch(const char *, const char *, const char *);
}
namespace Mic
{
    bool dispatch(const char *, const char *, const char *);
}

namespace Odom
{
    bool dispatch(const char *, const char *, const char *);
}
namespace IRmvt
{
    bool dispatch(const char *, const char *, const char *);
}
namespace SecMvt
{
    bool dispatch(const char *, const char *, const char *);
}
namespace FS
{
    bool dispatch(const char *, const char *, const char *);
}

namespace CfgS
{
    bool dispatch(const char *, const char *, const char *);
}
namespace Urg
{
    bool dispatch(const char *, const char *, const char *);
}

// ======================================================================
// ROUTEUR CENTRAL
// ======================================================================
bool dispatchVPIV(const char *var,
                  const char *prop,
                  const char *inst,
                  const char *value);

#endif // VPIV_DISPATCH_H
