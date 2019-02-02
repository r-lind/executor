/* Copyright 1986, 1989, 1990 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in QuickDraw.h (DO NOT DELETE THIS LINE) */

#include "rsys/common.h"
#include "QuickDraw.h"
#include "CQuickDraw.h"
#include "DialogMgr.h"
#include "rsys/quick.h"
#include "rsys/cquick.h"
#include "rsys/ctl.h"
#include "rsys/wind.h"

using namespace Executor;

void Executor::C_FrameRect(Rect *r)
{
    CALLRECT(frame, r);
}

void Executor::C_PaintRect(Rect *r)
{
    CALLRECT(paint, r);
}

void Executor::C_EraseRect(Rect *r)
{
    CALLRECT(erase, r);
}

void Executor::C_InvertRect(Rect *r)
{
    CALLRECT(invert, r);
}

void Executor::C_FillRect(Rect *r, Pattern pat)
{
    if(!EmptyRgn(PORT_VIS_REGION(thePort)))
    {
        ROMlib_fill_pat(pat);
        CALLRECT(fill, r);
    }
}

void Executor::C_FrameOval(Rect *r)
{
    CALLOVAL(frame, r);
}

void Executor::C_PaintOval(Rect *r)
{
    CALLOVAL(paint, r);
}

void Executor::C_EraseOval(Rect *r)
{
    CALLOVAL(erase, r);
}

void Executor::C_InvertOval(Rect *r)
{
    CALLOVAL(invert, r);
}

void Executor::C_FillOval(Rect *r, Pattern pat)
{
    ROMlib_fill_pat(pat);
    CALLOVAL(fill, r);
}

void Executor::C_FrameRoundRect(Rect *r, INTEGER ow, INTEGER oh)
{
    CALLRRECT(frame, r, ow, oh);
}

void Executor::C_PaintRoundRect(Rect *r, INTEGER ow, INTEGER oh)
{
    CALLRRECT(paint, r, ow, oh);
}

void Executor::C_EraseRoundRect(Rect *r, INTEGER ow, INTEGER oh)
{
    CALLRRECT(erase, r, ow, oh);
}

void Executor::C_InvertRoundRect(Rect *r, INTEGER ow, INTEGER oh)
{
    CALLRRECT(invert, r, ow, oh);
}

void Executor::C_FillRoundRect(Rect *r, INTEGER ow, INTEGER oh, Pattern pat)
{
    ROMlib_fill_pat(pat);
    CALLRRECT(fill, r, ow, oh);
}

void Executor::C_FrameArc(Rect *r, INTEGER start, INTEGER angle)
{
    CALLARC(frame, r, start, angle);
}

void Executor::C_PaintArc(Rect *r, INTEGER start, INTEGER angle)
{
    CALLARC(paint, r, start, angle);
}

void Executor::C_EraseArc(Rect *r, INTEGER start, INTEGER angle)
{
    CALLARC(erase, r, start, angle);
}

void Executor::C_InvertArc(Rect *r, INTEGER start, INTEGER angle)
{
    CALLARC(invert, r, start, angle);
}

void Executor::C_FillArc(Rect *r, INTEGER start, INTEGER angle, Pattern pat)
{
    ROMlib_fill_pat(pat);
    CALLARC(fill, r, start, angle);
}

void Executor::C_FrameRgn(RgnHandle rh)
{
    CALLRGN(frame, rh);
}

void Executor::C_PaintRgn(RgnHandle rh)
{
    CALLRGN(paint, rh);
}

void Executor::C_EraseRgn(RgnHandle rh)
{
    CALLRGN(erase, rh);
}

void Executor::C_InvertRgn(RgnHandle rh)
{
    CALLRGN(invert, rh);
}

void Executor::C_FillRgn(RgnHandle rh, Pattern pat)
{
    ROMlib_fill_pat(pat);
    CALLRGN(fill, rh);
}

void Executor::C_FramePoly(PolyHandle poly)
{
    CALLPOLY(frame, poly);
}

void Executor::C_PaintPoly(PolyHandle poly)
{
    CALLPOLY(paint, poly);
}

void Executor::C_ErasePoly(PolyHandle poly)
{
    CALLPOLY(erase, poly);
}

void Executor::C_InvertPoly(PolyHandle poly)
{
    CALLPOLY(invert, poly);
}

void Executor::C_FillPoly(PolyHandle poly, Pattern pat)
{
    ROMlib_fill_pat(pat);
    CALLPOLY(fill, poly);
}
