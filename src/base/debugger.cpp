#include "debugger.h"
#include <base/cpu.h>
#include <base/mactype.h>
#include <PowerCore.h>

using namespace Executor;
using namespace Executor::base;

Debugger *Debugger::instance = nullptr;

Debugger::Debugger()
{
    PowerCore& cpu = getPowerCore();
    cpu.debugger = [](PowerCore& cpu) {
        cpu.CIA = instance->interact1({ Reason::breakpoint, nullptr, CPUMode::ppc, cpu.CIA });
    };
    cpu.getNextBreakpoint = [](uint32_t addr) -> uint32_t { 
        return instance->getNextBreakpoint(addr, 4);
    };
    
    syn68k_debugger_callbacks.debugger = [](uint32_t addr) -> uint32_t {
        return instance->interact1({ Reason::breakpoint, nullptr, CPUMode::m68k, addr });
    };
    syn68k_debugger_callbacks.getNextBreakpoint = [](uint32_t addr) -> uint32_t {
        return instance->getNextBreakpoint(addr, 1); 
    };

    traps::entrypoints["Debugger"]->breakpoint = true;
}

Debugger::~Debugger()
{
    PowerCore& cpu = getPowerCore();
    cpu.debugger = nullptr;
    cpu.getNextBreakpoint = nullptr;
    syn68k_debugger_callbacks.debugger = nullptr;
    syn68k_debugger_callbacks.getNextBreakpoint = nullptr;
}

uint32_t Debugger::interact1(DebuggerEntry entry)
{
    DebuggerExit exit = interact(entry);
    singlestep = exit.singlestep;
    singlestepFrom = exit.addr;
    getPowerCore().flushCache();
    ROMlib_destroy_blocks(0, ~0, false);
    return exit.addr;
}

uint32_t Debugger::nmi68K(uint32_t /*interruptCallbackAddr*/)
{
    // FIXME: the debugger shouldn't be seeing the exception frame here
    auto& addr = *ptr_from_longint<GUEST<uint32_t>*>(EM_A7 + 2);
    addr = interact1({ Reason::nmi, nullptr, CPUMode::m68k, addr });
    return MAGIC_RTE_ADDRESS;
}

void Debugger::nmiPPC()
{
    PowerCore& cpu = getPowerCore();
    cpu.CIA = interact1({ Reason::nmi, nullptr, CPUMode::ppc, cpu.CIA });
}

uint32_t Debugger::trapBreak68K(uint32_t addr, const char *name)
{
    if(continuingFromEntrypoint)
    {
        continuingFromEntrypoint = false;
        return ~0;
    }

    uint32_t retaddr = *ptr_from_longint<GUEST<uint32_t>*>(EM_A7);

    uint16_t potentialTrap = *ptr_from_longint<GUEST<uint16_t>*>(retaddr-2);
    uint32_t trapaddr = 0;
    if((potentialTrap & 0xF000) == 0xA000)
    {
        if(potentialTrap & TOOLBIT)
            trapaddr = tooltraptable[potentialTrap & 0x3FF];
        else
            trapaddr = ostraptable[potentialTrap & 0xFF];
    }

    if(trapaddr == addr)
    {
        addr = retaddr - 2;
        EM_A7 += 4;
    }
    
    uint32_t newAddr = interact1({ Reason::entrypoint, name, CPUMode::m68k, addr });

    if(addr == newAddr)
    {
        continuingFromEntrypoint = true;
    }

    return newAddr;
}

uint32_t Debugger::getNextBreakpoint(uint32_t addr, uint32_t nextOffset)
{
    if(singlestep)
    {
        if(addr == singlestepFrom)
            return addr + nextOffset;
        else
            return addr;
    }
    
    auto breakpoint_it = breakpoints.lower_bound(addr);
    if(breakpoint_it == breakpoints.end())
        return 0xFFFFFFFF;
    else
        return *breakpoint_it;
}

void Debugger::initProcess(uint32_t entrypoint)
{
    breakpoints.clear();
    if(breakOnProcessEntry)
        breakpoints.insert(entrypoint);
}
