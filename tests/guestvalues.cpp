#include "gtest/gtest.h"
#define AUTOMATIC_CONVERSIONS
#include <rsys/mactype.h>
#include <rsys/byteswap.h>
using namespace Executor;

TEST(guestvalues, andUL)
{
    EXPECT_EQ(10, GUEST<unsigned long>(10UL) & 0x7FFF);
}

TEST(guestvalues, assignToInt16)
{
    GUEST<int16_t> x;
    int16_t y;

    y = (int8_t)0x81;
    x = (int8_t)0x81;
    EXPECT_EQ(y, x.get());

    y = (int16_t)0x8001;
    x = (int16_t)0x8001;
    EXPECT_EQ(y, x.get());

    y = (int32_t)10;
    x = (int32_t)10;
    EXPECT_EQ(y, x.get());

    y = (int64_t)10;
    x = (int64_t)10;
    EXPECT_EQ(y, x.get());

    y = (uint8_t)0x81;
    x = (uint8_t)0x81;
    EXPECT_EQ(y, x.get());

    y = (uint16_t)10;
    x = (uint16_t)10;
    EXPECT_EQ(y, x.get());

    y = (uint32_t)10;
    x = (uint32_t)10;
    EXPECT_EQ(y, x.get());

    y = (uint64_t)10;
    x = (uint64_t)10;
    EXPECT_EQ(y, x.get());
}

TEST(guestvalues, rawHostOrder)
{
    int x = 1;
    char& littleEndian = *reinterpret_cast<char*>(&x);

    GUEST<uint32_t> a = 0x12345678;
    GUEST<uint16_t> b = 0x1234;
    GUEST<Point> p = {0x1234, 0x5678};

    EXPECT_EQ(0x12345678, a);
    EXPECT_EQ(0x1234, b);
    EXPECT_EQ((Point{0x1234,0x5678}), p);
    
    EXPECT_EQ(0x12345678, a.raw_host_order()) << "As hexadecimal: " << std::hex << a.raw_host_order();
    EXPECT_EQ(0x1234, b.raw_host_order()) << "As hexadecimal: " << std::hex << b.raw_host_order();
    EXPECT_EQ(0x12345678, p.raw_host_order()) << "As hexadecimal: " << std::hex << p.raw_host_order();

    a.raw_host_order(0xBEEFCAFE);
    EXPECT_EQ(0xBEEFCAFE, a);

    b.raw_host_order(0xCAFE);
    EXPECT_EQ(0xCAFE, b);

    p.raw_host_order(0xBEEFCAFE);
    EXPECT_EQ((Point{(int16_t)0xBEEF, (int16_t)0xCAFE}), p);
}
