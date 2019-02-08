#include "gtest/gtest.h"
#include <base/cpu.h>
#include <base/mactype.h>
#include <PowerCore.h>

#define BASE 0x1000
#define WORDS (ptr_from_longint<GUEST<uint16_t>*>(BASE))
#define LONGS (ptr_from_longint<GUEST<uint32_t>*>(BASE))

using namespace Executor;

TEST(CPU, execute68K)
{
    EM_D0 = 0;
    auto p = WORDS;
    *p++ = 0x4E71;  //  NOP
    *p++ = 0x303C;  //  MOVE.W ___, d0
    *p++ = 0x002A;  //         #42
    *p++ = 0x4E75;  //  RTS
    
    ROMlib_destroy_blocks(BASE, 32, false);
    execute68K(BASE);

    EXPECT_EQ(42, EM_D0);
}

TEST(CPU, executePPC)
{
    getPowerCore().r[3] = 0;
    auto p = LONGS;
    *p++ = 0x60000000;  //  nop
    *p++ = 0x38600036;  //  li r3, 54
    *p++ = 0x3860002a;  //  li r4, 42
    *p++ = 0x4e800020;  //  blr
    
    getPowerCore().flushCache();
    executePPC(BASE);

    EXPECT_EQ(42, getPowerCore().r[3]);
}


TEST(CPU, breakpoint68K)
{
    EM_D0 = 0;
    auto p = WORDS;
    *p++ = 0x4E71;  //  NOP
    *p++ = 0x303C;  //  MOVE.W ___, d0
    *p++ = 0x0036;  //         #54
    *p++ = 0x303C;  //  MOVE.W ___, d0
    *p++ = 0x002A;  //         #42
    *p++ = 0x4E75;  //  RTS

    thread_local int breakHit;
    thread_local syn68k_addr_t stoppedAddr, stoppedD0;
    breakHit = 0;
    stoppedAddr = stoppedD0 = 0;

    syn68k_debugger_callbacks.getNextBreakpoint = [](syn68k_addr_t addr) -> syn68k_addr_t {
        if(breakHit == 0 && addr <= BASE + 6)
            return BASE + 6;
        else
            return 0xFFFFFFFF;
    };
    syn68k_debugger_callbacks.debugger = [](syn68k_addr_t addr) -> syn68k_addr_t {
        breakHit++;
        stoppedAddr = addr;
        stoppedD0 = EM_D0;
        destroy_blocks(0, ~0);
        return addr;
    };
    
    destroy_blocks(0, ~0);
    execute68K(BASE);

    EXPECT_EQ(42, EM_D0);
    EXPECT_EQ(1, breakHit);
    EXPECT_EQ(54, stoppedD0);
    EXPECT_EQ(BASE + 6, stoppedAddr);
}

TEST(CPU, breakpointPPC)
{
    auto& cpu = getPowerCore();
    cpu.r[3] = 0;
    auto p = LONGS;
    *p++ = 0x60000000;  //  nop
    *p++ = 0x38600036;  //  li r3, 54
    *p++ = 0x3860002a;  //  li r4, 42
    *p++ = 0x4e800020;  //  blr
    
    thread_local int breakHit;
    thread_local syn68k_addr_t stoppedAddr, stoppedR3;
    breakHit = 0;
    stoppedAddr = stoppedR3 = 0;

    cpu.getNextBreakpoint = [](syn68k_addr_t addr) -> syn68k_addr_t {
        if(breakHit == 0 && addr <= BASE + 8)
            return BASE + 8;
        else
            return 0xFFFFFFFF;
    };
    cpu.debugger = [](PowerCore& cpu) {
        breakHit++;
        stoppedAddr = cpu.CIA;
        stoppedR3 = cpu.r[3];
        cpu.flushCache();
    };

    cpu.flushCache();
    executePPC(BASE);

    EXPECT_EQ(42, cpu.r[3]);
    EXPECT_EQ(1, breakHit);
    EXPECT_EQ(54, stoppedR3);
    EXPECT_EQ(BASE + 8, stoppedAddr);
}

