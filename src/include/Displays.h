#pragma once

#include <AppleEvents.h>
#include <ProcessMgr.h>

#define MODULE_NAME Displays
#include <base/api-module.h>

namespace Executor
{

typedef UPP<void(AppleEvent * theEvent)> DMNotificationUPP;

DISPATCHER_TRAP(DisplayDispatch, 0xABEB, D0W);

extern OSErr C_DMRegisterNotifyProc(DMNotificationUPP proc, ProcessSerialNumber *psn);
PASCAL_SUBTRAP(DMRegisterNotifyProc, 0xABEB, 0x0414, DisplayDispatch);

}