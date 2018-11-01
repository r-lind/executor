/* Copyright 1986-1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in TextEdit.h (DO NOT DELETE THIS LINE) */

#include "rsys/common.h"
#include "TextEdit.h"
#include "MemoryMgr.h"
#include "ScrapMgr.h"

#include "rsys/cquick.h"
#include "rsys/mman.h"

using namespace Executor;

OSErr Executor::TEFromScrap()
{
    GUEST<int32_t> l;
    int32_t m;

    m = GetScrap(LM(TEScrpHandle), TICK("TEXT"), &l);
    if(m < 0)
    {
        EmptyHandle(LM(TEScrpHandle));
        LM(TEScrpLength) = 0;
    }
    else
        LM(TEScrpLength) = m;
    return m < 0 ? m : noErr;
}

OSErr Executor::TEToScrap()
{
    int32_t m;

    HLockGuard guard(LM(TEScrpHandle));

    m = PutScrap(LM(TEScrpLength), TICK("TEXT"),
                 *LM(TEScrpHandle));
    return m < 0 ? m : 0;
}

Handle Executor::TEScrapHandle()
{
    return LM(TEScrpHandle);
}

int32_t Executor::TEGetScrapLength()
{
    return LM(TEScrpLength);
}

void Executor::TESetScrapLength(int32_t ln)
{
    LM(TEScrpLength) = ln;
}
