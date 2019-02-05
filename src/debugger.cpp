#include <rsys/debugger.h>
#include <OSUtil.h>

#include <mon.h>
#include <base/cpu.h>
#include <PowerCore.h>
#include <syn68k_public.h>
#include <SegmentLdr.h>
#ifndef WIN32
#include <signal.h>
#endif

using namespace Executor;

namespace
{
bool mon_inited = false;
bool singlestep = false;
bool nmi = false;
uint32_t singlestepFromAddr;

void EnterDebugger();
}

void Executor::InitDebugger()
{
    if(mon_inited)
        return;

    mon_init();
    mon_read_byte = [](mon_addr_t addr) { return (uint32_t) *(uint8_t*)SYN68K_TO_US(addr); };
    mon_write_byte = [](mon_addr_t addr, uint32_t b) { *(uint8_t*)SYN68K_TO_US(addr) = (uint8_t) b; };

    PowerCore& cpu = getPowerCore();
    cpu.debugger = [](PowerCore& cpu) { EnterDebugger(); };
    cpu.getNextBreakpoint = [](uint32_t addr) {
        if(singlestep)
        {
            if(addr == singlestepFromAddr)
                return addr + 4;
            else
                return addr;
        }
        
        auto breakpoint_it = active_break_points.lower_bound(addr);
        if(breakpoint_it == active_break_points.end())
            return 0xFFFFFFFF;
        else
            return *breakpoint_it;
    };

    mon_add_command("es", [] { 
        ExitToShell(); 
    }, "es                       Exit To Shell\n");

    mon_add_command("r", [] {
        if(currentCPUMode == CPUMode::ppc)
        {
            PowerCore& cpu = getPowerCore();
            fprintf(monout, "CIA = %08x   cr = %08x  ctr = %08x   lr = %08x\n", cpu.CIA, cpu.cr, cpu.ctr, cpu.lr);
            fprintf(monout, "\n");
            for(int i = 0; i < 32; i++)
            {
                fprintf(monout, "%sr%d =   %08x", i < 10 ? " " : "", i, cpu.r[i]);
                if(i % 8 == 7)
                    fprintf(monout, "\n");
                else
                    fprintf(monout, "  ");
            }
            fprintf(monout, "\n");
            for(int i = 0; i < 32; i++)
            {
                fprintf(monout, "%sf%d = %10f", i < 10 ? " " : "", i, cpu.f[i]);
                if(i % 8 == 7)
                    fprintf(monout, "\n");
                else
                    fprintf(monout, "  ");
            }
        }
        else if(currentCPUMode == CPUMode::m68k)
        {
            fprintf(monout, "PC = %08x\n", currentM68KPC);
            for(int i = 0; i < 8; i++)
                fprintf(monout, "D%d = %08x%s", i, EM_DREG(i), i == 7 ? "\n" : " ");
            for(int i = 0; i < 8; i++)
                fprintf(monout, "A%d = %08x%s", i, EM_AREG(i), i == 7 ? "\n" : " ");
        }
        
    }, "r                        show ppc registers\n");

    mon_add_command("s", [] {
        PowerCore& cpu = getPowerCore();

        mon_exit_requested = true;
        singlestep = true;
        singlestepFromAddr = cpu.CIA;
    }, "s                        single step\n");

    mon_inited = true;

#ifndef WIN32
    struct sigaction act = {};
    act.sa_handler = [](int) {
        nmi = true;
    };
    sigaction(SIGINT, &act, nullptr);
#endif
}

void Executor::CheckForDebuggerInterrupt()
{
    if(nmi)
    {
        nmi = false;
        EnterDebugger();
    }
}

namespace
{

void EnterDebugger()
{
    InitDebugger();
    if(currentCPUMode == CPUMode::ppc)
        mon_dot_address = getPowerCore().CIA;
    else if(currentCPUMode == CPUMode::m68k)
        mon_dot_address = currentM68KPC;

    singlestep = false;
    const char *args[] = {"mon", "-m", "-r", nullptr};
    mon(3,args);

    getPowerCore().flushCache();
}

}

void Executor::C_DebugStr(StringPtr p)
{
    int i;

    fprintf(stderr, "debugstr: ");
    for(i = *p++; i-- > 0; fprintf(stderr, "%c", (LONGINT)*p++))
        ;
    fprintf(stderr, "\n");
}

void Executor::C_Debugger()
{
    EnterDebugger();
}
