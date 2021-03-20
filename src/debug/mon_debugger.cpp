#include <debug/mon_debugger.h>
#include <base/debugger.h>
#include <OSUtil.h>

#include <mon.h>
#include <mon_disass.h>
#include <base/cpu.h>
#include <PowerCore.h>
#include <syn68k_public.h>
#include <SegmentLdr.h>
#ifndef _WIN32
#include <signal.h>
#endif

using namespace Executor;


namespace
{
bool mon_singlestep = false;
bool nmi = false;
}


class MonDebugger : public base::Debugger
{
public:
    MonDebugger();
    ~MonDebugger();

    virtual bool interruptRequested() override;
    virtual DebuggerExit interact(DebuggerEntry e) override;
};

MonDebugger::MonDebugger()
{
    mon_init();
    mon_read_byte = [](mon_addr_t addr) { return (uint32_t) *(uint8_t*)SYN68K_TO_US(addr); };
    mon_write_byte = [](mon_addr_t addr, uint32_t b) { *(uint8_t*)SYN68K_TO_US(addr) = (uint8_t) b; };

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

    mon_add_command("atb", [] {
        if(mon_token != T_STRING)
            fprintf(monout, "Usage: atb \"entrypoint\"\n");
        else
        {
            std::string str = mon_string;
            mon_get_token();
            if(mon_token != T_END)
                fprintf(monout, "Usage: atb \"entrypoint\"\n");
            else if(auto p = traps::entrypoints.find(str); p != traps::entrypoints.end())
                p->second->breakpoint = true;
            else
                fprintf(monout, "No such entrypoint: %s\n", str.c_str());
        }
    }, "atb                      break on entry point\n");

    mon_add_command("atc", [] {
        if(mon_token != T_STRING)
            fprintf(monout, "Usage: atc \"entrypoint\"\n");
        else
        {
            std::string str = mon_string;
            mon_get_token();
            if(mon_token != T_END)
                fprintf(monout, "Usage: atc \"entrypoint\"\n");
            else if(auto p = traps::entrypoints.find(str); p != traps::entrypoints.end())
                p->second->breakpoint = false;
            else
                fprintf(monout, "No such entrypoint: %s\n", str.c_str());
        }
    }, "atc                      clear on entry point breakpoint\n");

    mon_add_command("s", [] {
        mon_exit_requested = true;
        mon_singlestep = true;
    }, "s                        single step\n");


#ifndef _WIN32
    struct sigaction act = {};
    act.sa_handler = [](int) {
        nmi = true;
    };
    sigaction(SIGINT, &act, nullptr);
#endif
}

bool MonDebugger::interruptRequested()
{
    if(nmi)
    {
        nmi = false;
        return true;
    }
    else
        return false;
}

auto MonDebugger::interact(DebuggerEntry entry) -> DebuggerExit
{
    mon_dot_address = entry.addr;

    const char* const cpus[] = { "none", "m68k", "ppc" };
    const char* const reasons[] = { "Ctrl-C", "breakpoint", "singlestep", "api breakpoint", "Debugger call", "DebugStr call" };

    currentCPUMode = entry.mode;

    fprintf(stdout, "(%s) %s", cpus[(int)entry.mode], reasons[(int)entry.reason]);
    if(entry.str)
        fprintf(stdout, ": %s\n", entry.str);
    else
        fprintf(stdout, "\n");

    if(entry.mode == CPUMode::m68k)
    {
        fprintf(stdout, "%08x: ", entry.addr);
        disass_68k(stdout, entry.addr);
    }
    else if(entry.mode == CPUMode::ppc)
    {
        uint32_t w = mon_read_word(entry.addr);
        fprintf(stdout, "%08x: %08x\t", entry.addr, w);
        disass_ppc(stdout, entry.addr, w);
    }

    mon_singlestep = false;
    const char *args[] = {"mon", "-m", "-r", nullptr};
    mon(3,args);
    
    breakpoints = active_break_points;

    return { entry.addr, mon_singlestep };
}

void Executor::InitMonDebugger()
{
    base::Debugger::instance = new MonDebugger();
}
