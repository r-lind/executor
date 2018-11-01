/* Copyright 1986, 1989, 1990 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in QuickDraw.h (DO NOT DELETE THIS LINE) */

/* HLock checked by ctm on Mon May 13 17:55:01 MDT 1991 */

#include "rsys/common.h"
#include "QuickDraw.h"
#include "MemoryMgr.h"

#include "rsys/quick.h"
#include "rsys/cquick.h"

using namespace Executor;

PolyHandle Executor::C_OpenPoly()
{
    PolyHandle ph;

    ph = (PolyHandle)NewHandle((Size)SMALLPOLY);
    HxX(ph, polySize) = SMALLPOLY;
    PORT_POLY_SAVE_X(qdGlobals().thePort) = (Handle)ph;
    HidePen();
    return (ph);
}

void Executor::C_ClosePoly()
{
    INTEGER top = 32767, left = 32767, bottom = -32767, right = -32767;
    GUEST<INTEGER> *ip, *ep;
    INTEGER i;
    PolyHandle ph;

    ph = (PolyHandle)PORT_POLY_SAVE(qdGlobals().thePort);
    for(ip = (GUEST<INTEGER> *)((char *)*ph + SMALLPOLY),
    ep = (GUEST<INTEGER> *)((char *)*ph + Hx(ph, polySize));
        ip != ep;)
    {
        if((i = *ip) <= top)
            top = i;
        ++ip;
        if(i >= bottom)
            bottom = i;
        if((i = *ip) <= left)
            left = i;
        ++ip;
        if(i >= right)
            right = i;
    }
    HxX(ph, polyBBox.top) = top;
    HxX(ph, polyBBox.left) = left;
    HxX(ph, polyBBox.bottom) = bottom;
    HxX(ph, polyBBox.right) = right;
    PORT_POLY_SAVE_X(qdGlobals().thePort) = nullptr;
    ShowPen();
}

void Executor::C_KillPoly(PolyHandle poly)
{
    DisposeHandle((Handle)poly);
}

void Executor::C_OffsetPoly(PolyHandle poly, INTEGER dh,
                            INTEGER dv) /* Note: IM I-191 is wrong */
{
    GUEST<Point> *pp, *ep;

    if(dh || dv)
    {
        HxX(poly, polyBBox.top) = Hx(poly, polyBBox.top) + dv;
        HxX(poly, polyBBox.bottom) = Hx(poly, polyBBox.bottom) + dv;
        HxX(poly, polyBBox.left) = Hx(poly, polyBBox.left) + dh;
        HxX(poly, polyBBox.right) = Hx(poly, polyBBox.right) + dh;
        pp = HxX(poly, polyPoints);
        ep = (GUEST<Point> *)(((char *)*poly) + Hx(poly, polySize));
        while(pp != ep)
        {
            pp->h = pp->h + (dh);
            pp->v = pp->v + (dv);
            pp++;
        }
    }
}
