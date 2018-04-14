#if !defined(_rsys_interfacelib_h_)
#define _rsys_interfacelib_h_

#include "rsys/cfm.h"
namespace Executor
{
extern OSErr ROMlib_GetInterfaceLib(Str63 library, OSType arch,
                                    LoadFlags loadflags, GUEST<ConnectionID> *cidp,
                                    GUEST<Ptr> *mainaddrp, Str255 errName);
}
#endif
