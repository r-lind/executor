#include "gtest/gtest.h"

#include "compat.h"

#ifdef EXECUTOR
#include <CQuickDraw.h>
#else
#include <Quickdraw.h>
#include <QDOffscreen.h>
#include <Windows.h>
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

    FillRect(&port.r, &qd.gray);
    EXPECT_EQ(0x80, port.data(0,0));
    EXPECT_EQ(0x40, port.data(1,0));
    
    ForeColor(whiteColor);
    BackColor(redColor);
    FillRect(&port.r, &qd.gray);

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


    FillRect(&world.r, &qd.gray);
    EXPECT_EQ(0x00000000, world.data32(0,0));
    EXPECT_EQ(0x00FFFFFF, world.data32(0,1));
    EXPECT_EQ(0x00FFFFFF, world.data32(1,0));
    EXPECT_EQ(0x00000000, world.data32(1,1));
    
    ForeColor(whiteColor);
    BackColor(redColor);

    const uint32_t rgbRed = 0xDD0806;

    EXPECT_EQ(0x00FFFFFF, world.world->fgColor);
    EXPECT_EQ(rgbRed, world.world->bkColor);

    FillRect(&world.r, &qd.gray);

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

    FillRect(&world.r, &qd.gray);
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

    FillRect(&world.r, &qd.gray);
    EXPECT_EQ(0xFF, world.data(0,0));
    EXPECT_EQ(0x00, world.data(0,1));
    EXPECT_EQ(0x00, world.data(1,0));
    EXPECT_EQ(0xFF, world.data(1,1));
}

TEST(QuickDraw, GrayPattern1)
{
    OffscreenPort port;

    FillRect(&port.r, &qd.gray);
    EXPECT_EQ(0x80, port.data(0,0));
    EXPECT_EQ(0x40, port.data(1,0));
}

TEST(QuickDraw, BlackPattern32)
{
    if(!hasDeepGWorlds())
        return;
    OffscreenWorld world(32);

    FillRect(&world.r, &qd.black);
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

TEST(QuickDraw, UnionRect)
{
    Rect r1,r2,r;
    
    r1 = {100,200,1000,2000};
    r2 = {1,2,10,20};
    UnionRect(&r1, &r2, &r);

    EXPECT_EQ(1, r.top);
    EXPECT_EQ(2, r.left);
    EXPECT_EQ(1000, r.bottom);
    EXPECT_EQ(2000, r.right);

    r1 = {1000,2000,100,200};
    r2 = {1,2,10,20};
    UnionRect(&r1, &r2, &r);

    EXPECT_EQ(1, r.top);
    EXPECT_EQ(2, r.left);
    EXPECT_EQ(100, r.bottom);
    EXPECT_EQ(200, r.right);

    r1 = {1,2,10,20};
    r2 = {0,0,0,0};
    UnionRect(&r1, &r2, &r);

    EXPECT_EQ(0, r.top);
    EXPECT_EQ(0, r.left);
    EXPECT_EQ(10, r.bottom);
    EXPECT_EQ(20, r.right);
}

TEST(QuickDraw, PinRect)
{
    auto pin = [](Rect& r, Point p) -> Point {
        auto l = PinRect(&r, p);
        return { (int16_t)(l >> 16), (int16_t)l };
    };
    Rect r = { 10,20,30,40 };
    Point p;

    p = pin(r, {15,25});
    EXPECT_EQ(15, p.v);
    EXPECT_EQ(25, p.h);

    p = pin(r, {30,40});
    EXPECT_EQ(29, p.v);
    EXPECT_EQ(39, p.h);

    p = pin(r, {35,45});
    EXPECT_EQ(29, p.v);
    EXPECT_EQ(39, p.h);
}
