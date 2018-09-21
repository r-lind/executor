#include "gtest/gtest.h"
#define AUTOMATIC_CONVERSIONS
#include <FileMgr.h>
#include <rsys/file.h>
#include <MemoryMgr.h>

using namespace Executor;

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
    Executor::InitMemory(&thingOnStack);

    memset(&LM(DrvQHdr), 0, sizeof(LM(DrvQHdr)));
    memset(&LM(VCBQHdr), 0, sizeof(LM(VCBQHdr)));
    memset(&LM(FSQHdr), 0, sizeof(LM(FSQHdr)));
    LM(DefVCBPtr) = 0;
    LM(FSFCBLen) = CWC(94);

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

    int result = RUN_ALL_TESTS();
    return result;
}
