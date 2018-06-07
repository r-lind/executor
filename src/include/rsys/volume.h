#pragma once

#include <ExMacTypes.h>
#include <FileMgr.h>

namespace Executor
{
    class Volume
    {
    public:
        virtual ~Volume();


        virtual OSErr PBHRename(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBHCreate(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBDirCreate(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBHDelete(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBHOpen(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBHOpenDF(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBHOpenRF(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBGetCatInfo(CInfoPBPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBSetCatInfo(CInfoPBPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBCatMove(CMovePBPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBGetVInfo(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBHGetFInfo(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBOpen(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBOpenRF(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBCreate(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBDelete(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBOpenWD(WDPBPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBGetFInfo(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBSetFInfo(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBHSetFInfo(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBSetFLock(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBHSetFLock(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBRstFLock(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBHRstFLock(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBSetFVers(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBRename(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBHGetVInfo(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBSetVInfo(HParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBFlushVol(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBUnmountVol(ParmBlkPtr pb) = 0;
        virtual OSErr PBEject(ParmBlkPtr pb) = 0;
        virtual OSErr PBOffLine(ParmBlkPtr pb) = 0;

        virtual OSErr PBRead(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBWrite(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBClose(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBAllocate(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBAllocContig(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBSetEOF(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBLockRange(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBUnlockRange(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBGetFPos(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBSetFPos(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBGetEOF(ParmBlkPtr pb, BOOLEAN async) = 0;
        virtual OSErr PBFlushFile(ParmBlkPtr pb, BOOLEAN async) = 0;

    };
}
