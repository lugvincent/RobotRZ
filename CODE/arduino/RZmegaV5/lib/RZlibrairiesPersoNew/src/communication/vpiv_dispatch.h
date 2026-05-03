// ======================================================================
// FICHIER : src/communication/vpiv_dispatch.h
// Routeur VPIV : déclaration des dispatchers
// ======================================================================

#ifndef VPIV_DISPATCH_H
#define VPIV_DISPATCH_H

#include <stdbool.h>

// URGENCE
#include "../system/urg.h"

// ======================================================================
// NAMESPACES DES MODULES
// (Ils doivent correspondre aux noms des fichiers dispatch_XXX.cpp)
// ======================================================================

namespace lrub      { bool dispatch(const char*, const char*, const char*); }
namespace lring     { bool dispatch(const char*, const char*, const char*); }

namespace Srv       { bool dispatch(const char*, const char*, const char*); }
namespace Mt        { bool dispatch(const char*, const char*, const char*); }
namespace US        { bool dispatch(const char*, const char*, const char*); }
namespace Mic       { bool dispatch(const char*, const char*, const char*); }

namespace Odom      { bool dispatch(const char*, const char*, const char*); }
namespace Mvt_ir    { bool dispatch(const char*, const char*, const char*); }
namespace FS        { bool dispatch(const char*, const char*, const char*); }

namespace SCfg      { bool dispatch(const char*, const char*, const char*); }
namespace UrgDispatch { bool dispatch(const char*, const char*, const char*); }

// ======================================================================
// ROUTEUR CENTRAL
// ======================================================================
bool dispatchVPIV(const char* var,
                  const char* inst,
                  const char* prop,
                  const char* value);

#endif // VPIV_DISPATCH_H
