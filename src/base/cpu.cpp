#include <base/cpu.h>
#include <PowerCore.h>
#include <prefs/prefs.h>

namespace
{
    PowerCore powerCore;
}

PowerCore& Executor::getPowerCore()
{
    return powerCore;
}


/*
 * NOTE: ROMlib_destroy_blocks is a wrapper routine that either destroys
 *	 a limited range or flushes the entire cache.  Apple's semantics
 *	 say that you must flush the entire cache in certain circumstances.
 *	 However, 99% of all programs need to destroy only a range of
 *	 addresses.
 */

unsigned long Executor::ROMlib_destroy_blocks(
    syn68k_addr_t start, uint32_t count, bool flush_only_faulty_checksums)
{
    unsigned long num_blocks_destroyed;

    if(ROMlib_flushoften)
    {
        start = 0;
        count = ~0;
    }
    if(count)
    {
        if(flush_only_faulty_checksums)
            num_blocks_destroyed = destroy_blocks_with_checksum_mismatch(start, count);
        else
            num_blocks_destroyed = destroy_blocks(start, count);
    }
    else
        num_blocks_destroyed = 0;

    return num_blocks_destroyed;
}