/* Copyright 1986, 1988, 1989, 1990 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in QuickDraw.h (DO NOT DELETE THIS LINE) */

#include "rsys/common.h"
#include "QuickDraw.h"
#include "CQuickDraw.h"
#include "MemoryMgr.h"
#include "rsys/cquick.h"
#include "rsys/picture.h"

using namespace Executor;

void Executor::C_StdOval(GrafVerb v, Rect *rp)
{
    Rect r;
    RgnHandle rh, rh2;
    PAUSEDECL;

    if(!EmptyRect(rp))
    {
        if(qdGlobals().thePort->picSave)
        {
            ROMlib_drawingverbrectpicupdate(v, rp);
            PICOP(OP_frameOval + (int)v);
            PICWRITE(rp, sizeof(*rp));
        };

        PAUSERECORDING;
        if(rp->bottom - rp->top < 4 && rp->right - rp->left < 4)
            StdRect(v, rp);
        else
        {
            rh = ROMlib_circrgn(rp);
            switch(v)
            {
                case frame:
                    if(PORT_REGION_SAVE(qdGlobals().thePort))
                        XorRgn(rh,
                               (RgnHandle)PORT_REGION_SAVE(qdGlobals().thePort),
                               (RgnHandle)PORT_REGION_SAVE(qdGlobals().thePort));
                    if(PORT_PEN_VIS(qdGlobals().thePort) >= 0)
                    {
                        r.top = rp->top + PORT_PEN_SIZE(qdGlobals().thePort).v;
                        r.left = rp->left + PORT_PEN_SIZE(qdGlobals().thePort).h;
                        r.bottom = rp->bottom - PORT_PEN_SIZE(qdGlobals().thePort).v;
                        r.right = rp->right - PORT_PEN_SIZE(qdGlobals().thePort).h;
                        if(r.top < r.bottom && r.left < r.right)
                        {
                            rh2 = ROMlib_circrgn(&r);
                            XorRgn(rh, rh2, rh);
                            DisposeRgn(rh2);
                        }
                        StdRgn(paint, rh);
                    }
                    break;
                case paint:
                case erase:
                case invert:
                case fill:
                    StdRgn(v, rh);
            }
            DisposeRgn(rh);
        }
        RESUMERECORDING;
    }
}
