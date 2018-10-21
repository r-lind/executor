#include "gtest/gtest.h"

#ifdef EXECUTOR
#define AUTOMATIC_CONVERSIONS
#include <QuickDraw.h>
#include <ResourceMgr.h>
using namespace Executor;

QDGlobals qd;

#include <rsys/cquick.h>
#include <rsys/vdriver.h>


class MockVDriver : public VideoDriver
{
    virtual void setColors(int first_color, int num_colors,
                               const struct ColorSpec *color_array) override
    {

    }

    virtual bool setMode(int width, int height, int bpp,
                                bool grayscale_p) override
    {
        width_ = 512;
        height_ = 342;
        rowBytes_ = 64;
        framebuffer_ = new uint8_t[64*342];
        bpp_ = 1;
        return true;
    }
};

#else
#include <Quickdraw.h>
#endif

TEST(QuickDraw, init1)
{
#ifdef EXECUTOR
    vdriver = new MockVDriver();
    initialize_68k_emulator(nullptr, false, (uint32_t *)SYN68K_TO_US(0), 0);
    InitResources();
    ROMlib_InitGDevices();
    LM(TheZone) = LM(ApplZone);
#endif
    InitGraf((Ptr)&qd.thePort);

    GrafPort port;
    OpenPort(&port);

    uint8_t bits[2] = {0,0};
    BitMap bm;
    bm.baseAddr = (Ptr)bits;
    bm.rowBytes = 1;
    SetRect(&bm.bounds, 0,0,2,2);

    port.portBits = bm;

    SetPort(&port);

    MoveTo(0,0);
    LineTo(2,2);

    ClosePort(&port);

    EXPECT_EQ(0x80, bits[0]);
    EXPECT_EQ(0x40, bits[1]);
}
