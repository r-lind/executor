/* Copyright 1986, 1988, 1989, 1990 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in QuickDraw.h (DO NOT DELETE THIS LINE) */

#include <base/common.h>
#include <QuickDraw.h>
#include <CQuickDraw.h>
#include <MemoryMgr.h>

#include <quickdraw/cquick.h>
#include <quickdraw/picture.h>

#include <util/handle_vector.h>

using namespace Executor;

using RgnVector = handle_vector<int16_t, RgnHandle, 10>;

#if 1
RgnHandle Executor::ROMlib_circrgn(const Rect *rectptr)
{
    const Rect& rect = *rectptr;

    RgnVector rgn;
    /*
    rgn.push_back(rect.top);
    rgn.push_back(rect.left+1);
    rgn.push_back(rect.right-1);
    rgn.push_back(RGN_STOP);
    rgn.push_back(rect.top+1);
    rgn.push_back(rect.left);
    rgn.push_back(rect.left+1);
    rgn.push_back(rect.right-1);
    rgn.push_back(rect.right);
    rgn.push_back(RGN_STOP);
    rgn.push_back(rect.bottom);
    rgn.push_back(rect.left);
    rgn.push_back(rect.right);
    rgn.push_back(RGN_STOP);
    rgn.push_back(RGN_STOP);*/



/*
    int x = 0, y = 0;
    int64_t a = int64_t(int(rect.bottom) - rect.top) * (int(rect.bottom) - rect.top);
    int64_t b = int64_t(int(rect.right) - rect.left) * (int(rect.right) - rect.left);


    int64_t y2b = a * b;

    {
        ++x;
        y2b -= a*x;


    }*/

    std::vector<int> vecA, vecB, vecC;

    for(int y = rect.top; y <= rect.bottom; y++)
    {
        bool oldInside = false;
        for(int x = rect.left; x <= rect.right; x++)
        {
            float xx = 2 * (x + 0.5f - rect.left) / float(rect.right - rect.left) - 1.0f;
            float yy = 2 * (y + 0.5f - rect.top) / float(rect.bottom - rect.top) - 1.0f;

            bool inside = xx*xx + yy*yy <= 1;
            if(inside != oldInside)
                vecB.push_back(x);
            oldInside = inside;
        }

        vecC.clear();
        std::set_symmetric_difference(vecA.begin(), vecA.end(), vecB.begin(), vecB.end(), std::back_inserter(vecC));
        swap(vecA, vecB);
        vecB.clear();

        if(!vecC.empty())
        {
            rgn.push_back(y);
            for(int x : vecC)
                rgn.push_back(x);
            rgn.push_back(RGN_STOP);
        }
    }
    rgn.push_back(RGN_STOP);

    size_t sz = sizeof(int16_t) * rgn.size() + 10;
    RgnHandle rgnH = rgn.release();
    (*rgnH)->rgnBBox = rect;
    (*rgnH)->rgnSize = sz;
    return rgnH;
}

#else

#define TERM (*ip++ = RGN_STOP_X)

#define ADD4(y, x1, x2) \
    (*ip++ = CW_RAW((y)), *ip++ = CW_RAW((x1)), *ip++ = CW_RAW((x2)), TERM)

#define ADD6(y, x1, x2, x3, x4)                                       \
    (*ip++ = CW_RAW((y)), *ip++ = CW_RAW((x1)), *ip++ = CW_RAW((x2)), \
     *ip++ = CW_RAW((x3)), *ip++ = CW_RAW((x4)), TERM)

RgnHandle Executor::ROMlib_circrgn(const Rect *r) /* INTERNAL */
{
    RgnHandle rh;
    INTEGER x, y, temp; /* some variables need to be longs */
    INTEGER d, e, ny, oy, nx, ox, savex, rad;
    INTEGER scalex, scaley;

    INTEGER dh, dv;
    Size maxsize;
    INTEGER top, bottom, centl, centr, centt, left, right;
    INTEGER *ip, *op, *ip2, *ep;
    INTEGER first;
    long long_region_size; /* so we can test for overflow */

#if !defined(LETGCCWAIL)
    ox = 0;
#endif /* LETGCCWAIL */

    top = r->top;
    bottom = r->bottom;
    dv = bottom - top;
    dh = (right = r->right) - (left = r->left);

    maxsize = 10 + (6 * dv + 1) * sizeof(INTEGER);
    rh = (RgnHandle)NewHandle(maxsize);
    (*rh)->rgnBBox = *r;

    if(false && dh == dv && dh < 10)
    { /* do small ones by hand */
        ip = (INTEGER *)*rh + 5;
        if(dh >= 4)
        {
            if(dh < 7)
            {
                ADD4(top, left + 1, right - 1);
                ADD6(top + 1, left, left + 1, right - 1, right);
                ADD6(bottom - 1, left, left + 1, right - 1, right);
                ADD4(bottom, left + 1, right - 1);
                TERM;
            }
            else
            {
                ADD4(top, left + 2, right - 2);
                ADD6(top + 1, left + 1, left + 2, right - 2, right - 1);
                ADD6(top + 2, left, left + 1, right - 1, right);
                ADD6(bottom - 2, left, left + 1, right - 1, right);
                ADD6(bottom - 1, left + 1, left + 2, right - 2, right - 1);
                ADD4(bottom, left + 2, right - 2);
                TERM;
            }
        }
        (*rh)->rgnSize = (char *)ip - (char *)*rh;
        SetHandleSize((Handle)rh, (Size)(*rh)->rgnSize);
        /*-->*/ return rh;
    }

    if(dh > dv)
    {
        scalex = false;
        scaley = true;
        rad = dh;
    }
    else if(dv > dh)
    {
        scalex = true;
        scaley = false;
        rad = dv;
    }
    else
    {
        scalex = false;
        scaley = false;
        rad = dv;
    }

    centl = r->left + dh / 2;
    centr = r->left + (dh + 1) / 2;
    centt = top + dv / 2;
    first = true;

    op = (INTEGER *)*rh + 5;
    x = 0;
    y = rad;
    oy = centt - top;
    d = 3 - 2 * rad;
    e = 3 - 4 * rad;
    while(y >= 0)
    {
        if(d < 0)
        {
            temp = 4 * x;
            d += temp + 6;
            e += temp + 4;
            x++;
        }
        else
        {
            savex = x;
            if(e > 0)
            {
                temp = 4 * y;
                d -= temp - 4;
                e += -temp + 6;
            }
            else
            {
                d += (temp = 4 * (x - y) + 10);
                e += temp;
                x++;
            }
            y--;
            if(scaley)
                ny = ((LONGINT)dv * y / dh) / 2;
            else
                ny = y / 2;
            if(ny != oy)
            {
                if(scalex)
                    nx = ((LONGINT)dh * savex / dv) / 2;
                else
                    nx = savex / 2;
                if(first)
                {
                    *op++ = CW_RAW(top);
                    *op++ = CW_RAW(centl - nx);
                    *op++ = CW_RAW(centr + nx);
                    *op++ = RGN_STOP_X;
                    ox = nx;
                    first = false;
                }
                else
                {
                    if(nx != ox)
                    {
                        *op++ = CW_RAW(centt - oy);
                        *op++ = CW_RAW(centl - nx);
                        *op++ = CW_RAW(centl - ox);
                        *op++ = CW_RAW(centr + ox);
                        *op++ = CW_RAW(centr + nx);
                        *op++ = RGN_STOP_X;
                        ox = nx;
                    }
                }
                oy = ny;
            }
        }
    }
    ip = op - 1;
    ep = (INTEGER *)*rh + 4;
    while(ip != ep)
    {
        ip -= 4;
        while(ip != ep && *ip != RGN_STOP_X)
            ip -= 2;
        ip2 = ip + 1;
        *op++ = CW_RAW(bottom - (CW_RAW(*ip2++) - top));
        while((*op++ = *ip2++) != RGN_STOP_X)
            ;
    }
    *op++ = RGN_STOP_X;

    long_region_size = sizeof(INTEGER) * (op - (INTEGER *)*rh);
    if(long_region_size >= 32768) /* test for overflow */
        SetEmptyRgn(rh);
    else
    {
        (*rh)->rgnSize = sizeof(INTEGER) * (op - (INTEGER *)*rh);
        SetHandleSize((Handle)rh, (Size)(*rh)->rgnSize);
    }

    return rh;
}
#endif

static RgnHandle roundRectRgn(const Rect* r, int16_t width, int16_t height)
{
    /*
    * TODO:  speed up this code... it is ridiculously slow
    */

    int16_t ovaldx = r->right - r->left - width;
    int16_t ovaldy = r->bottom - r->top - height;
    int16_t rectdx = r->right - r->left - width / 2;
    int16_t rectdy = r->bottom - r->top - height / 2;

    RgnHandle rh = NewRgn();    
    RectRgn(rh, r);
    
    Rect tempr;
    SetRect(&tempr, r->left, r->top,
            r->left + width, r->top + height);
    RgnHandle oval = ROMlib_circrgn(&tempr);

    SetRect(&tempr, r->left, r->top,
            r->left + width / 2, r->top + height / 2);
    
    RgnHandle smallr = NewRgn();
    RectRgn(smallr, &tempr);

    RgnHandle corner = NewRgn();
    DiffRgn(smallr, oval, corner);
    DiffRgn(rh, corner, rh);

    OffsetRgn(oval, ovaldx, 0);
    OffsetRgn(smallr, rectdx, 0);
    DiffRgn(smallr, oval, corner);
    DiffRgn(rh, corner, rh);

    OffsetRgn(oval, 0, ovaldy);
    OffsetRgn(smallr, 0, rectdy);
    DiffRgn(smallr, oval, corner);
    DiffRgn(rh, corner, rh);

    OffsetRgn(oval, -ovaldx, 0);
    OffsetRgn(smallr, -rectdx, 0);
    DiffRgn(smallr, oval, corner);
    DiffRgn(rh, corner, rh);

    DisposeRgn(smallr);
    DisposeRgn(corner);
    DisposeRgn(oval);
    
    return rh;
}

void Executor::C_StdRRect(GrafVerb verb, const Rect *r, INTEGER width, INTEGER height)
{
    PAUSEDECL;

    if(qdGlobals().thePort->picSave)
    {
        GUEST<Point> p = { height, width };
        ROMlib_drawingverbrectovalpicupdate(verb, r, &p);
        PICOP(OP_frameRRect + (int)verb);
        PICWRITE(r, sizeof(*r));
    }

    if(PORT_PEN_VIS(qdGlobals().thePort) < 0
       && (!PORT_REGION_SAVE(qdGlobals().thePort) || verb != frame))
        /*-->*/ return;

    PAUSERECORDING;
    if(width < 4 && height < 4)
        StdRect(verb, r);
    else
    {
        RgnHandle rh = roundRectRgn(r, width, height);
        if(verb == frame)
        {
            if(RgnHandle rsave = (RgnHandle)PORT_REGION_SAVE(qdGlobals().thePort))
                XorRgn(rh, rsave, rsave);

            Rect inner = *r;
            Point penSize = PORT_PEN_SIZE(qdGlobals().thePort);
            InsetRect(&inner, penSize.h, penSize.v);
            RgnHandle innerRgn = roundRectRgn(&inner, width - 2 * penSize.h, height - 2 * penSize.v);
            DiffRgn(rh, innerRgn, rh);
            DisposeRgn(innerRgn);
            StdRgn(paint, rh);
        }
        else
            StdRgn(verb, rh);
        DisposeRgn(rh);
    }
    RESUMERECORDING;
}
