/* Copyright 1986-1997 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in FileMgr.h (DO NOT DELETE THIS LINE) */

#include "rsys/common.h"
#include "QuickDraw.h"
#include "FileMgr.h"
#include "OSEvent.h"
#include "VRetraceMgr.h"
#include "OSUtil.h"
#include "MemoryMgr.h"
#include "StdFilePkg.h"

#include "rsys/hfs.h"
#include "rsys/file.h"
#include "rsys/futzwithdosdisks.h"
#include "rsys/ini.h"
#include "rsys/string.h"
#include "rsys/segment.h"
#include "rsys/paths.h"

#if !defined(WIN32)
#include <pwd.h>
#else
#include "winfs.h"
//#include "dosdisk.h"
#endif

#include <ctype.h>
#include <algorithm>

#include <cmrc/cmrc.hpp>

CMRC_DECLARE(resources);

using namespace Executor;

namespace Executor
{
int ROMlib_nosync = 0; /* if non-zero, we don't call sync () or fsync () */
char ROMlib_startdir[MAXPATHLEN];
#if defined(WIN32)
char ROMlib_start_drive;
#endif
std::string ROMlib_appname;
}


#if !defined(NDEBUG)
void Executor::fs_err_hook(OSErr err)
{
}
#endif

int ROMlib_lasterrnomapped;

#define MAX_ERRNO 50

#define install_errno(uerr, merr)         \
    do                                    \
    {                                     \
        gui_assert(uerr < NELEM(xtable)); \
        xtable[uerr] = merr;              \
    } while(false);

OSErr Executor::ROMlib_maperrno() /* INTERNAL */
{
    OSErr retval;
    static OSErr xtable[MAX_ERRNO + 1];
    static char been_here = false;
    int errno_save;

    if(!been_here)
    {
        int i;

        for(i = 0; i < (int)NELEM(xtable); ++i)
            xtable[i] = fsDSIntErr;

        install_errno(0, noErr);
        install_errno(EPERM, permErr);
        install_errno(ENOENT, fnfErr);
        install_errno(EIO, ioErr);
        install_errno(ENXIO, paramErr);
        install_errno(EBADF, fnOpnErr);
        install_errno(EAGAIN, fLckdErr);
        install_errno(ENOMEM, memFullErr);
        install_errno(EACCES, permErr);
        install_errno(EFAULT, paramErr);
        install_errno(EBUSY, fBsyErr);
        install_errno(EEXIST, dupFNErr);
        install_errno(EXDEV, fsRnErr);
        install_errno(ENODEV, nsvErr);
        install_errno(ENOTDIR, dirNFErr);
        install_errno(EINVAL, paramErr);
        install_errno(ENFILE, tmfoErr);
        install_errno(EMFILE, tmfoErr);
        install_errno(EFBIG, dskFulErr);
        install_errno(ENOSPC, dskFulErr);
        install_errno(ESPIPE, posErr);
        install_errno(EROFS, wPrErr);
        install_errno(EMLINK, dirFulErr);
#if !defined(WIN32)
        install_errno(ETXTBSY, fBsyErr);
        install_errno(EWOULDBLOCK, permErr);
#endif

        been_here = true;
    }

    errno_save = errno;
    ROMlib_lasterrnomapped = errno_save;

    if(errno_save < 0 || errno_save >= (int)NELEM(xtable))
        retval = fsDSIntErr;
    else
        retval = xtable[errno_save];

    if(retval == fsDSIntErr)
        warning_unexpected("fsDSIntErr errno = %d", errno_save);

    if(retval == dirNFErr)
        warning_trace_info("dirNFErr errno = %d", errno_save);

    fs_err_hook(retval);
    return retval;
}


Byte
Executor::open_attrib_bits(LONGINT file_id, VCB *vcbp, GUEST<INTEGER> *refnump)
{
    Byte retval;
    int i;

    retval = 0;
    *refnump = 0;
    for(i = 0; i < NFCB; i++)
    {
        if(ROMlib_fcblocks[i].fdfnum == file_id
           && ROMlib_fcblocks[i].fcvptr == vcbp)
        {
            if(*refnump == 0)
                *refnump = i * 94 + 2;
            if(ROMlib_fcblocks[i].fcflags & fcfisres)
                retval |= ATTRIB_RESOPEN;
            else
                retval |= ATTRIB_DATAOPEN;
        }
    }
    if(retval & (ATTRIB_RESOPEN | ATTRIB_DATAOPEN))
        retval |= ATTRIB_ISOPEN;
    return retval;
}

/* NOTE:  calling most of the routines here is a sign that the user may
	  be depending on the internal layout of things a bit too much */

void Executor::C_FInitQueue() /* IMIV-128 */
{
}

QHdrPtr Executor::C_GetFSQHdr() /* IMIV-175 */
{
    return (&LM(FSQHdr)); /* in UNIX domain, everything is synchronous */
}

QHdrPtr Executor::C_GetVCBQHdr() /* IMIV-178 */
{
    return (&LM(VCBQHdr));
}

QHdrPtr Executor::C_GetDrvQHdr() /* IMIV-182 */
{
    return (&LM(DrvQHdr));
}

/*
 * NOTE:  This is *the* PCBGetFCBInfo that we use whether
 *	  we're looking at hfs or ufs stuff.
 */

OSErr Executor::PBGetFCBInfo(FCBPBPtr pb, BOOLEAN async)
{
    filecontrolblock *fcbp, *efcbp;
    INTEGER i;

    if((i = pb->ioFCBIndx) > 0)
    {
        fcbp = (filecontrolblock *)(LM(FCBSPtr) + sizeof(INTEGER));
        efcbp = (filecontrolblock *)(LM(FCBSPtr) + *(GUEST<INTEGER> *)LM(FCBSPtr));
        if(pb->ioVRefNum < 0)
        {
            for(; fcbp != efcbp; fcbp++)
                if(fcbp->fcbFlNum && fcbp->fcbVPtr->vcbVRefNum == pb->ioVRefNum && --i <= 0)
                    break;
        }
        else if(pb->ioVRefNum == 0)
        {
            for(; fcbp != efcbp && (fcbp->fcbFlNum == 0 || --i > 0); fcbp++)
                ;
        }
        else /* if (pb->ioVRefNum > 0 */
        {
            for(; fcbp != efcbp; fcbp++)
                if(fcbp->fcbFlNum && fcbp->fcbVPtr->vcbDrvNum == pb->ioVRefNum && --i <= 0)
                    break;
        }
        if(fcbp == efcbp)
            PBRETURN(pb, fnOpnErr);
        pb->ioRefNum = (char *)fcbp - (char *)LM(FCBSPtr);
    }
    else
    {
        fcbp = ROMlib_refnumtofcbp(pb->ioRefNum);
        if(!fcbp)
            PBRETURN(pb, rfNumErr);
    }
    if(pb->ioNamePtr)
        str255assign(pb->ioNamePtr, fcbp->fcbCName);
    pb->ioFCBFlNm = fcbp->fcbFlNum;
    pb->ioFCBFlags = (fcbp->fcbMdRByt << 8) | (unsigned char)fcbp->fcbTypByt;
    pb->ioFCBStBlk = fcbp->fcbSBlk;
    pb->ioFCBEOF = fcbp->fcbEOF;
    pb->ioFCBCrPs = fcbp->fcbCrPs; 
    pb->ioFCBPLen = fcbp->fcbPLen;
    pb->ioFCBVRefNum = fcbp->fcbVPtr->vcbVRefNum;
    if(pb->ioFCBIndx <= 0 || pb->ioVRefNum == 0)
        pb->ioVRefNum = pb->ioFCBVRefNum;
    pb->ioFCBClpSiz = fcbp->fcbClmpSize;
    pb->ioFCBParID = fcbp->fcbDirID;
    PBRETURN(pb, noErr);
}

static bool
charcmp(char c1, char c2)
{
    bool retval;

    if(c1 == c2)
        retval = true;
    else if(c1 == '/')
        retval = c2 == '\\';
    else if(c1 == '\\')
        retval = c2 == '/';
    else
        retval = tolower(c1) == tolower(c2);
    return retval;
}

static int
slashstrcmp(const char *p1, const char *p2)
{
    int retval;

    retval = 0;

    while(*p1 || *p2)
    {
        if(!charcmp(*p1, *p2))
        {
            retval = -1;
            break;
        }
        ++p1;
        ++p2;
    }
    return retval;
}

static INTEGER ROMlib_driveno = 3;
static INTEGER ROMlib_ejdriveno = 2;

/*
 * NOTE: The way we handle drive information is pretty messed up right now.
 * In general the correct information is in the VCBExtra; we only recently
 * began putting it in the DriveExtra and right now we only use the info
 * in the DriveExtra to allow us to format floppies -- no other formatting
 * is currently permitted.  The problem is there's no easy way to map drive
 * characteristics from the non-Mac host into Mac type information unless
 * we can pull the information out of the Mac filesystem.
 */

DrvQExtra *
Executor::ROMlib_addtodq(ULONGINT drvsize, const char *devicename, INTEGER partition,
                         INTEGER drefnum, drive_flags_t flags, hfs_access_t *hfsp)
{
    INTEGER dno;
    DrvQExtra *dqp;
    DrvQEl *dp;
    int strl;
    GUEST<THz> saveZone;
    static bool seen_floppy = false;

    saveZone = LM(TheZone);
    LM(TheZone) = LM(SysZone);
#if !defined(LETGCCWAIL)
    dqp = (DrvQExtra *)0;
#endif
    dno = 0;
    for(dp = (DrvQEl *)LM(DrvQHdr).qHead; dp; dp = (DrvQEl *)dp->qLink)
    {
        dqp = (DrvQExtra *)((char *)dp - sizeof(LONGINT));
        if(dqp->partition == partition && slashstrcmp((char *)dqp->devicename, devicename) == 0)
        {
            dno = dqp->dq.dQDrive;
            /*-->*/ break;
        }
    }
    if(!dno)
    {
        if((flags & DRIVE_FLAGS_FLOPPY) && !seen_floppy)
        {
            dno = 1;
            seen_floppy = true;
        }
        else
        {
            if((flags & DRIVE_FLAGS_FIXED) || ROMlib_ejdriveno == 3)
                dno = ROMlib_driveno++;
            else
                dno = ROMlib_ejdriveno++;
        }
        dqp = (DrvQExtra *)NewPtr(sizeof(DrvQExtra));
        dqp->flags = 1 << 7; /* is not single sided */
        if(flags & DRIVE_FLAGS_LOCKED)
            dqp->flags = dqp->flags | 1L << 31;
        if(flags & DRIVE_FLAGS_FIXED)
            dqp->flags = dqp->flags | 8L << 16;
        else
            dqp->flags = dqp->flags | 2; /* IMIV-181 says
							   it can be 1 or 2 */

        /*	dqp->dq.qLink will be set up when we Enqueue this baby */
        dqp->dq.dQDrvSz = drvsize;
        dqp->dq.dQDrvSz2 = drvsize >> 16;
        dqp->dq.qType = 1;
        dqp->dq.dQDrive = dno;
        dqp->dq.dQRefNum = drefnum;
        dqp->dq.dQFSID = 0;
        if(!devicename)
            dqp->devicename = 0;
        else
        {
            strl = strlen(devicename);
            dqp->devicename = NewPtr(strl + 1);
            strcpy((char *)dqp->devicename, devicename);
        }
        dqp->partition = partition;
        if(hfsp)
            dqp->hfs = *hfsp;
        else
        {
            memset(&dqp->hfs, 0, sizeof(dqp->hfs));
            dqp->hfs.fd = -1;
        }
        Enqueue((QElemPtr)&dqp->dq, &LM(DrvQHdr));
    }
    LM(TheZone) = saveZone;
    return dqp;
}

std::string
Executor::expandPath(std::string name)
{
    if(name.empty())
        return "";

    switch(name[0])
    {
        case '+':
            name = ROMlib_startdir + name.substr(1);
            break;
        case '~':
        {
            auto home = getenv("HOME");
            if(home)
            {
                name = home + name.substr(1);
                break;
            }
        }
        break;
    }

#if defined(WIN32)
    std::replace(name.begin(), name.end(), '/', '\\');
#endif

    return name;
}

StringPtr Executor::ROMlib_exefname;

std::string Executor::ROMlib_ConfigurationFolder;
static std::string ROMlib_SystemFolder;
fs::path Executor::ROMlib_DirectoryMap;
static std::string ROMlib_MacVolumes;
std::string Executor::ROMlib_ScreenDumpFile;
static std::string ROMlib_OffsetFile;

LONGINT Executor::ROMlib_magic_offset = -1;

static void
skip_comments(FILE *fp)
{
    int c;

    while((c = getc(fp)) == '#')
    {
        while(c != '\n')
            c = getc(fp);
    }
    ungetc(c, fp);
}

static void
parse_offset_file(void)
{
    FILE *fp;

    fp = Ufopen(ROMlib_OffsetFile.c_str(), "r");
    if(!fp)
    {
#if 0
      warning_unexpected ("Couldn't open \"%s\"", ROMlib_OffsetFile);
#endif
    }
    else
    {
        int n_found;

        skip_comments(fp);
        n_found = fscanf(fp, "0x%08x", &ROMlib_magic_offset);
        if(n_found != 1)
            warning_unexpected("n_found = %d", n_found);
        fclose(fp);
    }
}

static bool
is_unix_path(const char *pathname)
{
    bool retval;

#if defined(MSDOS) || defined(CYGWIN32)
    if(pathname[0] && pathname[1] == ':' && (pathname[2] == '/' || pathname[2] == '\\'))
        pathname += 3;
#endif
    retval = strchr(pathname, ':') == 0;
    return retval;
}

static void MountMacVolumes(std::string macVolumes)
{
    GUEST<LONGINT> m;
    char *p, *ep;
    struct stat sbuf;

    m = 0;
    p = (char *)alloca(macVolumes.size() + 1);
    strcpy(p, macVolumes.c_str());
    while(p && *p)
    {
        ep = strchr(p, ';');
        if(ep)
            *ep = 0;
        std::string newp = expandPath(p);
        if(Ustat(newp.c_str(), &sbuf) == 0)
        {
            if(!S_ISDIR(sbuf.st_mode))
                ROMlib_openharddisk(newp.c_str(), &m);
            else
            {
                DIR *dirp;

                dirp = Uopendir(newp.c_str());
                if(dirp)
                {
#if defined(USE_STRUCT_DIRECT)
                    struct direct *direntp;
#else
                    struct dirent *direntp;
#endif

                    while((direntp = readdir(dirp)))
                    {
                        int namelen;

                        namelen = strlen(direntp->d_name);
                        if(namelen >= 4 && (strcasecmp(direntp->d_name + namelen - 4, ".hfv")
                                                == 0
                                            || strcasecmp(direntp->d_name + namelen - 4, ".ima")
                                                == 0))
                        {
                            ROMlib_openharddisk((newp + "/" + direntp->d_name).c_str(), &m);
                        }
                    }
                    closedir(dirp);
                }
            }
        }
        if(ep)
        {
            *ep = ';';
            p = ep + 1;
        }
        else
            p = 0;
    }

    futzwithdosdisks();
}

void Executor::InitSystemFolder(std::string systemFolder)
{
    CInfoPBRec cpb;
    WDPBRec wpb;

    if(is_unix_path(systemFolder.c_str()))
    {
        if(auto sysSpec = nativePathToFSSpec(fs::path(systemFolder) / "System")) // FIXME: SYSMACNAME
        {
            cpb.hFileInfo.ioNamePtr = (StringPtr)SYSMACNAME;
            cpb.hFileInfo.ioVRefNum = sysSpec->vRefNum;
            cpb.hFileInfo.ioDirID = sysSpec->parID;
        }
        else
        {
            fprintf(stderr, "Couldn't find '%s'\n", systemFolder.c_str());
            exit(1);
        }
    }
    else
    {
        int sysnamelen = 1 + systemFolder.size() + 1 + strlen(SYSMACNAME + 1) + 1;
        char *sysname = (char *)alloca(sysnamelen);
        *sysname = sysnamelen - 2; /* don't count first byte or nul */
        sprintf(sysname + 1, "%s:%s", systemFolder.c_str(), SYSMACNAME + 1);
        cpb.hFileInfo.ioNamePtr = (StringPtr)sysname;
        cpb.hFileInfo.ioVRefNum = 0;
        cpb.hFileInfo.ioDirID = 0;
    }
    cpb.hFileInfo.ioFDirIndex = 0;
    if(PBGetCatInfo(&cpb, false) == noErr)
    {
        wpb.ioNamePtr = 0;
        wpb.ioVRefNum = cpb.hFileInfo.ioVRefNum;
        wpb.ioWDProcID = TICK("unix");
        wpb.ioWDDirID = cpb.hFileInfo.ioFlParID;
        if(PBOpenWD(&wpb, false) == noErr)
            LM(BootDrive) = wpb.ioVRefNum;
    }
    else
    {
        fprintf(stderr, "Couldn't open System: '%s'\n", ROMlib_SystemFolder.c_str());
        exit(1);
    }
}

void Executor::InitPaths()
{
    auto initpath = [](const char *varname, const char *defval) {
        if(auto v = getenv(varname))
            return expandPath(v);
        else
            return expandPath(defval);
    };

    ROMlib_ConfigurationFolder = initpath("Configuration", "~/.executor/Configuration");
    ROMlib_SystemFolder = initpath("SystemFolder", "~/.executor/System Folder");
    ROMlib_DirectoryMap = initpath("ExecutorDirectoryMap", "~/.executor/cnidmap");
    ROMlib_MacVolumes = initpath("MacVolumes", "~/.executor/images");
    ROMlib_ScreenDumpFile = initpath("ScreenDumpFile", "/tmp/excscrn*.tif");
    ROMlib_OffsetFile = initpath("OffsetFile", "~/.executor/offset_file");
    ROMlib_PrintersIni = initpath("PrintersIni", "~/.executor/printers.ini");
    ROMlib_PrintDef = initpath("PrintDef", "~/.executor/printdef.ini");

#if !defined(LITTLEENDIAN)
    ROMlib_DirectoryMap += "-be";
#endif /* !defined(LITTLEENDIAN) */

    auto efs = cmrc::resources::get_filesystem();
    fs::path systemFolder = ROMlib_SystemFolder;

    auto copyfile = [&](fs::path dest, auto from) {
        fs::ofstream out(dest);
        std::ostreambuf_iterator<char> begin_dest(out);
        std::copy(from.begin(), from.end(), begin_dest);
    };

    auto ensureFile = [&](fs::path dest) {
        if(!fs::exists(dest))
        {
            copyfile(dest, efs.open(dest.filename().string()));
            if(efs.is_file(dest.filename().string() + ".ad"))
                copyfile(dest.parent_path() / ("%" + dest.filename().string()),
                            efs.open(dest.filename().string() + ".ad"));
        }
    };

    fs::create_directories(systemFolder);
    fs::create_directories(systemFolder / "Preferences");
    ensureFile(systemFolder / "System");
    ensureFile(systemFolder / "Browser");
    ensureFile(systemFolder / "Printer");
    ensureFile(systemFolder / "godata.sav");
    ensureFile(ROMlib_PrintDef);
    ensureFile(ROMlib_PrintersIni);
    fs::create_directories(ROMlib_ConfigurationFolder);
}

void Executor::ROMlib_fileinit() /* INTERNAL */
{
    GUEST<THz> savezone;

    LM(CurDirStore) = 2;
    memset(&LM(DrvQHdr), 0, sizeof(LM(DrvQHdr)));
    memset(&LM(VCBQHdr), 0, sizeof(LM(VCBQHdr)));
    memset(&LM(FSQHdr), 0, sizeof(LM(FSQHdr)));
    LM(DefVCBPtr) = 0;
    LM(FSFCBLen) = 94;

    savezone = LM(TheZone);
    LM(TheZone) = LM(SysZone);
    LM(FCBSPtr) = NewPtr((Size)sizeof(fcbhidden));
    ((fcbhidden *)LM(FCBSPtr))->nbytes = sizeof(fcbhidden);

    for(int i = 0; i < NFCB; i++)
    {
        ROMlib_fcblocks[i].fdfnum = 0;
        ROMlib_fcblocks[i].fcleof = i + 1;
        ROMlib_fcblocks[i].fcbTypByt = 0;
        ROMlib_fcblocks[i].fcbSBlk = 0;
        ROMlib_fcblocks[i].fcPLen = 0;
        ROMlib_fcblocks[i].fcbCrPs = 0;
        ROMlib_fcblocks[i].fcbBfAdr = 0;
        ROMlib_fcblocks[i].fcbFlPos = 0;
        ROMlib_fcblocks[i].fcbClmpSize = 1;
        ROMlib_fcblocks[i].fcbFType = 0;
        ROMlib_fcblocks[i].zero[0] = 0;
        ROMlib_fcblocks[i].zero[1] = 0;
        ROMlib_fcblocks[i].zero[2] = 0;
        ROMlib_fcblocks[i].fcname[0] = 0;
    }
    ROMlib_fcblocks[NFCB - 1].fcleof = -1;

#define NWDENTRIES 40
    INTEGER wdlen = NWDENTRIES * sizeof(wdentry) + sizeof(INTEGER);
    LM(WDCBsPtr) = NewPtr((Size)wdlen);
    LM(TheZone) = savezone;
    memset(LM(WDCBsPtr), 0, wdlen);
    *(GUEST<INTEGER> *)LM(WDCBsPtr) = wdlen;

    InitPaths();

    parse_offset_file();
    ROMlib_hfsinit();
    MountLocalVolume();

    MountMacVolumes(ROMlib_MacVolumes);

    InitSystemFolder(ROMlib_SystemFolder);
}

fcbrec *
Executor::PRNTOFPERR(INTEGER prn, OSErr *errp)
{
    fcbrec *retval;
    OSErr err;

    if(prn < 0 || prn >= *(GUEST<INTEGER> *)LM(FCBSPtr) || (prn % 94) != 2)
    {
        retval = 0;
        err = rfNumErr;
    }
    else
    {
        retval = (fcbrec *)((char *)LM(FCBSPtr) + prn);
        if(!retval->fdfnum)
        {
            retval = 0;
            err = rfNumErr;
        }
        else
            err = noErr;
    }
    *errp = err;
    return retval;
}
