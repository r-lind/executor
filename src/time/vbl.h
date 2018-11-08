#if !defined(__RSYS_VBL__)
#define __RSYS_VBL__

#include <base/traps.h>

#define MODULE_NAME rsys_vbl
#include <base/api-module.h>

namespace Executor
{
extern void C_ROMlib_vcatch(void);
PASCAL_FUNCTION(ROMlib_vcatch);
}

#endif /* !defined(__RSYS_VBL__) */
