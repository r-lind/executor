#if !defined(_PROCESSMGR_H_)
#define _PROCESSMGR_H_

#include <FileMgr.h>
#include <PPC.h>

#define MODULE_NAME ProcessMgr
#include <base/api-module.h>

/*
 * Copyright 1995, 1996 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

namespace Executor
{
typedef INTEGER LaunchFlags;

struct ProcessSerialNumber
{
    GUEST_STRUCT;
    GUEST<uint32_t> highLongOfPSN;
    GUEST<uint32_t> lowLongOfPSN;
};

/* for now, anyway */

/*
 * ARDI implementation of AppParameters.  This has not been determined by
 * looking at the data Apple returns, so it will not play well with anything
 * that knows the format of this data structure.  It's added so we can
 * support the launching of non-Mac applications from Executor.  TODO: figure
 * out exactly what the mac uses
 */

typedef struct
{
    GUEST_STRUCT;
    GUEST<uint32_t> magic;
    GUEST<INTEGER> n_fsspec;
    FSSpec fsspec[0];
} ROMlib_AppParameters_t;

enum
{
    APP_PARAMS_MAGIC = 0xd6434a1b,
}; /* chosen from /dev/random */

typedef ROMlib_AppParameters_t *AppParametersPtr;

struct LaunchParamBlockRec
{
    GUEST_STRUCT;
    GUEST<LONGINT> reserved1;
    GUEST<INTEGER> reserved2;
    GUEST<INTEGER> launchBlockID;
    GUEST<LONGINT> launchEPBLength;
    GUEST<INTEGER> launchFileFlags;
    GUEST<LaunchFlags> launchControlFlags;
    GUEST<FSSpecPtr> launchAppSpec;
    GUEST<ProcessSerialNumber> launchProcessSN;
    GUEST<LONGINT> launchPreferredSize;
    GUEST<LONGINT> launchMinimumSize;
    GUEST<LONGINT> launchAvailableSize;
    GUEST<AppParametersPtr> launchAppParameters;
};

enum
{
    extendedBlock = 0x4C43
};
enum
{
    extendedBlockLen = sizeof(LaunchParamBlockRec) - 12
};
enum
{
    launchContinue = 0x4000
};

struct ProcessInfoRec
{
    GUEST_STRUCT;
    GUEST<uint32_t> processInfoLength;
    GUEST<StringPtr> processName;
    GUEST<ProcessSerialNumber> processNumber;
    GUEST<uint32_t> processType;
    GUEST<OSType> processSignature;
    GUEST<uint32_t> processMode;
    GUEST<Ptr> processLocation;
    GUEST<uint32_t> processSize;
    GUEST<uint32_t> processFreeMem;
    GUEST<ProcessSerialNumber> processLauncher;
    GUEST<uint32_t> processLaunchDate;
    GUEST<uint32_t> processActiveTime;
    GUEST<FSSpecPtr> processAppSpec;
};
typedef ProcessInfoRec *ProcessInfoPtr;

/* flags for the `processMode' field of the `ProcessInformationRec'
   record */

enum
{
    modeDeskAccessory = 0x00020000,
    modeMultiLaunch = 0x00010000,
    modeNeedSuspendResume = 0x00004000,
    modeCanBackground = 0x00001000,
    modeDoesActivateOnFGSwitch = 0x00000800,
    modeOnlyBackground = 0x00000400,
    modeGetFrontClicks = 0x00000200,
    modeGetAppDiedMsg = 0x00000100,
    mode32BitCompatible = 0x00000080,
    modeHighLevelEventAware = 0x00000040,
    modeLocalAndRemoteHLEvents = 0x00000020,
    modeStationeryAware = 0x00000010,
    modeUseTextEditServices = 0x00000008,
};

enum
{
    kNoProcess = (0),
    kSystemProcess = (1),
    kCurrentProcess = (2),
};

enum
{
    procNotFound = (-600),
};

EXTERN_DISPATCHER_TRAP(OSDispatch, 0xA88F, StackW);

extern void process_create(bool desk_accessory_p,
                           uint32_t type, uint32_t signature);

extern OSErr C_GetCurrentProcess(ProcessSerialNumber *serial_number);
PASCAL_SUBTRAP(GetCurrentProcess, 0xA88F, 0x0037, OSDispatch);

extern OSErr C_GetNextProcess(ProcessSerialNumber *serial_number);
PASCAL_SUBTRAP(GetNextProcess, 0xA88F, 0x0038, OSDispatch);

extern OSErr C_GetProcessInformation(ProcessSerialNumber *serial_number,
                                                 ProcessInfoPtr info);
PASCAL_SUBTRAP(GetProcessInformation, 0xA88F, 0x003A, OSDispatch);

extern OSErr C_SameProcess(ProcessSerialNumber *serial_number0,
                                       ProcessSerialNumber *serial_number1,
                                       Boolean *same_out);
PASCAL_SUBTRAP(SameProcess, 0xA88F, 0x003D, OSDispatch);
extern OSErr C_GetFrontProcess(ProcessSerialNumber *serial_number, int32_t dummy = -1);
PASCAL_SUBTRAP(GetFrontProcess, 0xA88F, 0x0039, OSDispatch);

extern OSErr C_SetFrontProcess(ProcessSerialNumber *serial_number);
PASCAL_SUBTRAP(SetFrontProcess, 0xA88F, 0x003B, OSDispatch);

extern OSErr C_WakeUpProcess(ProcessSerialNumber *serial_number);
PASCAL_SUBTRAP(WakeUpProcess, 0xA88F, 0x003C, OSDispatch);

extern OSErr C_GetProcessSerialNumberFromPortName(PPCPortPtr port_name,
                                                              ProcessSerialNumber *serial_number);
PASCAL_SUBTRAP(GetProcessSerialNumberFromPortName, 0xA88F, 0x0035, OSDispatch);

extern OSErr C_GetPortNameFromProcessSerialNumber(PPCPortPtr port_name,
                                                              ProcessSerialNumber *serial_number);
PASCAL_SUBTRAP(GetPortNameFromProcessSerialNumber, 0xA88F, 0x0046, OSDispatch);

extern OSErr NewLaunch(StringPtr appl, INTEGER vrefnum,
                       LaunchParamBlockRec *lpbp);

static_assert(sizeof(ProcessSerialNumber) == 8);
static_assert(sizeof(LaunchParamBlockRec) == 44);
static_assert(sizeof(ProcessInfoRec) == 60);
}
#endif
