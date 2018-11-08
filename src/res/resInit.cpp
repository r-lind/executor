/* Copyright 1986 - 2000 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in ResourceMgr.h (DO NOT DELETE THIS LINE) */

#include "base/common.h"

#include "ResourceMgr.h"
#include "QuickDraw.h"
#include "MemoryMgr.h"

#include "res/resource.h"
#include "file/file.h"
#include "mman/mman.h"
#include "commandline/flags.h"
#include "rsys/version.h"
#include "rsys/appearance.h"

using namespace Executor;

/* extracts an int32_t from a string of the form [^(]*([0-9]*).*
 * () == 0, any invalid string == -1
 */

static int32_t
extract_vers_num(Handle h)
{
    uint32_t retval, old_retval;
    const char *p, *ep;
    bool seen_left, seen_right, done;

    retval = old_retval = 0;
    seen_left = false;
    seen_right = false;
    done = false;
    for(p = (char *)*h, ep = p + GetHandleSize(h);
        !done && p < ep; ++p)
    {
        switch(*p)
        {
            case '(':
                if(seen_left)
                    done = true;
                else
                    seen_left = true;
                break;
            case ')':
                seen_right = true;
                done = true;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if(seen_left)
                    retval = retval * 10 + *p - '0';
                if(retval < old_retval)
                    done = true;
                else
                    old_retval = retval;
                break;
            default:
                if(seen_left)
                    done = true;
                break;
        }
    }
    if(!seen_right)
        retval = -1;
    return retval;
}

INTEGER Executor::C_InitResources()
{
    /* Initialize globals */
    TheZoneGuard guard(LM(SysZone));

    Handle versh;
    int32_t versnum;

    LM(TopMapHndl) = nullptr;

    ROMlib_setreserr(noErr);
    str255assign(LM(SysResName), SYSMACNAME);
    LM(SysMap) = OpenRFPerm((StringPtr)SYSMACNAME, LM(BootDrive),
                           fsCurPerm);

    if(LM(SysMap) == -1)
    {
        fprintf(stderr, "OpenRFPerm (\"%.*s\", 0x%x, fsCurPerm) failed\n",
                SYSMACNAME[0], SYSMACNAME + 1, (uint16_t)LM(BootDrive));

        exit(1);
    }

    LM(SysMapHndl) = LM(TopMapHndl);
    ROMlib_invalar();
    SetResLoad(true);
    versh = GetResource(TICK("vers"), 1);
    versnum = extract_vers_num(versh);
    if(versnum < MINIMUM_SYSTEM_FILE_NEEDED)
        system_file_version_skew_p = true;

    ROMlib_set_appearance();

    return LM(SysMap);
}

void Executor::C_RsrcZoneInit() /* no op */
{
}
