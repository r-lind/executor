#include "gtest/gtest.h"

#include "compat.h"

#ifdef EXECUTOR
#include <ResourceMgr.h>
#include <FileMgr.h>
#else
#include <Resources.h>
#include <Files.h>
#endif

class ResourceTest : public testing::Test
{
public:
    short ref = -1;
    StringPtr filename;

    ResourceTest()
    {
        filename = PSTR("test-resfile");
        CreateResFile(filename);
        EXPECT_EQ(noErr, ResError());
        ref = OpenResFile(filename);
        EXPECT_EQ(noErr, ResError());
        EXPECT_NE(-1, ref);
    }

    ~ResourceTest()
    {
        if(ref != -1)
            CloseResFile(ref);
        FSDelete(filename,0);
    }
};

TEST_F(ResourceTest, CreateDelete)
{
    EXPECT_EQ(0, GetResFileAttrs(ref) & 0xE0);
}

TEST_F(ResourceTest, CreateDuplicate)
{
    CreateResFile(filename);
    EXPECT_EQ(opWrErr, ResError());
}

TEST_F(ResourceTest, GetResourceNotFound)
{
    Handle h;
    h = Get1Resource('QUUX', 128);
    EXPECT_EQ(nullptr, h);
    EXPECT_EQ(noErr, ResError());

    h = GetResource('QUUX', 128);
    EXPECT_EQ(nullptr, h);
    EXPECT_EQ(noErr, ResError());
}

TEST_F(ResourceTest, AddResource)
{
    Handle h;
    h = NewHandle(4);
    *(GUEST<uint32_t>*)*h = 0xDEADBEEF;
    ASSERT_NE(nullptr, h);

    AddResource(h, 'QUUX', 128, PSTR("Hello, world."));
    EXPECT_EQ(noErr, ResError());

    EXPECT_EQ(mapChanged, GetResFileAttrs(ref) & mapChanged);
    EXPECT_EQ(0, GetResFileAttrs(ref) & mapReadOnly);
    //EXPECT_EQ(mapCompact, GetResFileAttrs(ref) & mapCompact);

    Handle h2 = GetResource('QUUX', 128);
    EXPECT_EQ(h, h2);
    EXPECT_EQ(noErr, ResError());

        // ReleaseResource is documented to do nothing if resChanged is set
    ReleaseResource(h);
    EXPECT_NE(nullptr, *h);

        // .. so a new handle will not reuse the same address
    h2 = NewHandleClear(4);
    DisposeHandle(h2);
    EXPECT_NE(h, h2);

    h2 = GetResource('QUUX', 128);
    EXPECT_EQ(h, h2);
    
    CloseResFile(ref);

    h = GetResource('QUUX', 128);
    EXPECT_EQ(nullptr, h);

    ref = OpenResFile(filename);
    EXPECT_EQ(0, GetResFileAttrs(ref) & mapChanged);
    EXPECT_EQ(0, GetResFileAttrs(ref) & mapReadOnly);

    h = GetResource('QUUX', 128);
    EXPECT_NE(nullptr, h);
    EXPECT_NE(nullptr, *h);
    EXPECT_EQ(0xDEADBEEF, *(GUEST<uint32_t>*)*h);

    DetachResource(h);

    h2 = GetResource('QUUX', 128);
    EXPECT_NE(nullptr, h2);
    EXPECT_NE(nullptr, *h2);
    EXPECT_NE(h, h2);
    EXPECT_EQ(0xDEADBEEF, *(GUEST<uint32_t>*)*h2);

    ReleaseResource(h2);    // not really a way to check this

        // .. a new handle might reuse the same address
    Handle h3 = NewHandleClear(4);
        // but it's not guaranteed to.

        // but now the next GetResource probably won't reuse the
        // address.
    Handle h4 = GetResource('QUUX', 128);
    EXPECT_NE(nullptr, h4);
    EXPECT_NE(h2, h4);

    DisposeHandle(h3);
    DisposeHandle(h);
}
