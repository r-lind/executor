#include <rsys/volume.h>
#include <rsys/macros.h>
#include <MemoryMgr.h>

using namespace Executor;

Volume::~Volume()
{
    
}

OSErr Volume::PBGetVInfo(ParmBlkPtr pb, BOOLEAN async)
{
    if(pb->volumeParam.ioNamePtr)
        str255assign(MR(pb->volumeParam.ioNamePtr), vcb.vcbVN);
    pb->volumeParam.ioVCrDate = vcb.vcbCrDate;
    pb->volumeParam.ioVLsBkUp = vcb.vcbVolBkUp;
    pb->volumeParam.ioVAtrb = vcb.vcbAtrb;
    pb->volumeParam.ioVNmFls = vcb.vcbNmFls;
    pb->volumeParam.ioVNmAlBlks = vcb.vcbNmAlBlks;
    pb->volumeParam.ioVAlBlkSiz = vcb.vcbAlBlkSiz;
    pb->volumeParam.ioVClpSiz = vcb.vcbClpSiz;
    pb->volumeParam.ioAlBlSt = vcb.vcbAlBlSt;
    pb->volumeParam.ioVNxtFNum = vcb.vcbNxtCNID;
    pb->volumeParam.ioVFrBlk = vcb.vcbFreeBks;

    return noErr;
}
OSErr Volume::PBHGetVInfo(HParmBlkPtr pb, BOOLEAN async)
{
    PBGetVInfo(reinterpret_cast<ParmBlkPtr>(pb), false);

    pb->volumeParam.ioVRefNum = vcb.vcbVRefNum;
    pb->volumeParam.ioVSigWord = vcb.vcbSigWord;
    pb->volumeParam.ioVDrvInfo = vcb.vcbDrvNum;
    pb->volumeParam.ioVDRefNum = vcb.vcbDRefNum;
    pb->volumeParam.ioVFSID = vcb.vcbFSID;
    pb->volumeParam.ioVBkUp = vcb.vcbVolBkUp;
    pb->volumeParam.ioVSeqNum = vcb.vcbVSeqNum;
    pb->volumeParam.ioVWrCnt = vcb.vcbWrCnt;
    pb->volumeParam.ioVFilCnt = vcb.vcbFilCnt;
    pb->volumeParam.ioVDirCnt = vcb.vcbDirCnt;
    memcpy(pb->volumeParam.ioVFndrInfo, vcb.vcbFndrInfo, sizeof(vcb.vcbFndrInfo));

#if 0
        // copied from ufs - system folder?
    if(ISWDNUM(Cx(LM(BootDrive))))
    {
        wdp = WDNUMTOWDP(Cx(LM(BootDrive)));
        if(MR(wdp->vcbp) == vcbp)
            pb->volumeParam.ioVFndrInfo[1] = wdp->dirid;
    }
#endif

    return noErr;
}
