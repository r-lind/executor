#include "gtest/gtest.h"
#define AUTOMATIC_CONVERSIONS
#include <FileMgr.h>
#include <rsys/file.h>
#include <MemoryMgr.h>

using namespace Executor;

QDGlobals qd;
#include <rsys/cquick.h>
#include <rsys/vdriver.h>
#include <ResourceMgr.h>

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

class ExecutorTestEnvironment : public testing::Environment
{
    char *thingOnStack;
    fs::path tempDir;
public:
    ExecutorTestEnvironment(char* thingOnStack) : thingOnStack(thingOnStack) {}

    virtual void SetUp() override
    {
        tempDir = fs::current_path() / fs::unique_path("temptest-%%%%-%%%%-%%%%-%%%%");

        Executor::InitMemory(thingOnStack);
        Executor::ROMlib_fileinit();

        fs::create_directory(tempDir);

        if(auto fsspec = nativePathToFSSpec(tempDir))
        {
            WDPBRec wdpb;

            wdpb.ioVRefNum = fsspec->vRefNum;
            wdpb.ioWDDirID = fsspec->parID;
            wdpb.ioWDProcID = CLC(FOURCC('X', 'c', 't', 'r'));
            wdpb.ioNamePtr = fsspec->name;
            PBOpenWD(&wdpb, false);
            SetVol(nullptr, wdpb.ioVRefNum);
        }

        vdriver = new MockVDriver();
        initialize_68k_emulator(nullptr, false, (uint32_t *)SYN68K_TO_US(0), 0);
        InitResources();
        ROMlib_InitGDevices();
        LM(TheZone) = LM(ApplZone);

        ROMlib_color_init();

        InitGraf((Ptr)&qd.thePort);
    }

    virtual void TearDown() override
    {
        if(!tempDir.empty())
            fs::remove_all(tempDir);
    }
};

TEST(MemoryMgr, NewClearGetSizeDispose)
{
    Ptr p = NewPtrClear(42);

    ASSERT_NE(nullptr, p);
    ASSERT_EQ(0, *p);

    ASSERT_EQ(42, GetPtrSize(p));

    DisposePtr(p);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    char thingOnStack;
    testing::AddGlobalTestEnvironment(new ExecutorTestEnvironment(&thingOnStack));

    int result = RUN_ALL_TESTS();
    return result;
}
