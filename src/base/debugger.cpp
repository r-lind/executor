#include "debugger.h"
#include <base/cpu.h>
#include <base/mactype.h>
#include <PowerCore.h>
#include <cassert>

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
    uint16_t frameType [[maybe_unused]] = (*ptr_from_longint<GUEST<uint16_t>*>(EM_A7 + 6)) >> 12;
    assert(frameType == 0);

    uint16_t sr = *ptr_from_longint<GUEST<uint16_t>*>(EM_A7);
	uint32_t addr = *ptr_from_longint<GUEST<uint32_t>*>(EM_A7 + 2);
    EM_A7 += 8;

    assert((cpu_state.sr & 0x3000) == (sr & 0x3000));    // we should not need to switch stacks
    cpu_state.sr = sr & ~31;
    cpu_state.ccc = sr & 1;
    cpu_state.ccv = sr & 2;
    cpu_state.ccnz = ~sr & 4;
    cpu_state.ccn = sr & 8;
    cpu_state.ccx = sr & 16;
    interrupt_note_if_present();

    return interact1({ Reason::nmi, nullptr, CPUMode::m68k, addr });
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

        // figure out whether this was invoked via a trap
        // (as opposed to function pointer)
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

        // if it's a trap call, pop stuff from the stack
    if(trapaddr == addr)
    {
        addr = retaddr - 2;
        EM_A7 += 4;
    }
    
    uint32_t newAddr = interact1({ Reason::entrypoint, name, CPUMode::m68k, addr });

    if(addr == newAddr)
        continuingFromEntrypoint = true;

    return newAddr;
}

uint32_t Debugger::trapBreakPPC(PowerCore& cpu, const char *name)
{
    if(continuingFromEntrypoint)
    {
        continuingFromEntrypoint = false;
        return ~0;
    }

        // pop one stack frame so we are not in the emulator callback stub
    cpu.CIA = cpu.lr -4;
    cpu.r[2] = *ptr_from_longint<GUEST<uint32>*>(cpu.r[1]+0x14);
    cpu.lr = *ptr_from_longint<GUEST<uint32>*>(cpu.r[1]+0x8);

    uint32_t addr = cpu.CIA;
    uint32_t newAddr = interact1({ Reason::entrypoint, name, CPUMode::ppc, addr });

    if(addr == newAddr)
        continuingFromEntrypoint = true;

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
