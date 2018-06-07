/* Copyright 1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* NOTE:  This is the quick hack version that uses old-style calls.
          Eventually we should extract the common functionality and
	  then use that for both.  That will be a little cleaner, a little
	  faster and a little easier to debug, but not enough of any to
	  make it sensible to do that initially.  */

#include "rsys/common.h"

#include "MemoryMgr.h"
#include "FileMgr.h"
#include "ProcessMgr.h"

#include "rsys/file.h"
#include "rsys/string.h"
#include "rsys/hfs.h"
#include "rsys/executor.h"

using namespace Executor;

/*
 * extract the last component of a name:
 *
 * aaa:bbb:ccc  -> ccc
 * aaa:bbb:ccc: -> ccc
 */

static void
extract_name(Str255 dest, StringPtr source)
{
    int len, new_len;
    Byte *p;

    len = source[0];
    if(source[len] == ':') /* ignore trailing ':' */
        --len;
    for(p = source + len; p >= source + 1 && *p != ':'; --p)
        ;
    new_len = len - (p - source);
    dest[0] = new_len;
    memmove(dest + 1, p + 1, new_len);
}

OSErr Executor::C_FSMakeFSSpec(int16_t vRefNum, int32_t dir_id,
                               Str255 file_name, FSSpecPtr spec)
{
    Str255 local_file_name;
    OSErr retval;
    HParamBlockRec hpb;

    if(!file_name)
        warning_unexpected("!file_name");

    /*
 * Colons make things tricky because they are used both to identify volumes
 * and to delimit directories and sometimes to identify partial paths.  Right
 * now, most of these uses of colon shouldn't cause problems, but there are
 * some issues that need to be checked on a real Mac.  What do all these
 * mean?
 *
 *     dir_id      file_name
 *     310         foo:
 *     310         :foo:
 *     310         foo:bar
 *     310         :foo:bar
 *     310         :foo:bar:
 *
 * what about all those file_names, but with dir_id = 0, dir_id = 1, dir_id = 2
 * and dir_id = a number that isn't a directory id?
 */

    if(pstr_index_after(file_name, ':', 0))
        warning_unexpected("colon found");

    str255assign(local_file_name, file_name);
    hpb.volumeParam.ioNamePtr = RM((StringPtr)local_file_name);
    hpb.volumeParam.ioVRefNum = CW(vRefNum);
    if(file_name[0])
        hpb.volumeParam.ioVolIndex = CWC(-1);
    else
        hpb.volumeParam.ioVolIndex = CWC(0);

    retval = PBHGetVInfo(&hpb, false);

    if(retval == noErr)
    {
        CInfoPBRec cpb;

        str255assign(local_file_name, file_name);
        cpb.hFileInfo.ioNamePtr = RM((StringPtr)local_file_name);
        cpb.hFileInfo.ioVRefNum = CW(vRefNum);
        if(file_name[0])
            cpb.hFileInfo.ioFDirIndex = CWC(0);
        else
            cpb.hFileInfo.ioFDirIndex = CWC(-1);
        cpb.hFileInfo.ioDirID = CL(dir_id);
        retval = PBGetCatInfo(&cpb, false);
        if(retval == noErr)
        {
            spec->vRefNum = hpb.volumeParam.ioVRefNum;
            spec->parID = cpb.hFileInfo.ioFlParID;
            extract_name((StringPtr)spec->name, MR(cpb.hFileInfo.ioNamePtr));
        }
        else if(retval == fnfErr)
        {
            OSErr err;

            cpb.hFileInfo.ioNamePtr = nullptr;
            cpb.hFileInfo.ioVRefNum = CW(vRefNum);
            cpb.hFileInfo.ioFDirIndex = CWC(-1);
            cpb.hFileInfo.ioDirID = CL(dir_id);
            err = PBGetCatInfo(&cpb, false);
            if(err == noErr)
            {
                if(cpb.hFileInfo.ioFlAttrib & ATTRIB_ISADIR)
                {
                    spec->vRefNum = hpb.volumeParam.ioVRefNum;
                    spec->parID = CL(dir_id);
                    extract_name((StringPtr)spec->name, file_name);
                }
                else
                    retval = dupFNErr;
            }
        }
    }
    return retval;
}

/*
 * Part of ugly PAUP-specific hack below
 */

static void
create_temp_name(Str63 name, int i)
{
    OSErr err;
    ProcessSerialNumber psn;

    err = GetCurrentProcess(&psn);
    if(err == noErr)
        sprintf((char *)name + 1,
                "%x%x.%x", CL(psn.highLongOfPSN), CL(psn.lowLongOfPSN), i);
    else
        sprintf((char *)name + 1, "%d.%x", err, i);
    name[0] = strlen((char *)name + 1);
}

/*
 * On real HFS volumes we could do some B-tree manipulation to achieve
 * the correct results, *but* we'd need this code for ufs volumes anyway,
 * so for now we do the same thing on both.
 */

OSErr Executor::C_FSpExchangeFiles(FSSpecPtr src, FSSpecPtr dst)
{
    OSErr retval;

    warning_unimplemented("poorly implemented");
    if(src->vRefNum != dst->vRefNum)
        retval = diffVolErr;
    else if(ROMlib_creator != TICK("PAUP") || src->parID != dst->parID)
        retval = wrgVolTypeErr;
    else
    {
        /* Evil hack to get PAUP to work -- doesn't bother adjusting FCBs */
        FSSpec tmp_spec;
        int i;

        i = 0;
        tmp_spec = *dst;
        do
        {
            create_temp_name(tmp_spec.name, i++);
            retval = FSpRename(dst, tmp_spec.name);
        } while(retval == dupFNErr);
        if(retval == noErr)
        {
            retval = FSpRename(src, dst->name);
            if(retval != noErr)
                FSpRename(&tmp_spec, dst->name);
            else
            {
                retval = FSpRename(&tmp_spec, src->name);
                if(retval != noErr)
                {
                    FSpRename(dst, src->name);
                    FSpRename(&tmp_spec, dst->name);
                }
            }
        }
    }
    return retval;
}

typedef OSErr (*open_procp)(HParmBlkPtr pb, BOOLEAN sync);

static OSErr
open_helper(FSSpecPtr spec, SignedByte perms, GUEST<int16_t> *refoutp,
            open_procp procp)
{
    OSErr retval;
    HParamBlockRec hpb;

    hpb.ioParam.ioVRefNum = spec->vRefNum;
    hpb.fileParam.ioDirID = spec->parID;
    hpb.ioParam.ioNamePtr = RM((StringPtr)spec->name);
    if(perms == fsWrPerm)
    {
        warning_unexpected(NULL_STRING);
        perms = fsRdWrPerm;
    }
    hpb.ioParam.ioPermssn = perms;
    retval = procp(&hpb, false);
    if(retval == noErr)
        *refoutp = hpb.ioParam.ioRefNum;
    return retval;
}

OSErr Executor::C_FSpOpenDF(FSSpecPtr spec, SignedByte perms,
                            GUEST<int16_t> *refoutp)
{
    return open_helper(spec, perms, refoutp, PBHOpenDF);
}

OSErr Executor::C_FSpOpenRF(FSSpecPtr spec, SignedByte perms,
                            GUEST<int16_t> *refoutp)
{
    return open_helper(spec, perms, refoutp, PBHOpenRF);
}

OSErr Executor::C_FSpCreate(FSSpecPtr spec, OSType creator, OSType file_type,
                            ScriptCode script)
{
    OSErr retval;

    retval = HCreate(CW(spec->vRefNum), CL(spec->parID), spec->name,
                     creator, file_type);
    return retval;
}

OSErr Executor::C_FSpDirCreate(FSSpecPtr spec, ScriptCode script,
                               GUEST<int32_t> *created_dir_id)
{
    OSErr retval;
    HParamBlockRec hpb;

    hpb.ioParam.ioVRefNum = spec->vRefNum;
    hpb.fileParam.ioDirID = spec->parID;
    hpb.ioParam.ioNamePtr = RM((StringPtr)spec->name);
    retval = PBDirCreate(&hpb, false);
    if(retval == noErr)
        *created_dir_id = hpb.fileParam.ioDirID;
    return retval;
}

OSErr Executor::C_FSpDelete(FSSpecPtr spec)
{
    OSErr retval;
    HParamBlockRec hpb;

    hpb.ioParam.ioVRefNum = spec->vRefNum;
    hpb.fileParam.ioDirID = spec->parID;
    hpb.ioParam.ioNamePtr = RM((StringPtr)spec->name);
    retval = PBHDelete(&hpb, false);
    return retval;
}

OSErr Executor::C_FSpGetFInfo(FSSpecPtr spec, FInfo *fndr_info)
{
    OSErr retval;
    HParamBlockRec hpb;

    hpb.fileParam.ioVRefNum = spec->vRefNum;
    hpb.fileParam.ioDirID = spec->parID;
    hpb.fileParam.ioNamePtr = RM((StringPtr)spec->name);
    hpb.fileParam.ioFDirIndex = CWC(0);
    retval = PBHGetFInfo(&hpb, false);
    if(retval == noErr)
        *fndr_info = hpb.fileParam.ioFlFndrInfo;
    return retval;
}

OSErr Executor::C_FSpSetFInfo(FSSpecPtr spec, FInfo *fndr_info)
{
    OSErr retval;
    HParamBlockRec hpb;

    warning_unimplemented("poorly implemented: call of PBHGetFInfo wasteful");
    hpb.fileParam.ioVRefNum = spec->vRefNum;
    hpb.fileParam.ioDirID = spec->parID;
    hpb.fileParam.ioNamePtr = RM((StringPtr)spec->name);
    hpb.fileParam.ioFDirIndex = CWC(0);
    retval = PBHGetFInfo(&hpb, false);
    if(retval == noErr)
    {
        hpb.fileParam.ioDirID = spec->parID;
        hpb.fileParam.ioFlFndrInfo = *fndr_info;
        retval = PBHSetFInfo(&hpb, false);
    }
    return retval;
}

typedef OSErr (*lock_procp)(HParmBlkPtr pb, BOOLEAN async);

static OSErr
lock_helper(FSSpecPtr spec, lock_procp procp)
{
    OSErr retval;
    HParamBlockRec hpb;

    hpb.fileParam.ioVRefNum = spec->vRefNum;
    hpb.fileParam.ioDirID = spec->parID;
    hpb.fileParam.ioNamePtr = RM((StringPtr)spec->name);
    retval = procp(&hpb, false);
    return retval;
}

OSErr Executor::C_FSpSetFLock(FSSpecPtr spec)
{
    return lock_helper(spec, PBHSetFLock);
}

OSErr Executor::C_FSpRstFLock(FSSpecPtr spec)
{
    return lock_helper(spec, PBHRstFLock);
}

OSErr Executor::C_FSpRename(FSSpecPtr spec, Str255 new_name)
{
    OSErr retval;
    HParamBlockRec hpb;

    hpb.fileParam.ioVRefNum = spec->vRefNum;
    hpb.fileParam.ioDirID = spec->parID;
    hpb.fileParam.ioNamePtr = RM((StringPtr)spec->name);
    hpb.ioParam.ioMisc = guest_cast<LONGINT>(RM(new_name));
    retval = PBHRename(&hpb, false);
    return retval;
}

OSErr Executor::C_FSpCatMove(FSSpecPtr src, FSSpecPtr dst)
{
    OSErr retval;
    CMovePBRec cbr;

    if(src->vRefNum != dst->vRefNum)
        retval = paramErr;
    else
    {
        cbr.ioVRefNum = src->vRefNum;
        cbr.ioDirID = src->parID;
        cbr.ioNamePtr = RM((StringPtr)src->name);
        cbr.ioNewName = RM((StringPtr)dst->name);
        cbr.ioNewDirID = dst->parID;
        retval = PBCatMove(&cbr, false);
    }
    return retval;
}

void Executor::C_FSpCreateResFile(FSSpecPtr spec, OSType creator,
                                  OSType file_type, ScriptCode script)
{
    HCreateResFile_helper(CW(spec->vRefNum), CL(spec->parID),
                          spec->name, creator, file_type, script);
}

INTEGER Executor::C_FSpOpenResFile(FSSpecPtr spec, SignedByte perms)
{
    INTEGER retval;

    retval = HOpenResFile(CW(spec->vRefNum), CL(spec->parID), spec->name,
                          perms);
    return retval;
}

/* NOTE: the HCreate and HOpenRF are not traps, they're just high level
   calls that are handy to use elsewhere, so they're included here. */

OSErr
Executor::HCreate(INTEGER vref, LONGINT dirid, Str255 name, OSType creator, OSType type)
{
    HParamBlockRec hpb;
    OSErr retval;

    hpb.fileParam.ioNamePtr = RM(name);
    hpb.fileParam.ioVRefNum = CW(vref);
    hpb.fileParam.ioDirID = CL(dirid);
    retval = PBHCreate(&hpb, false);
    if(retval == noErr)
    {
        hpb.fileParam.ioFDirIndex = CWC(0);
        retval = PBHGetFInfo(&hpb, false);
        if(retval == noErr)
        {
            hpb.fileParam.ioFlFndrInfo.fdCreator = CL(creator);
            hpb.fileParam.ioFlFndrInfo.fdType = CL(type);
            hpb.fileParam.ioDirID = CL(dirid);
            retval = PBHSetFInfo(&hpb, false);
        }
    }
    return retval;
}

OSErr
Executor::HOpenRF(INTEGER vref, LONGINT dirid, Str255 name, SignedByte perm,
                  GUEST<INTEGER> *refp)
{
    HParamBlockRec hpb;
    OSErr retval;

    hpb.fileParam.ioNamePtr = RM(name);
    hpb.fileParam.ioVRefNum = CW(vref);
    hpb.ioParam.ioPermssn = CB(perm);
    hpb.ioParam.ioMisc = CLC(0);
    hpb.fileParam.ioDirID = CL(dirid);
    retval = PBHOpenRF(&hpb, false);
    if(retval == noErr)
        *refp = hpb.ioParam.ioRefNum;
    return retval;
}

OSErr
Executor::HOpenDF(INTEGER vref, LONGINT dirid, Str255 name, SignedByte perm,
                  GUEST<INTEGER> *refp)
{
    HParamBlockRec hpb;
    OSErr retval;

    hpb.fileParam.ioNamePtr = RM(name);
    hpb.fileParam.ioVRefNum = CW(vref);
    hpb.ioParam.ioPermssn = CB(perm);
    hpb.ioParam.ioMisc = CLC(0);
    hpb.fileParam.ioDirID = CL(dirid);
    retval = PBHOpenDF(&hpb, false);
    if(retval == noErr)
        *refp = hpb.ioParam.ioRefNum;
    return retval;
}

OSErr
Executor::HOpen(INTEGER vref, LONGINT dirid, Str255 name, SignedByte perm,
                  GUEST<INTEGER> *refp)
{
    HParamBlockRec hpb;
    OSErr retval;

    hpb.fileParam.ioNamePtr = RM(name);
    hpb.fileParam.ioVRefNum = CW(vref);
    hpb.ioParam.ioPermssn = CB(perm);
    hpb.ioParam.ioMisc = CLC(0);
    hpb.fileParam.ioDirID = CL(dirid);
    retval = PBHOpen(&hpb, false);
    if(retval == noErr)
        *refp = hpb.ioParam.ioRefNum;
    return retval;
}
