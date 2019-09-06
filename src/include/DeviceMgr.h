#if !defined(__DEVICEMGR__)
#define __DEVICEMGR__

/*
 * Copyright 1989, 1990 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

#include <WindowMgr.h>
#include <FileMgr.h>

#define MODULE_NAME DeviceMgr
#include <base/api-module.h>

/*
 * Note the structure below is similar to that presented on IM-188,
 * but I don't use offsets to the routines, but pointers to the routines
 * directly.  This means the size of the structure is larger than one
 * might expect.
 */

namespace Executor
{

struct DCtlEntry;
typedef DCtlEntry *DCtlPtr;
typedef GUEST<DCtlPtr> *DCtlHandle;
typedef GUEST<DCtlHandle> *DCtlHandlePtr;

using DriverUPP = UPP<OSErr (ParmBlkPtr, DCtlPtr), Register<D0(A0, A1)>>;

typedef struct
{
    GUEST<DriverUPP> udrvrOpen;
    GUEST<DriverUPP> udrvrPrime; /* read and write */
    GUEST<DriverUPP> udrvrCtl; /* control and killio */
    GUEST<DriverUPP> udrvrStatus;
    GUEST<DriverUPP> udrvrClose;
    Str255 udrvrName;
} umacdriver, *umacdriverptr;

struct ramdriver
{
    GUEST_STRUCT;
    GUEST<INTEGER> drvrFlags;
    GUEST<INTEGER> drvrDelay;
    GUEST<INTEGER> drvrEMask;
    GUEST<INTEGER> drvrMenu;
    GUEST<INTEGER> drvrOpen;
    GUEST<INTEGER> drvrPrime;
    GUEST<INTEGER> drvrCtl;
    GUEST<INTEGER> drvrStatus;
    GUEST<INTEGER> drvrClose;
    GUEST<char> drvrName;
};

typedef ramdriver *ramdriverptr;

typedef GUEST<ramdriverptr> *ramdriverhand;

typedef enum { Open,
               Prime,
               Ctl,
               Stat,
               Close } DriverRoutineType;

struct DCtlEntry
{
    GUEST_STRUCT;
    GUEST<umacdriverptr> dCtlDriver; /* not just Ptr */
    GUEST<INTEGER> dCtlFlags;
    GUEST<QHdr> dCtlQHdr;
    GUEST<LONGINT> dCtlPosition;
    GUEST<Handle> dCtlStorage;
    GUEST<INTEGER> dCtlRefNum;
    GUEST<LONGINT> dCtlCurTicks;
    GUEST<WindowPtr> dCtlWindow;
    GUEST<INTEGER> dCtlDelay;
    GUEST<INTEGER> dCtlEMask;
    GUEST<INTEGER> dCtlMenu;
};


enum
{
    asyncTrpBit = (1 << 10),
    noQueueBit = (1 << 9),
};

enum
{
    NEEDTIMEBIT = (1 << 13),
};

enum
{
    aRdCmd = 2,
    aWrCmd = 3,
};

enum
{
    killCode = 1,
};

enum
{
    NDEVICES = 48,
};

enum
{
    abortErr = (-27),
    badUnitErr = (-21),
    controlErr = (-17),
    dInstErr = (-26),
    dRemovErr = (-25),
    notOpenErr = (-28),
    openErr = (-23),
    readErr = (-19),
    statusErr = (-18),
    unitEmptyErr = (-22),
    writErr = (-20),
};

typedef struct
{
    DriverUPP open;
    DriverUPP prime;
    DriverUPP ctl;
    DriverUPP status;
    DriverUPP close;
    StringPtr name;
    INTEGER refnum;
} driverinfo;

const LowMemGlobal<DCtlHandlePtr> UTableBase { 0x11C }; // DeviceMgr IMII-192 (false);
const LowMemGlobal<ProcPtr[8]> Lvl1DT { 0x192 }; // DeviceMgr IMII-197 (false);
const LowMemGlobal<ProcPtr[8]> Lvl2DT { 0x1B2 }; // DeviceMgr IMII-198 (false);
const LowMemGlobal<INTEGER> UnitNtryCnt { 0x1D2 }; // DeviceMgr ThinkC (true-b);
const LowMemGlobal<Ptr> VIA { 0x1D4 }; // DeviceMgr IMIII-39 (true-b);
const LowMemGlobal<Ptr> SCCRd { 0x1D8 }; // DeviceMgr IMII-199 (false);
const LowMemGlobal<Ptr> SCCWr { 0x1DC }; // DeviceMgr IMII-199 (false);
const LowMemGlobal<Ptr> IWM { 0x1E0 }; // DeviceMgr ThinkC (false);
const LowMemGlobal<ProcPtr[4]> ExtStsDT { 0x2BE }; // DeviceMgr IMII-199 (false);
const LowMemGlobal<Ptr> JFetch { 0x8F4 }; // DeviceMgr IMII-194 (false);
const LowMemGlobal<Ptr> JStash { 0x8F8 }; // DeviceMgr IMII-195 (false);
const LowMemGlobal<Ptr> JIODone { 0x8FC }; // DeviceMgr IMII-195 (false);

extern OSErr PBControl(ParmBlkPtr pbp, BOOLEAN a);
extern OSErr PBStatus(ParmBlkPtr pbp, BOOLEAN a);
extern OSErr PBKillIO(ParmBlkPtr pbp, BOOLEAN a);

FILE_TRAP(PBControl, ParmBlkPtr, 0xA004);
FILE_TRAP(PBStatus, ParmBlkPtr, 0xA005);
FILE_TRAP(PBKillIO, ParmBlkPtr, 0xA006);

extern OSErr OpenDriver(ConstStringPtr name, GUEST<INTEGER> *rnp);
NOTRAP_FUNCTION2(OpenDriver);
extern OSErr CloseDriver(INTEGER rn);
NOTRAP_FUNCTION2(CloseDriver);
extern OSErr Control(INTEGER rn, INTEGER code,
                     Ptr param);
NOTRAP_FUNCTION2(Control);
extern OSErr Status(INTEGER rn, INTEGER code, Ptr param);
NOTRAP_FUNCTION2(Status);
extern OSErr KillIO(INTEGER rn);
NOTRAP_FUNCTION2(KillIO);

extern DCtlHandle GetDCtlEntry(INTEGER rn);
NOTRAP_FUNCTION2(GetDCtlEntry);

static_assert(sizeof(ramdriver) == 20);
static_assert(sizeof(DCtlEntry) == 40);
}
#endif /* __DEVICEMGR__ */
