// ======================================================================
// FICHIER : src/communication/vpiv_dispatch.h
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
