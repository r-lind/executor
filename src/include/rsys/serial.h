/*
 * Copyright 1998 by Abacus Research and Development, Inc.
 * All rights reserved.
 *
 */

#if !defined(_SERIAL_H_)

#include <MemoryMgr.h>
#include <DeviceMgr.h>

#define MODULE_NAME rsys_serial
#include <base/api-module.h>
#define _SERIAL_H_

namespace Executor
{
struct sersetbuf_t
{
    GUEST_STRUCT;
    GUEST<Ptr> p;
    GUEST<INTEGER> i;
};

extern OSErr C_ROMlib_serialopen(ParmBlkPtr pbp, DCtlPtr dcp);
REGISTER_FUNCTION(ROMlib_serialopen, D0(A0,A1));
extern OSErr C_ROMlib_serialprime(ParmBlkPtr pbp, DCtlPtr dcp);
REGISTER_FUNCTION(ROMlib_serialprime, D0(A0,A1));
extern OSErr C_ROMlib_serialctl(ParmBlkPtr pbp, DCtlPtr dcp);
REGISTER_FUNCTION(ROMlib_serialctl, D0(A0,A1));
extern OSErr C_ROMlib_serialstatus(ParmBlkPtr pbp, DCtlPtr dcp);
REGISTER_FUNCTION(ROMlib_serialstatus, D0(A0,A1));
extern OSErr C_ROMlib_serialclose(ParmBlkPtr pbp, DCtlPtr dcp);
REGISTER_FUNCTION(ROMlib_serialclose, D0(A0,A1));

static_assert(sizeof(sersetbuf_t) == 6);

void InitSerialDriver();
}
#endif
