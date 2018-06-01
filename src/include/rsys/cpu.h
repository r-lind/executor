#pragma once
#include <syn68k_public.h>

class PowerCore;

namespace Executor
{
    PowerCore& getPowerCore();

    unsigned long ROMlib_destroy_blocks(syn68k_addr_t start, uint32_t count,
                                        bool flush_only_faulty_checksums);
}