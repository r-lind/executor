#pragma once

#include <rsys/cfm.h>

namespace Executor
{
    namespace builtinlibs
    {
        void addPPCEntrypoint(const char *library, const char *function, void *ppc_code);

        ConnectionID getBuiltinLib(Str255 libname);
    }
}
