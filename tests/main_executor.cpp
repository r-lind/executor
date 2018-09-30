#include "gtest/gtest.h"
#define AUTOMATIC_CONVERSIONS
#include <FileMgr.h>
#include <rsys/file.h>
#include <MemoryMgr.h>

using namespace Executor;

class ExecutorTestEnvironment : public testing::Environment
{
    char *thingOnStack;
public:
    ExecutorTestEnvironment(char* thingOnStack) : thingOnStack(thingOnStack) {}

    virtual void SetUp() override
    {
        Executor::InitMemory(thingOnStack);
        Executor::ROMlib_fileinit();

        if(auto fsspec = nativePathToFSSpec(fs::current_path()))
        {
            WDPBRec wdpb;

            wdpb.ioVRefNum = fsspec->vRefNum;
            wdpb.ioWDDirID = fsspec->parID;
            wdpb.ioWDProcID = CLC(FOURCC('X', 'c', 't', 'r'));
            wdpb.ioNamePtr = fsspec->name;
            PBOpenWD(&wdpb, false);
            SetVol(nullptr, wdpb.ioVRefNum);
        }
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
