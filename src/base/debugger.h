#pragma once

#include <base/cpu.h>

#include <stdint.h>
#include <set>

namespace Executor
{   
    namespace base
    {
        class Debugger
        {
        public:
            Debugger();
            ~Debugger();

            uint32_t nmi68K(uint32_t addr);
            void nmiPPC();
            uint32_t trapBreak68K(uint32_t addr, const char *name);

            static Debugger *instance;

            virtual bool interruptRequested() { return false; }

        protected:
            enum class Reason
            {
                nmi,
                breakpoint,
                singlestep,
                entrypoint,
                debugger_call,
                debugstr_call
            };

            struct DebuggerEntry
            {
                Reason reason;
                const char* str;
                CPUMode mode;
                uint32_t addr;
            };

            struct DebuggerExit
            {
                uint32_t addr;
                bool singlestep = false;
            };

            virtual DebuggerExit interact(DebuggerEntry e) = 0;

            std::set<uint32_t> breakpoints;
        private:
            uint32_t getNextBreakpoint(uint32_t addr, uint32_t nextOffset);
            uint32_t interact1(DebuggerEntry e);

            bool singlestep = false;
            bool continuingFromEntrypoint = false;
            uint32_t singlestepFrom;
        };
    }
}
