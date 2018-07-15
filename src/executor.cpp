/* Copyright 1992-1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

//#define DEBUG

#include "rsys/common.h"

#include "QuickDraw.h"
#include "CQuickDraw.h"
#include "ResourceMgr.h"
#include "SegmentLdr.h"
#include "MemoryMgr.h"
#include "StdFilePkg.h"
#include "EventMgr.h"
#include "VRetraceMgr.h"
#include "OSUtil.h"
#include "FontMgr.h"
#include "ScrapMgr.h"
#include "ToolboxUtil.h"
#include "FileMgr.h"
#include "ControlMgr.h"
#include "DeviceMgr.h"
#include "PrintMgr.h"
#include "CommTool.h"

#include "rsys/trapglue.h"
#include "rsys/cquick.h"
#include "rsys/file.h"
#include "rsys/prefs.h"
#include "rsys/segment.h"
#include "rsys/host.h"
#include "rsys/executor.h"
#include "rsys/hfs.h"
#include "rsys/vdriver.h"
#include "rsys/trapname.h"

#include "rsys/options.h"
#include "rsys/string.h"


using namespace Executor;


void Executor::SFSaveDisk_Update(INTEGER vrefnum, Str255 filename)
{
    ParamBlockRec pbr;
    Str255 save_name;

    str255assign(save_name, filename);
    pbr.volumeParam.ioNamePtr = RM((StringPtr)save_name);
    pbr.volumeParam.ioVolIndex = CWC(-1);
    pbr.volumeParam.ioVRefNum = CW(vrefnum);
    PBGetVInfo(&pbr, false);
    LM(SFSaveDisk) = CW(-CW(pbr.volumeParam.ioVRefNum));
}

void Executor::executor_main(void)
{
    char quickbytes[grafSize];
    LONGINT tmpA5;
    GUEST<INTEGER> mess, count_s;
    INTEGER count;
    INTEGER exevrefnum, toskip;
    AppFile thefile;
    Byte *p;
    WDPBRec wdpb;
    CInfoPBRec hpb;
    Str255 name;
    StringPtr fName;

    EM_A5 = US_TO_SYN68K(&tmpA5);
    LM(CurrentA5) = guest_cast<Ptr>(CL(EM_A5));
    InitGraf((Ptr)quickbytes + sizeof(quickbytes) - 4);
    InitFonts();
    InitCRM();
    FlushEvents(everyEvent, 0);
    InitWindows();
    TEInit();
    InitDialogs((ProcPtr)0);
    InitCursor();

    /* ROMlib_WriteWhen(WriteInOSEvent); */

    LM(FinderName)[0] = MIN(strlen(BROWSER_NAME), sizeof(LM(FinderName)) - 1);
    memcpy(LM(FinderName) + 1, BROWSER_NAME, LM(FinderName)[0]);

    CountAppFiles(&mess, &count_s);
    count = CW(count_s);
    if(count > 0)
        GetAppFiles(1, &thefile);
    else
        thefile.fType = 0;

    if(thefile.fType == CLC(FOURCC('A', 'P', 'P', 'L')))
    {
        ClrAppFiles(1);
        Munger(MR(LM(AppParmHandle)), 2L * sizeof(INTEGER), (Ptr)0,
               (LONGINT)sizeof(AppFile), (Ptr) "", 0L);
    }
    hpb.hFileInfo.ioNamePtr = RM(&thefile.fName[0]);
    hpb.hFileInfo.ioVRefNum = thefile.vRefNum;
    hpb.hFileInfo.ioFDirIndex = CWC(0);
    hpb.hFileInfo.ioDirID = CLC(0);
    PBGetCatInfo(&hpb, false);

    if(thefile.fType == CLC(FOURCC('A', 'P', 'P', 'L')))
        fName = thefile.fName;
    else
        ExitToShell();

    for(p = fName + fName[0] + 1;
        p > fName && *--p != ':';)
        ;
    toskip = p - fName;
    LM(CurApName)[0] = MIN(fName[0] - toskip, 31);
    BlockMoveData((Ptr)fName + 1 + toskip, (Ptr)LM(CurApName) + 1,
                  (Size)LM(CurApName)[0]);

    wdpb.ioVRefNum = hpb.hFileInfo.ioVRefNum;
    wdpb.ioWDDirID = hpb.hFileInfo.ioFlParID;
    SFSaveDisk_Update(CW(hpb.hFileInfo.ioVRefNum), fName);
    LM(CurDirStore) = hpb.hFileInfo.ioFlParID;
    wdpb.ioWDProcID = CLC(FOURCC('X', 'c', 't', 'r'));
    wdpb.ioNamePtr = 0;
    PBOpenWD(&wdpb, false);
    exevrefnum = CW(wdpb.ioVRefNum);
    Launch(LM(CurApName), exevrefnum);
}
