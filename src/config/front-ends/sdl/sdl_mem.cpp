/* 
 * Copyright 1998 by Abacus Research and
 * Development, Inc.  All rights reserved.
 *
 * Derived from public domain source code written by Sam Lantinga
 */

#include "rsys/common.h"
#include "MemoryMgr.h"

using namespace Executor;

char *sdl_ReallocHandle(Executor::Handle mem, int len)
{
    ReallocateHandle(mem, len);
    if(LM(MemErr) != noErr)
        return nullptr;
    else
        return (char *)*mem;
}
