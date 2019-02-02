#pragma once

#include <rsys/cfm.h>
#include <functional>

class PowerCore;

namespace Executor
{
    namespace builtinlibs
    {
        uint32_t handleSC(PowerCore& cpu);

        void addPPCEntrypoint(const char *library, const char *function, std::function<uint32_t (PowerCore&)> code);

        ConnectionID getBuiltinLib(Str255 libname);

        Ptr makeUndefinedSymbolStub(const char *name);
    }
}
