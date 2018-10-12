/* Copyright 1992 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#include "rsys/common.h"
#include "OSUtil.h"
#include "FileMgr.h"
#include "MemoryMgr.h"
#include "rsys/hfs.h"
#include "rsys/file.h"

using namespace Executor;

#if defined(TESTFCB)
void testfcb()
{
    short length;
    filecontrolblock *fp;
    INTEGER i;

    length = CW(*(short *)CL(LM(FCBSPtr)));
    fp = (filecontrolblock *)((short *)CL(LM(FCBSPtr)) + 1);
    printf("length = %d, length / 94 = %d, length mod 94 = %d\n",
           length, length / 94, length % 94);
    for(i = 0; i < 40 && i < length / 94; i++, fp++)
    {
        printf("# %ld flags 0x%x vers %d sblk %d EOF %ld PLEN %ld mark %ld\n"
               "vptr 0x%lx pbuffer 0x%lx FlPos %d clmpsiz %ld BTCBPtr 0x%lx\n"
               "ext (%d %d) (%d %d) (%d %d) FNDR '%c%c%c%c' CatPos 0x%lx\n"
               "parid %ld name %s\n",
               CL(fp->fcbFlNum), fp->fcbMdRByt,
               fp->fcbTypByt, CW(fp->fcbSBlk), CL(fp->fcbEOF), CL(fp->fcbPLen), CL(fp->fcbCrPs),
               CL(fp->fcbVPtr), CL(fp->fcbBfAdr), CW(fp->fcbFlPos), CL(fp->fcbClmpSize),
               CL(fp->fcbBTCBPtr),
               CW(fp->fcbExtRec[0].blockstart), CW(fp->fcbExtRec[0].blockcount),
               CW(fp->fcbExtRec[1].blockstart), CW(fp->fcbExtRec[1].blockcount),
               CW(fp->fcbExtRec[2].blockstart), CW(fp->fcbExtRec[2].blockcount),
               (short)(CL(fp->fcbFType) >> 24), (short)(CL(fp->fcbFType) >> 16),
               (short)CL(fp->fcbFType) >> 8, (short)CL(fp->fcbFType),
               CL(fp->fcbCatPos), CL(fp->fcbDirID), fp->fcbCName + 1);
    }
}
#endif /* TESTFCB */

#if 0
void myFInitQueue(void)/* IMIV-128 */
{
    /* When we support asynchronous stuff we'll have to do this */
}

QHdrPtr myGetFSQHdr(void)
{
    return &LM(FSQHdr);
}

QHdrPtr myGetVCBQHdr(void)
{
    return &LM(VCBQHdr);
}
#endif


#if 0
QHdrPtr myGetDrvQHdr(void)
{
    return &LM(DrvQHdr);
}
#endif
