#include "gtest/gtest.h"

#include <Files.h>
#include <Devices.h>

TEST(Files, CreateDelete)
{
    ParamBlockRec pb;
    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = 0;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test";
    PBCreateSync(&pb);

    ASSERT_EQ(noErr, pb.ioParam.ioResult);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = 0;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test";
    PBDeleteSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = 0;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test";
    PBDeleteSync(&pb);

    EXPECT_EQ(fnfErr, pb.ioParam.ioResult);
}

class FileTest : public testing::Test
{
protected:
    Str255 file1 = "\ptemp-test";
    short vRefNum = 0;
    long dirID = 0;
    short refNum = -1;

    FileTest()
    {
        HParamBlockRec pb;
        memset(&pb, 42, sizeof(pb));
        pb.fileParam.ioCompletion = nullptr;
        pb.fileParam.ioVRefNum = vRefNum;
        pb.fileParam.ioNamePtr = file1;
        pb.fileParam.ioDirID = dirID;
        PBHCreateSync(&pb);

        EXPECT_EQ(noErr, pb.fileParam.ioResult);
    }

    ~FileTest()
    {
        close();

        HParamBlockRec pb;
        memset(&pb, 42, sizeof(pb));
        pb.fileParam.ioCompletion = nullptr;
        pb.fileParam.ioVRefNum = vRefNum;
        pb.fileParam.ioDirID = dirID;
        pb.fileParam.ioNamePtr = file1;
        PBHDeleteSync(&pb);

        EXPECT_EQ(noErr, pb.fileParam.ioResult);
    }

    void close(short rn)
    {
        if(refNum > 0)
        {
            ParamBlockRec pb;
            memset(&pb, 42, sizeof(pb));
            pb.ioParam.ioCompletion = nullptr;
            pb.ioParam.ioRefNum = rn;
            PBCloseSync(&pb);

            EXPECT_EQ(noErr, pb.fileParam.ioResult);
        }
    }


    void close()
    {
        if(refNum > 0)
            close(refNum);
        refNum = -1;
    }

    template<class F>
    short open(F OpenFun, SInt8 perm)
    {
        HParamBlockRec hpb;
        memset(&hpb, 42, sizeof(hpb));
        hpb.ioParam.ioCompletion = nullptr;
        hpb.ioParam.ioVRefNum = vRefNum;
        hpb.fileParam.ioDirID = dirID;
        hpb.ioParam.ioNamePtr = file1;
        hpb.ioParam.ioPermssn = perm;
        OpenFun(&hpb);

        EXPECT_EQ(noErr, hpb.fileParam.ioResult);
        EXPECT_GE(hpb.ioParam.ioRefNum, 2);

        if(hpb.fileParam.ioResult == noErr)
            refNum = hpb.ioParam.ioRefNum;
        return refNum;
    }

    short open(SInt8 perm = fsCurPerm)
    {
        return open([](auto pbp){PBHOpenSync(pbp);}, perm);
    }
    short openDF(SInt8 perm = fsCurPerm)
    {
        return open([](auto pbp){PBHOpenDFSync(pbp);}, perm);
    }
    short openRF(SInt8 perm = fsCurPerm)
    {
        return open([](auto pbp){PBHOpenRFSync(pbp);}, perm);
    }

    void hello()
    {
        open();

        char buf1[] = "Hello, world.";
        char buf2[100];

        ParamBlockRec pb;
        memset(&pb, 42, sizeof(pb));
        pb.ioParam.ioCompletion = nullptr;
        pb.ioParam.ioRefNum = refNum;
        pb.ioParam.ioPosMode = fsFromStart;
        pb.ioParam.ioPosOffset = 0;
        pb.ioParam.ioBuffer = buf1;
        pb.ioParam.ioReqCount = 13;
        PBWriteSync(&pb);

        EXPECT_EQ(noErr, pb.ioParam.ioResult);
        EXPECT_EQ(13, pb.ioParam.ioActCount);

        close();
    }

};



TEST_F(FileTest, Open)
{
    open();
}

TEST_F(FileTest, OpenDF)
{
    openDF();
}
TEST_F(FileTest, OpenRF)
{
    openRF();
}

TEST_F(FileTest, OpenTwiceForRead)
{
    open(fsRdPerm);

    HParamBlockRec hpb;
    memset(&hpb, 42, sizeof(hpb));
    hpb.ioParam.ioCompletion = nullptr;
    hpb.ioParam.ioVRefNum = vRefNum;
    hpb.fileParam.ioDirID = dirID;
    hpb.ioParam.ioNamePtr = file1;
    hpb.ioParam.ioPermssn = fsRdPerm;
    PBHOpenSync(&hpb);

    EXPECT_EQ(noErr, hpb.fileParam.ioResult);
    EXPECT_GE(hpb.ioParam.ioRefNum, 2);

    if(hpb.fileParam.ioResult == noErr)
    {
        short ref2 = hpb.ioParam.ioRefNum;

        EXPECT_NE(refNum, ref2);
        if(ref2 > 0 && refNum != ref2)
            close(ref2);
    }
}

TEST_F(FileTest, OpenTwiceForWrite)
{
    open(fsRdWrPerm);

    HParamBlockRec hpb;
    memset(&hpb, 42, sizeof(hpb));
    hpb.ioParam.ioCompletion = nullptr;
    hpb.ioParam.ioVRefNum = vRefNum;
    hpb.fileParam.ioDirID = dirID;
    hpb.ioParam.ioNamePtr = file1;
    hpb.ioParam.ioPermssn = fsRdWrPerm;
    PBHOpenSync(&hpb);

    EXPECT_EQ(opWrErr, hpb.fileParam.ioResult);
    EXPECT_GE(hpb.ioParam.ioRefNum, 2);
    EXPECT_EQ(hpb.ioParam.ioRefNum, refNum);

    if(hpb.fileParam.ioResult == noErr || hpb.fileParam.ioResult == opWrErr)
    {
        short ref2 = hpb.ioParam.ioRefNum;

        if(ref2 > 0 && refNum != ref2)
            close(ref2);
    }
}

TEST_F(FileTest, WriteRead)
{
    open();

    char buf1[] = "Hello, world.";
    char buf2[100];

    ParamBlockRec pb;
    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioPosMode = fsFromStart;
    pb.ioParam.ioPosOffset = 0;
    pb.ioParam.ioBuffer = buf1;
    pb.ioParam.ioReqCount = 13;
    PBWriteSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);
    EXPECT_EQ(13, pb.ioParam.ioActCount);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioPosMode = fsFromStart;
    pb.ioParam.ioPosOffset = 0;
    pb.ioParam.ioBuffer = buf2;
    pb.ioParam.ioReqCount = 13;
    PBReadSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);
    EXPECT_EQ(13, pb.ioParam.ioActCount);

    EXPECT_EQ(0, memcmp(buf1, buf2, 13));


    close();


    memset(buf2, 0, 13);
    open();

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioPosMode = fsFromStart;
    pb.ioParam.ioPosOffset = 0;
    pb.ioParam.ioBuffer = buf2;
    pb.ioParam.ioReqCount = 13;
    PBReadSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);
    EXPECT_EQ(13, pb.ioParam.ioActCount);

    EXPECT_EQ(0, memcmp(buf1, buf2, 13));
}


TEST_F(FileTest, Seek)
{
    hello();
    open(fsRdPerm);

    ParamBlockRec pb;

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioPosMode = fsAtMark;
    pb.ioParam.ioPosOffset = -1;
    PBSetFPosSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);
    EXPECT_EQ(0, pb.ioParam.ioPosOffset);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioPosMode = fsFromStart;
    pb.ioParam.ioPosOffset = 3;
    PBSetFPosSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);
    EXPECT_EQ(3, pb.ioParam.ioPosOffset);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioPosMode = fsFromMark;
    pb.ioParam.ioPosOffset = 4;
    PBSetFPosSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);
    EXPECT_EQ(7, pb.ioParam.ioPosOffset);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioPosMode = fsFromLEOF;
    pb.ioParam.ioPosOffset = -3;
    PBSetFPosSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);
    EXPECT_EQ(10, pb.ioParam.ioPosOffset);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioPosMode = fsFromStart;
    pb.ioParam.ioPosOffset = 42;
    PBSetFPosSync(&pb);

    EXPECT_EQ(eofErr, pb.ioParam.ioResult);
    EXPECT_EQ(13, pb.ioParam.ioPosOffset);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioPosMode = fsFromLEOF;
    pb.ioParam.ioPosOffset = -42;
    PBSetFPosSync(&pb);

    EXPECT_EQ(posErr, pb.ioParam.ioResult);
    EXPECT_PRED1(
        [](long off) { 
            return off == 13        // Mac OS 9
                || off == 13-42;    // System 6
        }, pb.ioParam.ioPosOffset);

}