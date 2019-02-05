#pragma once
#include <syn68k_public.h>

class PowerCore;

namespace Executor
{
    enum class CPUMode
    {
        none,
        m68k,
        ppc
    };

    PowerCore& getPowerCore();

    extern CPUMode currentCPUMode;
    extern syn68k_addr_t currentM68KPC;

    void execute68K(syn68k_addr_t addr);
    void executePPC(syn68k_addr_t addr);

    unsigned long ROMlib_destroy_blocks(syn68k_addr_t start, uint32_t count,
                                        bool flush_only_faulty_checksums);
}