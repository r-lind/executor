#include "gtest/gtest.h"

#ifdef EXECUTOR
#define AUTOMATIC_CONVERSIONS
#include <QuickDraw.h>
#include <CQuickDraw.h>
#include <MemoryMgr.h>

using namespace Executor;
extern QDGlobals qd;

#define PTR(x) (&inout(x))

    // FIXME: Executor still defines Pattern as an array, rather than an array wrapped in a struct
#define PATREF(pat) (pat)

inline bool hasDeepGWorlds()
{
    return true;
}
#else
#include <Quickdraw.h>
#include <QDOffscreen.h>
#include <Gestalt.h>

#define PTR(x) (&x)
#define PATREF(pat) (&(pat))


inline bool hasDeepGWorlds()
{
    long resp;
    if(Gestalt(gestaltQuickdrawFeatures, &resp))
        return false;
    return resp & (1 << gestaltHasDeepGWorlds);
}
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

    uint8_t& data(int row, int offset)
    {
        return reinterpret_cast<uint8_t&>(
            port.portBits.baseAddr[row * port.portBits.rowBytes + offset]);
    }
};

struct OffscreenWorld
{
    GWorldPtr world = nullptr;
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

    uint8_t& data(int row, int offset)
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
    if(!hasDeepGWorlds())
        return;
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
    if(!hasDeepGWorlds())
        return;
    OffscreenWorld world(16);

    EraseRect(&world.r);
    MoveTo(0,0);
    LineTo(2,2);
    
    EXPECT_EQ(0, world.data16(0,0));
    EXPECT_EQ(0x7FFF, world.data16(0,1));
}

TEST(QuickDraw, GWorld32)
{
    if(!hasDeepGWorlds())
        return;
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
    if(!hasDeepGWorlds())
        return;
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
    if(!hasDeepGWorlds())
        return;
    OffscreenWorld world(32);

    FillRect(&world.r, PATREF(qd.gray));
    EXPECT_EQ(0x00000000, world.data32(0,0));
    EXPECT_EQ(0x00FFFFFF, world.data32(0,1));
    EXPECT_EQ(0x00FFFFFF, world.data32(1,0));
    EXPECT_EQ(0x00000000, world.data32(1,1));
}

TEST(QuickDraw, GrayPattern8)
{
    if(!hasDeepGWorlds())
        return;
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
    if(!hasDeepGWorlds())
        return;
    OffscreenWorld world(32);

    FillRect(&world.r, PATREF(qd.black));
    EXPECT_EQ(0x00000000, world.data32(0,0));
    EXPECT_EQ(0x00000000, world.data32(0,1));
    EXPECT_EQ(0x00000000, world.data32(1,0));
    EXPECT_EQ(0x00000000, world.data32(1,1));
}

TEST(QuickDraw, CopyBit8)
{
    if(!hasDeepGWorlds())
        return;
    OffscreenWorld world1(8);

    world1.data(0,0) = 1;
    world1.data(0,1) = 2;
    world1.data(1,0) = 3;
    world1.data(1,1) = 4;
    OffscreenWorld world2(8);
    EraseRect(&world2.r);

    RgnHandle rgn1 = NewRgn();
    SetRectRgn(rgn1, 0,0,2,1);
    RgnHandle rgn2 = NewRgn();
    SetRectRgn(rgn2, 0,0,1,2);
    UnionRgn(rgn1, rgn2, rgn1);
    DisposeRgn(rgn2);

    CopyBits(
        &GrafPtr(world1.world)->portBits,
        &GrafPtr(world2.world)->portBits,
        &world1.r, &world2.r, srcCopy, rgn1);

    DisposeRgn(rgn1);

    EXPECT_EQ(1, world2.data(0,0));
    EXPECT_EQ(2, world2.data(0,1));
    EXPECT_EQ(3, world2.data(1,0));
    EXPECT_EQ(0, world2.data(1,1));
}

TEST(QuickDraw, CopyBit32)
{
    if(!hasDeepGWorlds())
        return;
    OffscreenWorld world1(32);

    world1.data(0,3) = 1;
    world1.data(0,7) = 2;
    world1.data(1,3) = 3;
    world1.data(1,7) = 4;
    OffscreenWorld world2(32);
    EraseRect(&world2.r);

    RgnHandle rgn1 = NewRgn();
    SetRectRgn(rgn1, 0,0,2,1);
    RgnHandle rgn2 = NewRgn();
    SetRectRgn(rgn2, 0,0,1,2);
    UnionRgn(rgn1, rgn2, rgn1);
    DisposeRgn(rgn2);

    CopyBits(
        &GrafPtr(world1.world)->portBits,
        &GrafPtr(world2.world)->portBits,
        &world1.r, &world2.r, srcCopy, rgn1);

    DisposeRgn(rgn1);

    EXPECT_EQ(1, world2.data(0,3));
    EXPECT_EQ(2, world2.data(0,7));
    EXPECT_EQ(3, world2.data(1,3));
    EXPECT_EQ(255, world2.data(1,7));
}

TEST(QuickDraw, CopyMask8)
{
    if(!hasDeepGWorlds())
        return;
    OffscreenPort mask;

    mask.data(0,0) = 0xC0;
    mask.data(1,0) = 0x80;
    
    OffscreenWorld world1(8);

    world1.data(0,0) = 1;
    world1.data(0,1) = 2;
    world1.data(1,0) = 3;
    world1.data(1,1) = 4;
    OffscreenWorld world2(8);
    EraseRect(&world2.r);

    CopyMask(
        &GrafPtr(world1.world)->portBits,
        &mask.port.portBits,
        &GrafPtr(world2.world)->portBits,
        &world1.r, &mask.r, &world2.r);


    EXPECT_EQ(1, world2.data(0,0));
    EXPECT_EQ(2, world2.data(0,1));
    EXPECT_EQ(3, world2.data(1,0));
    EXPECT_EQ(0, world2.data(1,1));
}

TEST(QuickDraw, CopyMask32)
{
    if(!hasDeepGWorlds())
        return;
    OffscreenPort mask;

    mask.data(0,0) = 0xC0;
    mask.data(1,0) = 0x80;
    
    OffscreenWorld world1(32);

    world1.data(0,3) = 1;
    world1.data(0,7) = 2;
    world1.data(1,3) = 3;
    world1.data(1,7) = 4;
    OffscreenWorld world2(32);
    EraseRect(&world2.r);

    CopyMask(
        &GrafPtr(world1.world)->portBits,
        &mask.port.portBits,
        &GrafPtr(world2.world)->portBits,
        &world1.r, &mask.r, &world2.r);


    EXPECT_EQ(1, world2.data(0,3));
    EXPECT_EQ(2, world2.data(0,7));
    EXPECT_EQ(3, world2.data(1,3));
    EXPECT_EQ(255, world2.data(1,7));
}
