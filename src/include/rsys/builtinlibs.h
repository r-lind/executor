#pragma once

#include <rsys/cfm.h>

class PowerCore;

namespace Executor
{
    namespace builtinlibs
    {
        uint32_t handleSC(PowerCore& cpu);

        void addPPCEntrypoint(const char *library, const char *function, uint32_t (*code)(PowerCore&));

        ConnectionID getBuiltinLib(Str255 libname);

        Ptr makeUndefinedSymbolStub(const char *name);
    }
}
