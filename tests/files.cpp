#include "gtest/gtest.h"

#include <Files.h>
#include <Devices.h>

TEST(Files, GetWDInfo)
{
    WDPBRec wdpb;
    memset(&wdpb, 42, sizeof(wdpb));
    wdpb.ioCompletion = nullptr;
    wdpb.ioVRefNum = 0;
    Str255 buf;
    wdpb.ioNamePtr = nullptr;
    wdpb.ioWDIndex = 0;

    PBGetWDInfoSync(&wdpb);
    ASSERT_EQ(noErr, wdpb.ioResult);

    short vRefNum = wdpb.ioWDVRefNum;
    long dirID = wdpb.ioWDDirID;

    EXPECT_LT(vRefNum, 0);
    EXPECT_GT(dirID, 0);
}

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

TEST(Files, CreateDeleteDir)
{
    HParamBlockRec pb;
    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = 0;
    pb.fileParam.ioDirID = 0;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test";
    PBDirCreateSync(&pb);

    ASSERT_EQ(noErr, pb.ioParam.ioResult);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = 0;
    pb.fileParam.ioDirID = 0;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test";
    PBHDeleteSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = 0;
    pb.fileParam.ioDirID = 0;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test";
    PBHDeleteSync(&pb);

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
        if(dirID == 0 || vRefNum == 0)
        {
            WDPBRec wdpb;
            memset(&wdpb, 42, sizeof(wdpb));
            wdpb.ioCompletion = nullptr;
            wdpb.ioVRefNum = vRefNum;
            Str255 buf;
            wdpb.ioNamePtr = nullptr;
            wdpb.ioWDIndex = 0;

            PBGetWDInfoSync(&wdpb);
            EXPECT_EQ(noErr, wdpb.ioResult);

            vRefNum = wdpb.ioWDVRefNum;
            dirID = wdpb.ioWDDirID;
        }

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
        return open([](auto pbp){
            PBHOpenDFSync(pbp);
            if(pbp->ioParam.ioResult == paramErr)
                PBHOpenSync(pbp);
        }, perm);
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


TEST_F(FileTest, ReadEOF)
{
    hello();
    open();

    char buf1[] = "Hello, world.";
    char buf2[100];

    ParamBlockRec pb;

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioPosMode = fsFromStart;
    pb.ioParam.ioPosOffset = 0;
    pb.ioParam.ioBuffer = buf2;
    pb.ioParam.ioReqCount = 26;
    PBReadSync(&pb);

    EXPECT_EQ(eofErr, pb.ioParam.ioResult);
    EXPECT_EQ(13, pb.ioParam.ioActCount);
    EXPECT_EQ(13, pb.ioParam.ioPosOffset);

    EXPECT_EQ(0, memcmp(buf1, buf2, 13));
}

TEST_F(FileTest, CaseInsensitive)
{
    CInfoPBRec ipb;

    Str255 name = "\pTEMP-test";

    // Files are found with case-insensitive match on file names
    // but GetCatInfo does not write to ioNamePtr

    memset(&ipb, 42, sizeof(ipb));
    ipb.hFileInfo.ioCompletion = nullptr;
    ipb.hFileInfo.ioVRefNum = vRefNum;
    Str255 buf;
    ipb.hFileInfo.ioNamePtr = name;
    ipb.hFileInfo.ioFDirIndex = 0;
    ipb.hFileInfo.ioDirID = dirID;

    PBGetCatInfoSync(&ipb);

    EXPECT_EQ(noErr, ipb.hFileInfo.ioResult);

    EXPECT_EQ("TEMP-test", std::string(name + 1, name + 1 + name[0]));
}

TEST_F(FileTest, Pathname)
{
    CInfoPBRec ipb;
    long curDirID = dirID;
    std::string path(file1 + 1, file1 + 1 + file1[0]);



    for(int i = 0; i < 100 && curDirID != 1; i++)
    {
        memset(&ipb, 42, sizeof(ipb));
        ipb.dirInfo.ioCompletion = nullptr;
        ipb.dirInfo.ioVRefNum = vRefNum;
        Str255 buf;
        ipb.dirInfo.ioNamePtr = buf;
        ipb.dirInfo.ioFDirIndex = -1;
        ipb.dirInfo.ioDrDirID = curDirID;

        PBGetCatInfoSync(&ipb);

        ASSERT_EQ(noErr, ipb.dirInfo.ioResult);

        if(curDirID == 2)
        {
            EXPECT_EQ(1, ipb.dirInfo.ioDrParID);
        }
        curDirID = ipb.dirInfo.ioDrParID;
        
        EXPECT_EQ(vRefNum, ipb.dirInfo.ioVRefNum);
        path = std::string(buf + 1, buf + 1 + buf[0]) + ":" + path;
    };

    ASSERT_EQ(curDirID, 1);

    EXPECT_NE("", path);
    EXPECT_NE(std::string::npos, path.find(':'));
    EXPECT_NE(0, path.find(':'));

    //printf("%s\n", path.c_str());

    hello();

    Str255 pathP;
    pathP[0] = std::min(size_t(255), path.size());
    memcpy(pathP+1, path.data(), pathP[0]);


    memset(&ipb, 42, sizeof(ipb));
    ipb.hFileInfo.ioCompletion = nullptr;
    ipb.hFileInfo.ioVRefNum = 0;
    ipb.hFileInfo.ioNamePtr = pathP;
    ipb.hFileInfo.ioFDirIndex = 0;
    ipb.hFileInfo.ioDirID = 0;

    PBGetCatInfoSync(&ipb);

    EXPECT_EQ(noErr, ipb.hFileInfo.ioResult);
    EXPECT_NE(0, ipb.hFileInfo.ioFlParID);


    HParamBlockRec hpb;
    memset(&hpb, 42, sizeof(hpb));
    hpb.ioParam.ioCompletion = nullptr;
    hpb.ioParam.ioVRefNum = 0;
    hpb.fileParam.ioDirID = 0;
    hpb.ioParam.ioNamePtr = pathP;
    hpb.ioParam.ioPermssn = fsRdPerm;
    PBHOpenSync(&hpb);

    EXPECT_EQ(noErr, hpb.fileParam.ioResult);
    EXPECT_GE(hpb.ioParam.ioRefNum, 2);

    if(hpb.fileParam.ioResult == noErr)
    {
        refNum = hpb.ioParam.ioRefNum;

        char buf1[] = "Hello, world.";
        char buf2[100];

        ParamBlockRec pb;

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

TEST(Files, DeleteFullDirectory)
{
    HParamBlockRec pb;
    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = 0;
    pb.fileParam.ioDirID = 0;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBDirCreateSync(&pb);

    ASSERT_EQ(noErr, pb.ioParam.ioResult);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = 0;
    pb.fileParam.ioDirID = 0;
    pb.ioParam.ioNamePtr = (StringPtr)"\p:temp-test-dir:file in dir";
    PBHCreateSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = 0;
    pb.fileParam.ioDirID = 0;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBHDeleteSync(&pb);

    EXPECT_EQ(fBsyErr, pb.ioParam.ioResult);


    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = 0;
    pb.fileParam.ioDirID = 0;
    pb.ioParam.ioNamePtr = (StringPtr)"\p:temp-test-dir:file in dir";
    PBHDeleteSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = 0;
    pb.fileParam.ioDirID = 0;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBHDeleteSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);
}


TEST_F(FileTest, UpDirRelative)
{
    HParamBlockRec hpb;
    memset(&hpb, 42, sizeof(hpb));
    hpb.ioParam.ioCompletion = nullptr;
    hpb.ioParam.ioVRefNum = vRefNum;
    hpb.fileParam.ioDirID = dirID;
    hpb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBDirCreateSync(&hpb);

    ASSERT_EQ(noErr, hpb.ioParam.ioResult);

    long tempDirID = hpb.fileParam.ioDirID;

    Str255 relPath = "\p::temp-test";
    memset(&hpb, 42, sizeof(hpb));
    hpb.ioParam.ioCompletion = nullptr;
    hpb.ioParam.ioVRefNum = vRefNum;
    hpb.fileParam.ioDirID = tempDirID;
    hpb.ioParam.ioNamePtr = relPath;
    hpb.ioParam.ioPermssn = fsRdPerm;
    PBHOpenSync(&hpb);

    EXPECT_EQ(noErr, hpb.fileParam.ioResult);
    EXPECT_GE(hpb.ioParam.ioRefNum, 2);
    refNum = hpb.ioParam.ioRefNum;

    memset(&hpb, 42, sizeof(hpb));
    hpb.ioParam.ioCompletion = nullptr;
    hpb.ioParam.ioVRefNum = vRefNum;
    hpb.fileParam.ioDirID = dirID;
    hpb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBHDeleteSync(&hpb);

    EXPECT_EQ(noErr, hpb.ioParam.ioResult);
}


TEST_F(FileTest, Rename)
{
    HParamBlockRec hpb;
    memset(&hpb, 42, sizeof(hpb));
    hpb.ioParam.ioCompletion = nullptr;
    hpb.ioParam.ioVRefNum = vRefNum;
    hpb.fileParam.ioDirID = dirID;
    hpb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBDirCreateSync(&hpb);

    ASSERT_EQ(noErr, hpb.ioParam.ioResult);

    long tempDirID = hpb.fileParam.ioDirID;

    CInfoPBRec ipb;
    memset(&ipb, 42, sizeof(ipb));
    ipb.hFileInfo.ioCompletion = nullptr;
    ipb.hFileInfo.ioVRefNum = vRefNum;
    ipb.hFileInfo.ioNamePtr = file1;
    ipb.hFileInfo.ioFDirIndex = 0;
    ipb.hFileInfo.ioDirID = dirID;

    PBGetCatInfoSync(&ipb);

    long cnid = ipb.hFileInfo.ioDirID;
    EXPECT_EQ(noErr, ipb.hFileInfo.ioResult);
    EXPECT_GT(cnid, 0);
     

    Str255 fileName2 = "\ptemp-test Renamed";
    Str255 badName = "\p:temp-test-dir:blah";

    hpb.ioParam.ioCompletion = nullptr;
    hpb.ioParam.ioVRefNum = vRefNum;
    hpb.fileParam.ioDirID = dirID;
    hpb.ioParam.ioNamePtr = file1;
    hpb.ioParam.ioMisc = (Ptr)badName;
    
    PBHRenameSync(&hpb);
    EXPECT_EQ(bdNamErr, hpb.ioParam.ioResult);


    hpb.ioParam.ioCompletion = nullptr;
    hpb.ioParam.ioVRefNum = vRefNum;
    hpb.fileParam.ioDirID = dirID;
    hpb.ioParam.ioNamePtr = file1;
    hpb.ioParam.ioMisc = (Ptr)fileName2;
    
    PBHRenameSync(&hpb);
    OSErr renameErr = hpb.ioParam.ioResult;
    EXPECT_EQ(noErr, renameErr);

    memset(&ipb, 42, sizeof(ipb));
    ipb.hFileInfo.ioCompletion = nullptr;
    ipb.hFileInfo.ioVRefNum = vRefNum;
    ipb.hFileInfo.ioNamePtr = fileName2;
    ipb.hFileInfo.ioFDirIndex = 0;
    ipb.hFileInfo.ioDirID = dirID;

    PBGetCatInfoSync(&ipb);

    EXPECT_EQ(noErr, ipb.hFileInfo.ioResult);
    EXPECT_EQ(cnid, ipb.hFileInfo.ioDirID);

    memset(&ipb, 42, sizeof(ipb));
    ipb.hFileInfo.ioCompletion = nullptr;
    ipb.hFileInfo.ioVRefNum = vRefNum;
    ipb.hFileInfo.ioNamePtr = file1;
    ipb.hFileInfo.ioFDirIndex = 0;
    ipb.hFileInfo.ioDirID = dirID;

    PBGetCatInfoSync(&ipb);

    EXPECT_EQ(fnfErr, ipb.hFileInfo.ioResult);


    memset(&hpb, 42, sizeof(hpb));
    hpb.ioParam.ioCompletion = nullptr;
    hpb.ioParam.ioVRefNum = vRefNum;
    hpb.fileParam.ioDirID = dirID;
    hpb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBHDeleteSync(&hpb);

    EXPECT_EQ(noErr, hpb.ioParam.ioResult);

    if(renameErr == noErr)
        memcpy(file1, fileName2, fileName2[0]+1);
}


void doWrite(short refNum, long offset, const char* p, long n)
{
    ParamBlockRec pb;

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioPosMode = fsFromStart;
    pb.ioParam.ioPosOffset = offset;
    pb.ioParam.ioBuffer = (Ptr)p;
    pb.ioParam.ioReqCount = n;
    PBWriteSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);
    EXPECT_EQ(n, pb.ioParam.ioActCount);
    EXPECT_EQ(offset + n, pb.ioParam.ioPosOffset);
}

bool verifyContents(short refNum, const char *p, long n)
{
    char *buf = new char[n + 32];

    ParamBlockRec pb;

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioRefNum = refNum;
    PBGetEOFSync(&pb);
    EXPECT_EQ(noErr, pb.ioParam.ioResult);
    EXPECT_EQ(n, (UInt32)pb.ioParam.ioMisc);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioPosMode = fsFromStart;
    pb.ioParam.ioPosOffset = 0;
    pb.ioParam.ioBuffer = buf;
    pb.ioParam.ioReqCount = n + 32;
    PBReadSync(&pb);

    bool sizeMatches = (n == pb.ioParam.ioActCount);
    EXPECT_EQ(eofErr, pb.ioParam.ioResult);
    EXPECT_EQ(n, pb.ioParam.ioActCount);
    EXPECT_EQ(n, pb.ioParam.ioPosOffset);

    if(sizeMatches)
        EXPECT_EQ(0, memcmp(buf, p, n)) << "comparing " << n << " bytes - got '" << buf << "' expecting '" << p << "'";
    bool memMatches = memcmp(buf, p, n) == 0;
    
    delete[] buf;

    return sizeMatches && memMatches;
}
void setEOF(short refNum, long n)
{
    ParamBlockRec pb;

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioRefNum = refNum;
    pb.ioParam.ioMisc = (Ptr) n;
    PBSetEOFSync(&pb);
    EXPECT_EQ(noErr, pb.ioParam.ioResult);
}

void eofCommon(short refNum)
{
        // earlier system versions deliver undefined data when a file is grown using SetEOF
        // in Mac OS X 10.4 + classic + MacOS 9, the test also passes without the following line:
    doWrite(refNum, 0, "\0\0\0\0\0\0\0\0", 8);

    setEOF(refNum, 4);
    EXPECT_TRUE(verifyContents(refNum, "\0\0\0\0", 4));

    setEOF(refNum, 8);
    EXPECT_TRUE(verifyContents(refNum, "\0\0\0\0\0\0\0\0", 8));

    setEOF(refNum, 4);
    EXPECT_TRUE(verifyContents(refNum, "\0\0\0\0", 4));

    doWrite(refNum, 6, "a", 1);
    EXPECT_TRUE(verifyContents(refNum, "\0\0\0\0\0\0a", 7));

    doWrite(refNum, 0, "Hello, world.", 13);
    EXPECT_TRUE(verifyContents(refNum, "Hello, world.", 13));

    setEOF(refNum, 5);
    EXPECT_TRUE(verifyContents(refNum, "Hello", 5));
}

TEST_F(FileTest, SetEOF)
{
    open();
    eofCommon(refNum);
    close();
}

TEST_F(FileTest, SetEOF_RF)
{
    openRF();
    eofCommon(refNum);
    close();
}

TEST_F(FileTest, SetEOF_BothForks)
{
    open();
    eofCommon(refNum);
    close();

    openRF();
    eofCommon(refNum);
    close();

    open();
    EXPECT_TRUE(verifyContents(refNum, "Hello", 5));
    doWrite(refNum, 0, "\0\0\0\0", 4);
    EXPECT_TRUE(verifyContents(refNum, "\0\0\0\0o", 5));

    eofCommon(refNum);
    close();
}


TEST(Files, CatMove)
{
    WDPBRec wdpb;
    memset(&wdpb, 42, sizeof(wdpb));
    wdpb.ioCompletion = nullptr;
    wdpb.ioVRefNum = 0;
    Str255 buf;
    wdpb.ioNamePtr = nullptr;
    wdpb.ioWDIndex = 0;

    PBGetWDInfoSync(&wdpb);
    EXPECT_EQ(noErr, wdpb.ioResult);

    short vRefNum = wdpb.ioWDVRefNum;
    long dirID = wdpb.ioWDDirID;


    HParamBlockRec pb;
    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = vRefNum;
    pb.fileParam.ioDirID = dirID;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBDirCreateSync(&pb);

    ASSERT_EQ(noErr, pb.ioParam.ioResult);
    long newDirID = pb.fileParam.ioDirID;
    
    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = vRefNum;
    pb.fileParam.ioDirID = dirID;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test";
    PBHCreateSync(&pb);
    EXPECT_EQ(noErr, pb.ioParam.ioResult);

    CMovePBRec mpb;
    
    memset(&mpb, 42, sizeof(mpb));
    mpb.ioCompletion = nullptr;
    mpb.ioVRefNum = vRefNum;
    mpb.ioDirID = dirID;
    mpb.ioNamePtr = (StringPtr)"\ptemp-test";
    mpb.ioNewName = nullptr;
    mpb.ioNewDirID = newDirID;
    PBCatMoveSync(&mpb);

    EXPECT_EQ(noErr, mpb.ioResult);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = vRefNum;
    pb.fileParam.ioDirID = dirID;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test";
    PBHDeleteSync(&pb);

    EXPECT_EQ(fnfErr, pb.ioParam.ioResult);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = vRefNum;
    pb.fileParam.ioDirID = newDirID;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test";
    PBHDeleteSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);


    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = vRefNum;
    pb.fileParam.ioDirID = dirID;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBHDeleteSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);
}

TEST_F(FileTest, GetFInfo)
{
    HParamBlockRec pb;
    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = vRefNum;
    pb.fileParam.ioDirID = dirID;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBDirCreateSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);
    long newDirID = pb.fileParam.ioDirID;
    
    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = vRefNum;
    pb.fileParam.ioDirID = dirID;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    pb.fileParam.ioFDirIndex = 0;
    PBHGetFInfoSync(&pb);
    EXPECT_EQ(fnfErr, pb.ioParam.ioResult);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = vRefNum;
    pb.fileParam.ioDirID = dirID;
    pb.ioParam.ioNamePtr = file1;
    pb.fileParam.ioFDirIndex = 0;
    PBHGetFInfoSync(&pb);
    EXPECT_EQ(noErr, pb.ioParam.ioResult);


    CInfoPBRec ipb;
    memset(&ipb, 42, sizeof(ipb));
    ipb.hFileInfo.ioCompletion = nullptr;
    ipb.hFileInfo.ioVRefNum = vRefNum;
    ipb.hFileInfo.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    ipb.hFileInfo.ioFDirIndex = 0;
    ipb.hFileInfo.ioDirID = dirID;

    PBGetCatInfoSync(&ipb);
    EXPECT_EQ(noErr, ipb.hFileInfo.ioResult);

    EXPECT_GE(ipb.dirInfo.ioDrCrDat, pb.fileParam.ioFlCrDat);
    EXPECT_GE(ipb.dirInfo.ioDrMdDat, pb.fileParam.ioFlMdDat);
    EXPECT_GE(pb.fileParam.ioFlCrDat, 0);
    EXPECT_GE(pb.fileParam.ioFlMdDat, 0);

    memset(&ipb, 42, sizeof(ipb));
    ipb.hFileInfo.ioCompletion = nullptr;
    ipb.hFileInfo.ioVRefNum = vRefNum;
    ipb.hFileInfo.ioNamePtr = file1;
    ipb.hFileInfo.ioFDirIndex = 0;
    ipb.hFileInfo.ioDirID = dirID;

    PBGetCatInfoSync(&ipb);
    EXPECT_EQ(noErr, ipb.hFileInfo.ioResult);

    int comparison = memcmp((void*)&pb.fileParam.ioResult, (void*)&ipb.hFileInfo.ioResult, (char*)&ipb.hFileInfo.ioFlBkDat - (char*)&ipb.hFileInfo.ioResult);
    EXPECT_EQ(0, comparison);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = vRefNum;
    pb.fileParam.ioDirID = dirID;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBHDeleteSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);

}


TEST_F(FileTest, GetFInfoIndexed)
{
    HParamBlockRec pb;
    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = vRefNum;
    pb.fileParam.ioDirID = dirID;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBDirCreateSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);
    long newDirID = pb.fileParam.ioDirID;
    

    std::set<std::string> filesCat;
    std::set<std::string> filesInfo;

    for(int i = 1; ; i++)
    {
        Str255 buffer;
        CInfoPBRec ipb;

        buffer[0] = 0;
        memset(&ipb, 42, sizeof(ipb));
        ipb.hFileInfo.ioCompletion = nullptr;
        ipb.hFileInfo.ioVRefNum = vRefNum;
        ipb.hFileInfo.ioNamePtr = buffer;
        ipb.hFileInfo.ioFDirIndex = i;
        ipb.hFileInfo.ioDirID = dirID;

        PBGetCatInfoSync(&ipb);
        if(ipb.hFileInfo.ioResult == fnfErr)
            break;

        EXPECT_EQ(noErr, ipb.hFileInfo.ioResult);
        EXPECT_NE(0, buffer[0]);

        if(ipb.hFileInfo.ioFlAttrib & kioFlAttribDirMask)
            ;
        else
            filesCat.emplace(buffer+1, buffer+1+buffer[0]);
    }
    for(int i = 1; ; i++)
    {
        Str255 buffer;
        buffer[0] = 0;
        memset(&pb, 42, sizeof(pb));
        pb.ioParam.ioCompletion = nullptr;
        pb.ioParam.ioVRefNum = vRefNum;
        pb.fileParam.ioDirID = dirID;
        pb.ioParam.ioNamePtr = buffer;
        pb.fileParam.ioFDirIndex = i;
        PBHGetFInfoSync(&pb);
        if(pb.ioParam.ioResult == fnfErr)
            break;

        EXPECT_EQ(noErr, pb.ioParam.ioResult);

        filesInfo.emplace(buffer+1, buffer+1+buffer[0]);
    }

    EXPECT_EQ(filesCat, filesInfo);

    memset(&pb, 42, sizeof(pb));
    pb.ioParam.ioCompletion = nullptr;
    pb.ioParam.ioVRefNum = vRefNum;
    pb.fileParam.ioDirID = dirID;
    pb.ioParam.ioNamePtr = (StringPtr)"\ptemp-test-dir";
    PBHDeleteSync(&pb);

    EXPECT_EQ(noErr, pb.ioParam.ioResult);

}


TEST(Files, DirID1)
{
    std::set<std::string> filesCat;
    CInfoPBRec ipb;
    Str255 rootName;
    Str255 firstItem;
    Str255 relPath;
    Str255 dir1Name;

    // get root directory name (== volume name)
    memset(&ipb, 42, sizeof(ipb));
    ipb.dirInfo.ioCompletion = nullptr;
    ipb.dirInfo.ioVRefNum = 0;
    ipb.dirInfo.ioNamePtr = rootName;
    ipb.dirInfo.ioFDirIndex = -1;
    ipb.dirInfo.ioDrDirID = 2;
    
    PBGetCatInfoSync(&ipb);
    EXPECT_EQ(noErr, ipb.hFileInfo.ioResult);

    // get the name of the first item of the root directory (doesn't matter what it is)
    memset(&ipb, 42, sizeof(ipb));
    ipb.dirInfo.ioCompletion = nullptr;
    ipb.dirInfo.ioVRefNum = 0;
    ipb.dirInfo.ioNamePtr = firstItem;
    ipb.dirInfo.ioFDirIndex = 1;
    ipb.dirInfo.ioDrDirID = 2;
    
    PBGetCatInfoSync(&ipb);
    EXPECT_EQ(noErr, ipb.hFileInfo.ioResult);

    /// --- 

    // make sure dirID==1 is not a "real" directory
    memset(&ipb, 42, sizeof(ipb));
    ipb.dirInfo.ioCompletion = nullptr;
    ipb.dirInfo.ioVRefNum = 0;
    ipb.dirInfo.ioNamePtr = dir1Name;
    ipb.dirInfo.ioFDirIndex = -1;
    ipb.dirInfo.ioDrDirID = 1;
    
    PBGetCatInfoSync(&ipb);
    EXPECT_EQ(fnfErr, ipb.hFileInfo.ioResult);  // we aren't allowed to get info for dir 1

    // access root directory by name
    memset(&ipb, 42, sizeof(ipb));
    ipb.hFileInfo.ioCompletion = nullptr;
    ipb.hFileInfo.ioVRefNum = 0;
    ipb.hFileInfo.ioNamePtr = rootName;
    ipb.hFileInfo.ioFDirIndex = 0;
    ipb.hFileInfo.ioDirID = 1;

    PBGetCatInfoSync(&ipb);
    EXPECT_EQ(noErr, ipb.hFileInfo.ioResult);
    EXPECT_EQ(2, ipb.hFileInfo.ioDirID);
    EXPECT_EQ(1, ipb.hFileInfo.ioFlParID);

    // access file in root directory using relative path
    relPath[0] = rootName[0] + firstItem[0] + 2;
    relPath[1] = ':';
    memcpy(relPath + 2, rootName + 1, rootName[0]);
    relPath[2 + rootName[0]] = ':';
    memcpy(relPath + 2 + rootName[0] + 1, firstItem + 1, firstItem[0]);

    memset(&ipb, 42, sizeof(ipb));
    ipb.hFileInfo.ioCompletion = nullptr;
    ipb.hFileInfo.ioVRefNum = 0;
    ipb.hFileInfo.ioNamePtr = relPath;
    ipb.hFileInfo.ioFDirIndex = 0;
    ipb.hFileInfo.ioDirID = 1;

    PBGetCatInfoSync(&ipb);
    EXPECT_EQ(noErr, ipb.hFileInfo.ioResult);
    EXPECT_EQ(2, ipb.hFileInfo.ioFlParID);

    // test that root directory is not a subdir of itself
    memset(&ipb, 42, sizeof(ipb));
    ipb.hFileInfo.ioCompletion = nullptr;
    ipb.hFileInfo.ioVRefNum = 0;
    ipb.hFileInfo.ioNamePtr = rootName;
    ipb.hFileInfo.ioFDirIndex = 0;
    ipb.hFileInfo.ioDirID = 2;

    PBGetCatInfoSync(&ipb);
    EXPECT_EQ(fnfErr, ipb.hFileInfo.ioResult);
    
    // ... and that the relative path doesn't work either
    memset(&ipb, 42, sizeof(ipb));
    ipb.hFileInfo.ioCompletion = nullptr;
    ipb.hFileInfo.ioVRefNum = 0;
    ipb.hFileInfo.ioNamePtr = relPath;
    ipb.hFileInfo.ioFDirIndex = 0;
    ipb.hFileInfo.ioDirID = 2;

    PBGetCatInfoSync(&ipb);
    EXPECT_EQ(dirNFErr, ipb.hFileInfo.ioResult);
}



// Question:
// does the "poor man's search path" look for the *file name* or the *relative pathname*?

    // just here to verify that ALlocate does what I think it does,
    // not what Executor originally thought it did.
TEST_F(FileTest, Allocate)
{
    open();
    long allocSz = 1234, eof = 42;
    OSErr err = Allocate(refNum, &allocSz);
    EXPECT_EQ(noErr, err);
    EXPECT_GE(allocSz, 1234);

    err = GetEOF(refNum, &eof);
    EXPECT_EQ(noErr, err);
    EXPECT_EQ(0, eof);

    close();

}