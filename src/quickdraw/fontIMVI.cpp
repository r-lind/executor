/* Copyright 1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#include <base/common.h>

#include <FontMgr.h>

using namespace Executor;

static bool outline_preferred_p = false;

void Executor::C_SetOutlinePreferred(Boolean _outline_preferred_p)
{
    outline_preferred_p = _outline_preferred_p;
}

Boolean Executor::C_GetOutlinePreferred()
{
    return outline_preferred_p;
}

Boolean Executor::C_IsOutline(Point numer, Point denom)
{
    return false;
}

OSErr Executor::C_OutlineMetrics(int16_t byte_count, Ptr text, Point numer,
                                 Point denom, GUEST<int16_t> *y_max, GUEST<int16_t> *y_min,
                                 GUEST<Fixed> *aw_array, GUEST<Fixed> *lsb_array,
                                 Rect *bounds_array)
{
    warning_unimplemented("");
    /* ### paramErr */
    return -50;
}

static bool preserve_glyph_p = false;

void Executor::C_SetPreserveGlyph(Boolean _preserve_glyph_p)
{
    preserve_glyph_p = _preserve_glyph_p;
}

Boolean Executor::C_GetPreserveGlyph()
{
    return preserve_glyph_p;
}

OSErr Executor::C_FlushFonts()
{
    warning_unimplemented("");
    /* ### paramErr */
    return -50;
}
