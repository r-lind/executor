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
#include "SoundDvr.h"
#include "PrintMgr.h"
#include "StartMgr.h"
#include "CommTool.h"

#include "rsys/trapglue.h"
#include "rsys/cquick.h"
#include "rsys/file.h"
#include "rsys/soundopts.h"
#include "rsys/prefs.h"
#include "rsys/segment.h"
#include "rsys/host.h"
#include "rsys/executor.h"
#include "rsys/hfs.h"
#include "rsys/float.h"
#include "rsys/vdriver.h"
#include "rsys/trapname.h"

#include "rsys/options.h"
#include "rsys/suffix_maps.h"
#include "rsys/string.h"

#include "rsys/emustubs.h"

using namespace Executor;


void Executor::executor_main(void)
{
    char quickbytes[grafSize];
    LONGINT tmpA5;
    GUEST<INTEGER> mess, count_s;
    INTEGER count;
    INTEGER exevrefnum, toskip;
    AppFile thefile;
    Byte *p;
    int i;
    WDPBRec wdpb;
    CInfoPBRec hpb;
    Str255 name;
    StringPtr fName;

    LM(SCSIFlags) = CWC(0xEC00); /* scsi+clock+xparam+mmu+adb
				 (no fpu,aux or pwrmgr) */

    LM(MCLKPCmiss1) = 0; /* &LM(MCLKPCmiss1) = 0x358 + 72 (MacLinkPC starts
			   adding the 72 byte offset to VCB pointers too
			   soon, starting with 0x358, which is not the
			   address of a VCB) */

    LM(MCLKPCmiss2) = 0; /* &LM(MCLKPCmiss1) = 0x358 + 78 (MacLinkPC misses) */
    LM(AuxCtlHead) = 0;
    LM(CurDeactive) = 0;
    LM(CurActivate) = 0;
    LM(macfpstate)[0] = 0;
    LM(fondid) = 0;
    LM(PrintErr) = 0;
    LM(mouseoffset) = 0;
    LM(heapcheck) = 0;
    LM(DefltStack) = CLC(0x2000); /* nobody really cares about these two */
    LM(MinStack) = CLC(0x400); /* values ... */
    LM(IAZNotify) = 0;
    LM(CurPitch) = 0;
    LM(JSwapFont) = RM((ProcPtr)&FMSwapFont);
    LM(JInitCrsr) = RM((ProcPtr)&InitCursor);

    LM(Key1Trans) = RM((Ptr)&stub_Key1Trans);
    LM(Key2Trans) = RM((Ptr)&stub_Key2Trans);
    LM(JFLUSH) = RM(&FlushCodeCache);
    LM(JResUnknown1) = LM(JFLUSH); /* I don't know what these are supposed to */
    LM(JResUnknown2) = LM(JFLUSH); /* do, but they're not called enough for
				   us to worry about the cache flushing
				   overhead */

    //LM(CPUFlag) = 2; /* mc68020 */
    LM(CPUFlag) = 4; /* mc68040 */
    LM(UnitNtryCnt) = 0; /* how many units in the table */

    LM(TheZone) = LM(SysZone);
    LM(VIA) = RM(NewPtr(16 * 512)); /* IMIII-43 */
    memset(MR(LM(VIA)), 0, (LONGINT)16 * 512);
    *(char *)MR(LM(VIA)) = 0x80; /* Sound Off */

#define SCC_SIZE 1024

    LM(SCCRd) = RM(NewPtrSysClear(SCC_SIZE));
    LM(SCCWr) = RM(NewPtrSysClear(SCC_SIZE));

    LM(SoundBase) = RM(NewPtr(370 * sizeof(INTEGER)));
#if 0
    memset(CL(LM(SoundBase)), 0, (LONGINT) 370 * sizeof(INTEGER));
#else /* !0 */
    for(i = 0; i < 370; ++i)
        ((GUEST<INTEGER> *)MR(LM(SoundBase)))[i] = CWC(0x8000); /* reference 0 sound */
#endif /* !0 */
    LM(TheZone) = LM(ApplZone);
    LM(HiliteMode) = CB(0xFF);
    /* Mac II has 0x3FFF here */
    LM(ROM85) = CWC(0x3FFF);

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

    LM(loadtrap) = 0;

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
    {
        const char *p = NULL;

        if(count > 0)
        {
            p = ROMlib_find_best_creator_type_match(CL(hpb.hFileInfo.ioFlFndrInfo.fdCreator),
                                                    CL(hpb.hFileInfo.ioFlFndrInfo.fdType));
        }
        fName = name;
        if(!p)
            ExitToShell();
        else
        {
            ROMlib_exit = true;
            str255_from_c_string(fName, p);
            hpb.hFileInfo.ioNamePtr = RM(fName);
            hpb.hFileInfo.ioVRefNum = CWC(0);
            hpb.hFileInfo.ioFDirIndex = CWC(0);
            hpb.hFileInfo.ioDirID = CLC(0);
            PBGetCatInfo(&hpb, false);

            {
                HParamBlockRec hp;
                Str255 fName2;

                memset(&hp, 0, sizeof hp);
                str255assign(fName2, fName);
                hp.ioParam.ioNamePtr = RM((StringPtr)fName2);
                hp.volumeParam.ioVolIndex = CWC(-1);
                PBHGetVInfo(&hp, false);
                hpb.hFileInfo.ioVRefNum = hp.ioParam.ioVRefNum;
            }
        }
    }

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
