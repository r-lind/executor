#include "gtest/gtest.h"

#ifdef EXECUTOR
#define AUTOMATIC_CONVERSIONS
#include <QuickDraw.h>
#include <CQuickDraw.h>
#include <MemoryMgr.h>

using namespace Executor;
extern QDGlobals qd;

#define PTR(x) (&guestref(x))

    // FIXME: Executor still defines Pattern as an array, rather than an array wrapped in a struct
#define PATREF(pat) (pat)
#else
#include <Quickdraw.h>
#include <QDOffscreen.h>

#define PTR(x) (&x)
#define PATREF(pat) (&(pat))
#endif

struct OffscreenPort
{
    GrafPort port;
    Rect r;

    OffscreenPort(int height = 2, int width = 2)
    {
        OpenPort(&port);

        short rowBytes = (width + 31) / 31 * 4;
        port.portBits.rowBytes = rowBytes;
        port.portBits.baseAddr = NewPtrClear(rowBytes * height);
        SetRect(&r, 0,0,width,height);
        port.portBits.bounds = r;

        set();
    }

    ~OffscreenPort()
    {
        ClosePort(&port);
    }

    void set()
    {
        SetPort(&port);
    }

    uint8_t data(int row, int offset)
    {
        return port.portBits.baseAddr[row * port.portBits.rowBytes + offset];
    }
};

struct OffscreenWorld
{
    GWorldPtr world;
    Rect r;
    
    OffscreenWorld(int depth, int height = 2, int width = 2)
    {
        SetRect(&r,0,0,2,2);
        NewGWorld(PTR(world), depth, &r, nullptr, nullptr, 0);
        LockPixels(world->portPixMap);
        set();
    }

    ~OffscreenWorld()
    {
        if((GrafPtr)world == qd.thePort)
            SetGDevice(GetMainDevice());
        DisposeGWorld(world);
    }

    void set()
    {
        SetGWorld(world, nullptr);
    }

    uint8_t data(int row, int offset)
    {
        Ptr baseAddr = (*world->portPixMap)->baseAddr;
        uint8_t *data = (uint8_t*)baseAddr;
        short rowBytes = (*world->portPixMap)->rowBytes;
        rowBytes &= 0x3FFF;

        return data[row * rowBytes + offset];
    }

    uint16_t data16(int row, int offset)
    {
        return data(row, offset*2) * 256 + data(row, offset*2+1);
    }
    uint32_t data32(int row, int offset)
    {
        return data16(row, offset*2) * 65536 + data16(row, offset*2+1);
    }
};


TEST(QuickDraw, BWLine1)
{
    OffscreenPort port;

    EraseRect(&port.r);
    MoveTo(0,0);
    LineTo(2,2);

    EXPECT_EQ(0x80, port.data(0,0));
    EXPECT_EQ(0x40, port.data(1,0));
}

TEST(QuickDraw, GWorld8)
{
    OffscreenWorld world(8);

    EraseRect(&world.r);
    MoveTo(0,0);
    LineTo(2,2);
    
    EXPECT_EQ(255, world.data(0,0));
    EXPECT_EQ(0, world.data(0,1));
    EXPECT_EQ(0, world.data(1,0));
    EXPECT_EQ(255, world.data(1,1));
}

TEST(QuickDraw, GWorld16)
{
    OffscreenWorld world(16);

    EraseRect(&world.r);
    MoveTo(0,0);
    LineTo(2,2);
    
    EXPECT_EQ(0, world.data16(0,0));
    EXPECT_EQ(0x7FFF, world.data16(0,1));
}

TEST(QuickDraw, GWorld32)
{
    OffscreenWorld world(32);

    EraseRect(&world.r);
    MoveTo(0,0);
    LineTo(2,2);
    
    EXPECT_EQ(0, world.data32(0,0));
    EXPECT_EQ(0x00FFFFFF, world.data32(0,1));
}

TEST(QuickDraw, BasicQDColorBW)
{
    OffscreenPort port;

    FillRect(&port.r, PATREF(qd.gray));
    EXPECT_EQ(0x80, port.data(0,0));
    EXPECT_EQ(0x40, port.data(1,0));
    
    ForeColor(whiteColor);
    BackColor(redColor);
    FillRect(&port.r, PATREF(qd.gray));

    EXPECT_EQ(0x40, port.data(0,0));
    EXPECT_EQ(0x80, port.data(1,0));

    EXPECT_EQ(whiteColor, port.port.fgColor);
    EXPECT_EQ(redColor, port.port.bkColor);
}

TEST(QuickDraw, BasicQDColor32)
{
    OffscreenWorld world(32);


    FillRect(&world.r, PATREF(qd.gray));
    EXPECT_EQ(0x00000000, world.data32(0,0));
    EXPECT_EQ(0x00FFFFFF, world.data32(0,1));
    EXPECT_EQ(0x00FFFFFF, world.data32(1,0));
    EXPECT_EQ(0x00000000, world.data32(1,1));
    
    ForeColor(whiteColor);
    BackColor(redColor);

    const uint32_t rgbRed = 0xDD0806;

    EXPECT_EQ(0x00FFFFFF, world.world->fgColor);
    EXPECT_EQ(rgbRed, world.world->bkColor);

    FillRect(&world.r, PATREF(qd.gray));

    EXPECT_EQ(0x00FFFFFF, world.data32(0,0));
    EXPECT_EQ(rgbRed, world.data32(0,1));
    EXPECT_EQ(rgbRed, world.data32(1,0));
    EXPECT_EQ(0x00FFFFFF, world.data32(1,1));
} 

TEST(QuickDraw, GrayPattern32)
{
    OffscreenWorld world(32);

    FillRect(&world.r, PATREF(qd.gray));
    EXPECT_EQ(0x00000000, world.data32(0,0));
    EXPECT_EQ(0x00FFFFFF, world.data32(0,1));
    EXPECT_EQ(0x00FFFFFF, world.data32(1,0));
    EXPECT_EQ(0x00000000, world.data32(1,1));
}

TEST(QuickDraw, GrayPattern8)
{
    OffscreenWorld world(8);

    FillRect(&world.r, PATREF(qd.gray));
    EXPECT_EQ(0xFF, world.data(0,0));
    EXPECT_EQ(0x00, world.data(0,1));
    EXPECT_EQ(0x00, world.data(1,0));
    EXPECT_EQ(0xFF, world.data(1,1));
}

TEST(QuickDraw, GrayPattern1)
{
    OffscreenPort port;

    FillRect(&port.r, PATREF(qd.gray));
    EXPECT_EQ(0x80, port.data(0,0));
    EXPECT_EQ(0x40, port.data(1,0));
}

TEST(QuickDraw, BlackPattern32)
{
    OffscreenWorld world(32);

    FillRect(&world.r, PATREF(qd.black));
    EXPECT_EQ(0x00000000, world.data32(0,0));
    EXPECT_EQ(0x00000000, world.data32(0,1));
    EXPECT_EQ(0x00000000, world.data32(1,0));
    EXPECT_EQ(0x00000000, world.data32(1,1));
}
