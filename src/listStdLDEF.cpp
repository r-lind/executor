/* Copyright 1989, 1990 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#include "rsys/common.h"
#include "ListMgr.h"
#include "MemoryMgr.h"
#include "rsys/list.h"

using namespace Executor;

static void draw(BOOLEAN sel, Rect *rect, INTEGER doff, INTEGER dl,
                 ListHandle list)
{
    GrafPtr savePort;

    savePort = qdGlobals().thePort;
    SetPort(HxP(list, port));
    EraseRect(rect);
    MoveTo(rect->left + Hx(list, indent.h), rect->top + Hx(list, indent.v));
    HLock((Handle)HxP(list, cells));
    DrawText((Ptr)*HxP(list, cells) + doff, 0, dl);
    HUnlock((Handle)HxP(list, cells));
    if(sel)
        InvertRect(rect);
    SetPort(savePort);
}

void Executor::C_ldef0(INTEGER msg, BOOLEAN sel, Rect *rect, Cell cell,
                       INTEGER doff, INTEGER dl, ListHandle list) /* IMIV-276 */
{
    GrafPtr savePort;
    FontInfo fi;

    switch(msg)
    {
        case lInitMsg:
            savePort = qdGlobals().thePort;
            SetPort(HxP(list, port));
            GetFontInfo(&fi);
            HxX(list, indent.h) = 5;
            HxX(list, indent.v) = fi.ascent;
            SetPort(savePort);
            break;
        case lDrawMsg:
            draw(sel, rect, doff, dl, list);
            break;
        case lHiliteMsg:
            savePort = qdGlobals().thePort;
            SetPort(HxP(list, port));
            InvertRect(rect);
            SetPort(savePort);
            break;
        case lCloseMsg: /* nothing special to do */
            break;
        default: /* weirdness */
            break;
    }
}
