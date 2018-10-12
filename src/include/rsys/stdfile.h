#if !defined(__RSYS_STDFILE__)
#define __RSYS_STDFILE__

#include "FileMgr.h"
#include "EventMgr.h"
#include "ControlMgr.h"
#include "DialogMgr.h"
#include "rsys/file.h"

#define MODULE_NAME rsys_stdfile
#include <rsys/api-module.h>

namespace Executor
{
/*
 * Copyright 1989 - 1995 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

#define putPrompt 3
#define putDiskName 4
#define putNmList 8

#define getDiskName 4
#define getDotted 9

#define MICONLETTER 0
#define MICONCFOLDER 1
#define MICONFLOPPY 2
#define MICONOFOLDER 3
#define MICONAPP 4
#define MICONDISK 5

#define FAKEREDRAW 101
#define FAKECURDIR 102
#define FAKEOPENDIR 103

extern void C_ROMlib_stdftrack(ControlHandle, INTEGER);
PASCAL_FUNCTION(ROMlib_stdftrack);
extern Boolean C_ROMlib_stdffilt(DialogPtr, EventRecord *, GUEST<INTEGER> *);
PASCAL_FUNCTION(ROMlib_stdffilt);
extern void ROMlib_init_stdfile(void);

enum
{
    STANDARD_HEIGHT = 200,
    STANDARD_WIDTH = 348
};

OSErr C_unixmount(CInfoPBRec *cbp);
PASCAL_FUNCTION(unixmount);
}
#endif /* !defined(__RSYS_STDFILE__) */
