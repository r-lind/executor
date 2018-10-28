/* Copyright 1986, 1989, 1990 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in ResourceMgr.h (DO NOT DELETE THIS LINE) */

#include "rsys/common.h"
#include "ResourceMgr.h"
#include "FileMgr.h"
#include "MemoryMgr.h"
#include "ToolboxUtil.h"
#include "rsys/resource.h"
#include "rsys/glue.h"
#include "rsys/mman.h"
#include "rsys/file.h"

using namespace Executor;

typedef res_sorttype_t *sorttypeptr;

typedef GUEST<sorttypeptr> *sorttypehand;

static LONGINT addtype(resmaphand, ResType);
static LONGINT addname(resmaphand, StringPtr);
static OSErr writemap(resmaphand);
static void fillst(sorttypehand, resref *, resref *);
static void getdat(INTEGER, LONGINT, LONGINT, Handle *);
static void putdat(INTEGER, LONGINT, LONGINT *, Handle);
static LONGINT walkst(res_sorttype_t *, res_sorttype_t *, INTEGER, LONGINT);
static void compactdata(resmaphand);

void Executor::C_SetResInfo(Handle res, INTEGER id, StringPtr name)
{
    resmaphand map;
    typref *tr;
    resref *rr;
    char *sp;
    INTEGER sl;
    OSErr err;

    ROMlib_setreserr(ROMlib_findres(res, &map, &tr, &rr));
    if(LM(ResErr) != noErr)
        return;
    if(rr->ratr & resProtected)
    {
        ROMlib_setreserr(resAttrErr); /* IV-18 */
        return;
    }
    rr->rid = id;
    if(name)
    {
        sl = U(name[0]);
        if(U(*(sp = (char *)STARH(map) + Hx(map, namoff) + rr->noff)) < sl || rr->noff == -1)
        {
            SetHandleSize((Handle)map, Hx(map, rh.maplen) + sl + 1);
            err = MemError();
            if(ROMlib_setreserr(err))
                return;
            rr->noff = Hx(map, rh.maplen) - Hx(map, namoff);
            HxX(map, rh.maplen) = Hx(map, rh.maplen) + sl + 1;
            sp = (char *)STARH(map) + Hx(map, namoff) + rr->noff;
            warning_unimplemented("we leak space here");
        }
        str255assign(sp, name);
    }
}

void Executor::C_SetResAttrs(Handle res, INTEGER attrs)
{
    resmaphand map;
    typref *tr;
    resref *rr;

    ROMlib_setreserr(ROMlib_findres(res, &map, &tr, &rr));
    if(LM(ResErr) != noErr)
        return;
    rr->ratr = attrs;
}

void Executor::C_ChangedResource(Handle res)
{
    resmaphand map;
    typref *tr;
    resref *rr;
    Size oldsize, newsize;

    ROMlib_setreserr(ROMlib_findres(res, &map, &tr, &rr));
    if(LM(ResErr) != noErr)
        return;
    if(rr->ratr & resProtected)
    {
        ROMlib_setreserr(resAttrErr); /* IV-18 */
        return;
    }
    rr->ratr |= resChanged;
    HxX(map, resfatr) |= mapChanged;
    if(rr->doff[0] != 0xff || rr->doff[1] != 0xff || rr->doff[2] != 0xff)
    {
        oldsize = ROMlib_SizeResource(res, false);
        newsize = GetHandleSize((Handle)rr->rhand);
        if(newsize > oldsize)
        {
            HxX(map, resfatr) |= mapCompact;
            rr->doff[0] = rr->doff[1] = rr->doff[2] = 0xff;
        }
    }
}

static LONGINT addtype(resmaphand map, ResType typ)
{
    typref *tr, t;
    INTEGER i;
    LONGINT off;

    WALKTR(map, i, tr)
    if(tr->rtyp == typ)
        return ((LONGINT)((char *)tr - (char *)STARH(map)));
    EWALKTR(tr)
    ROMlib_invalar();
    t.rtyp = typ;
    t.nres = -1;
    t.rloff = NAMEOFF(map) - TYPEOFF(map);
    off = (LONGINT)((char *)tr - (char *)STARH(map));
    Munger((Handle)map, off, (Ptr) "", (LONGINT)0, (Ptr)&t, sizeof(t));
    NUMTMINUS1X(map) = NUMTMINUS1(map) + 1;
    MAPLENX(map) = MAPLEN(map) + sizeof(t);
    NAMEOFFX(map) = NAMEOFF(map) + sizeof(t);
    WALKTR(map, i, tr)
    tr->rloff = tr->rloff + sizeof(t);
    EWALKTR(tr)
    return (off);
}

static LONGINT addname(resmaphand map, StringPtr name)
{
    LONGINT retval;

    if(!name || !name[0])
        retval = -1;
    else
    {
        LONGINT namelen;

        namelen = U(name[0]) + 1;

        Munger((Handle)map, MAPLEN(map), (Ptr) "", (LONGINT)0, (Ptr)name,
               namelen);
        retval = MAPLEN(map) - NAMEOFF(map);
        MAPLENX(map) = MAPLEN(map) + namelen;
    }
    return retval;
}

void Executor::C_AddResource(Handle data, ResType typ, INTEGER id,
                             StringPtr name)
{
    LONGINT toff, roff;
    typref *tr, *tr2;
    resref r, *rr;
    resmaphand map;
    INTEGER i, j;

    if(!data)
    {
        ROMlib_setreserr(addResFailed);
        return;
    }
    ROMlib_setreserr(noErr);
    map = ROMlib_rntohandl(LM(CurMap), (Handle *)0);
    if(!map)
    {
        ROMlib_setreserr(resFNotFound);
        return;
    }
    if(ROMlib_findmapres(map, data, &tr, &rr) == noErr)
    {
        ROMlib_setreserr(addResFailed);
        /*-->*/ return;
    }
    ROMlib_setreserr(noErr);
    toff = addtype(map, typ);
    r.rid = id;
    r.noff = addname(map, name);
    r.ratr = resChanged;
    r.doff[0] = r.doff[1] = r.doff[2] = 0xff;
    HxX(map, resfatr) |= mapChanged;
    r.rhand = data;
    HSetRBit(data);
    tr = (typref *)((char *)STARH(map) + toff);
#if 1
    WALKRR(map, tr, j, rr)
    if(rr->rid > id)
        break;
    EWALKRR(rr)
#else /* 0 */
    rr = (resref *)((char *)STARH(map) + Hx(map, typoff) + tr->rloff);
#endif /* 0 */
    roff = (LONGINT)((char *)rr - (char *)STARH(map));
    /* roff is from the beginning of the map */
    Munger((Handle)map, roff, (Ptr) "", (LONGINT)0, (Ptr)&r,
           sizeof(resref));
    roff -= TYPEOFF(map);
    /* roff is from the beginning of the typelist */
    MAPLENX(map) = MAPLEN(map) + sizeof(resref);
    NAMEOFFX(map) = NAMEOFF(map) + sizeof(resref);
    tr2 = (typref *)((char *)STARH(map) + toff);
    tr2->nres = tr2->nres + 1;
    WALKTR(map, i, tr)
    if(tr->rloff >= roff && tr != tr2)
        /*  if (tr->rloff > roff)  would probably work, but I'm chicken
								      -- ctm */
        tr->rloff = tr->rloff + sizeof(resref);
    EWALKTR(tr)
}

void Executor::C_RemoveResource(Handle res)
{
    resmaphand map;
    typref *tr;
    resref *rr;
    LONGINT troff, rroff, nmoff, nlen;
    INTEGER i, j, tloss;

#if !defined(LETGCCWAIL)
    nlen = 0;
#endif /* LETGCCWAIL */

    if(!res)
    {
        ROMlib_setreserr(rmvResFailed);
        /*-->*/ return;
    }
    map = ROMlib_rntohandl(LM(CurMap), (Handle *)0);
    if(!map)
    {
        ROMlib_setreserr(resFNotFound);
        /*-->*/ return;
    }
    ROMlib_setreserr(ROMlib_findmapres(map, res, &tr, &rr));
    if(LM(ResErr) != noErr)
        /*-->*/ return;
    if(rr->ratr & resProtected)
    {
        ROMlib_setreserr(rmvResFailed);
        /*-->*/ return;
    }
    HSetState(res, HGetState(res) & ~RSRCBIT);
    troff = (char *)tr - (char *)STARH(map);
    rroff = (char *)rr - (char *)STARH(map);
    if(rr->noff != -1)
    {
        nmoff = rr->noff + NAMEOFF(map);
        nlen = U(*((char *)STARH(map) + nmoff)) + 1;
        Munger((Handle)map, nmoff, (Ptr)0, nlen, (Ptr) "", (LONGINT)0);
        nmoff -= NAMEOFF(map);
        MAPLENX(map) = MAPLEN(map) - nlen;
    }
    else
        nmoff = 0x7fff;
    HxX(map, resfatr) |= mapChanged | mapCompact;
    Munger((Handle)map, rroff, (Ptr)0, (LONGINT)sizeof(resref),
           (Ptr) "", (LONGINT)0);
    rroff -= TYPEOFF(map);
    MAPLENX(map) = MAPLEN(map) - sizeof(resref);
    NAMEOFFX(map) = NAMEOFF(map) - sizeof(resref);
    tr = (typref *)((char *)STARH(map) + troff);
    tr->nres = tr->nres - 1;
    if(tr->nres == -1)
    {
        ROMlib_invalar();
        NUMTMINUS1X(map) = NUMTMINUS1(map) - 1;
        Munger((Handle)map, troff, (Ptr)0, (LONGINT)sizeof(typref), (Ptr) "",
               (LONGINT)0);
        tloss = sizeof(typref);
        MAPLENX(map) = MAPLEN(map) - sizeof(typref);
        NAMEOFFX(map) = NAMEOFF(map) - sizeof(typref);
    }
    else
        tloss = 0;
    WALKTR(map, i, tr)
    if(tr->rloff > rroff)
        tr->rloff = tr->rloff - sizeof(resref);
    tr->rloff = tr->rloff - tloss;
    WALKRR(map, tr, j, rr)
    if(rr->noff > nmoff)
        rr->noff = rr->noff - nlen;
    EWALKRR(rr)
    EWALKTR(tr)
}

/* writemap:  given a map, writemap writes it out */

static OSErr writemap(resmaphand map)
{
    OSErr terr;
    LONGINT lc;

    terr = SetFPos(Hx(map, resfn), fsFromStart, 0L);
    if(terr != noErr)
        return (terr);
    lc = sizeof(reshead);
    terr = FSWriteAll(Hx(map, resfn), guestref(lc), (Ptr) & (HxX(map, rh)));
    if(terr != noErr)
        return (terr);
    terr = SetFPos(Hx(map, resfn), fsFromStart, Hx(map, rh.rmapoff));
    if(terr != noErr)
        return (terr);
    lc = Hx(map, rh.maplen);
    terr = FSWriteAll(Hx(map, resfn), guestref(lc), (Ptr)STARH(map));
    if(terr == noErr)
        HxX(map, resfatr) &= ~(mapChanged);
    return (terr);
}

/* ROMlib_wr: ROMlib_wr is the helper routine for WriteResource, the
	      resource file associated with map is updated to include
	      the resource pointed to by rr */

void Executor::ROMlib_wr(resmaphand map, resref *rr) /* INTERNAL */
{
    LONGINT rsize, newloc, lc;
    GUEST<LONGINT> swappedrsize;
    Handle res;

    res = (Handle)rr->rhand;
    if((rr->ratr & resChanged) && res)
    {
        rsize = GetHandleSize(res);
        if(rr->doff[0] == 0xFF && rr->doff[1] == 0xFF && rr->doff[2] == 0xFF)
        {
            newloc = Hx(map, rh.rmapoff) - Hx(map, rh.rdatoff);
            HxX(map, rh.rmapoff) = Hx(map, rh.rmapoff) + rsize + sizeof(LONGINT);
            HxX(map, rh.datlen) = Hx(map, rh.datlen) + rsize + sizeof(LONGINT);
            B3ASSIGN(rr->doff, newloc);
        }
        else
            newloc = B3TOLONG(rr->doff);
        ROMlib_setreserr(SetFPos(Hx(map, resfn), fsFromStart, Hx(map, rh.rdatoff) + newloc));
        if(LM(ResErr) != noErr)
            return;
        lc = sizeof(LONGINT);
        swappedrsize = rsize;
        ROMlib_setreserr(FSWriteAll(Hx(map, resfn), guestref(lc), (Ptr)&swappedrsize));
        if(LM(ResErr) != noErr)
            return;
        lc = rsize;
        ROMlib_setreserr(FSWriteAll(Hx(map, resfn), guestref(lc), STARH(res)));
        if(LM(ResErr) != noErr)
            return;
        rr->ratr &= ~resChanged;
    }
    else
        ROMlib_setreserr(noErr);
}

static void fillst(sorttypehand st, resref *rp, resref *rep)
{
    res_sorttype_t *end = STARH(st), *sp;
    LONGINT newoff;

    end++->diskoff = -1; /* real disk offsets are always > 0 */
    while(rp != rep)
    {
        newoff = B3TOLONG(rp->doff);
#if 0 /* compiler botch if we leave the next line in */
	gui_assert(newoff >= 0);
#endif
        for(sp = end - 1; newoff < sp->diskoff; sp--)
            ;
        sp++;
        BlockMoveData((Ptr)sp, (Ptr)(sp + 1), (Size)(end - sp) * sizeof(res_sorttype_t));
        sp->diskoff = newoff;
        sp->rrptr = rp;
        end++;
        rp++;
    }
}

static void getdat(INTEGER fn, LONGINT datoff, LONGINT doff, Handle *h)
{
    LONGINT size, lc;
    GUEST<LONGINT> size_s;

    gui_assert(datoff >= 0);
    gui_assert(doff >= 0);
    SetFPos(fn, fsFromStart, datoff + doff);
    lc = sizeof(LONGINT);
    FSReadAll(fn, guestref(lc), (Ptr)&size_s);
    size = size_s;
    gui_assert(size >= 0);
    *h = NewHandle(size);
    gui_assert(*h);
    HSetState(*h, RSRCBIT);
    lc = size;
    HLock(*h);
    FSReadAll(fn, guestref(lc), STARH(*h));
    gui_assert(lc == size);
    HUnlock(*h);
}

static void putdat(INTEGER fn, LONGINT datoff, LONGINT *doffp, Handle h)
{
    LONGINT size = GetHandleSize(h), lc;
    GUEST<LONGINT> swappedsize;

    gui_assert(size >= 0);
    gui_assert(datoff >= 0);
    gui_assert(*doffp >= 0);
    SetFPos(fn, fsFromStart, datoff + *doffp);
    lc = sizeof(LONGINT);
    swappedsize = size;
    FSWriteAll(fn, guestref(lc), (Ptr)&swappedsize);
    lc = size;
    HLock(h);
    FSWriteAll(fn, guestref(lc), STARH(h));
    HUnlock(h);
    *doffp += sizeof(LONGINT) + size;
}

static LONGINT walkst(res_sorttype_t *sp, res_sorttype_t *sep, INTEGER fn,
                      LONGINT datoff)
{
    LONGINT size;
    LONGINT doff;
    Handle h;
    LONGINT lc;

    doff = 0;
    while(sp != sep)
    {
        B3ASSIGN(sp->rrptr->doff, doff);
#if 0
        if (sp->rrptr->ratr & resChanged) {
            putdat(fn, datoff, &doff, (Handle) sp->rrptr->rhand);
        } else if (!sp->rrptr->rhand || !*(Handle)sp->rrptr->rhand) {
	    getdat(fn, datoff, sp->diskoff, &h);
	    putdat(fn, datoff, &doff,  h);
	    DisposeHandle(h);
	} else if (doff != sp->diskoff)
	    putdat(fn, datoff, &doff, (Handle) sp->rrptr->rhand);
        else
	    doff += GetHandleSize((Handle) sp->rrptr->rhand) +
							       sizeof(LONGINT);
#else
        if(sp->rrptr->ratr & resChanged)
        {
            putdat(fn, datoff, &doff, (Handle)sp->rrptr->rhand);
            sp->rrptr->ratr &= ~resChanged;
        }
        else if(doff == sp->diskoff)
        {
            /* read and advance */
            SetFPos(fn, fsFromStart, datoff + doff);
            lc = sizeof(LONGINT);
            GUEST<LONGINT> size_s;
            FSReadAll(fn, guestref(lc), (Ptr)&size_s);
            gui_assert(lc == sizeof(LONGINT));
            size = size_s;
            gui_assert(size >= 0);
            doff += size + sizeof(LONGINT);
        }
        else
        {
            getdat(fn, datoff, sp->diskoff, &h);
            putdat(fn, datoff, &doff, h);
            HSetState(h, HGetState(h) & ~RSRCBIT);
            DisposeHandle(h);
        }
#endif
        sp++;
    }
    return doff;
}

static void compactdata(resmaphand map)
{
    INTEGER nres;
    LONGINT resoff, datlen;
    sorttypehand st;
    resref *rr;
    INTEGER mapstate, ststate;

    resoff = TYPEOFF(map) + sizeof(INTEGER) + (NUMTMINUS1(map) + 1) * sizeof(typref);
    nres = (NAMEOFF(map) - resoff) / sizeof(resref);
    st = (sorttypehand)NewHandle((Size)sizeof(res_sorttype_t) * (nres + 1));
    mapstate = HGetState((Handle)map);
    HLock((Handle)map);
    rr = (resref *)((char *)STARH(map) + resoff);
    fillst(st, rr, rr + nres);
    ststate = HGetState((Handle)st);
    HLock((Handle)st);
    datlen = walkst(STARH(st) + 1, STARH(st) + nres + 1, Hx(map, resfn),
                    Hx(map, rh.rdatoff));
    HSetState((Handle)st, ststate);
    HSetState((Handle)map, mapstate);
    HxX(map, resfatr) |= mapChanged;
    HxX(map, resfatr) |= ~mapCompact;
    HxX(map, rh.rmapoff) = sizeof(reshead) + sizeof(rsrvrec) + datlen;
    HxX(map, rh.datlen) = datlen;
    DisposeHandle((Handle)st);
}

void Executor::C_UpdateResFile(INTEGER rn)
{
    resmaphand map;
    INTEGER i, j;
    typref *tr;
    resref *rr;
    BOOLEAN needtowalk;
    fcbrec *fp;
    OSErr err;
    IOParam iopb;

    map = ROMlib_rntohandl(rn, (Handle *)0);
    if(!map)
    {
        ROMlib_setreserr(resFNotFound);
        return;
    }
    ROMlib_setreserr(noErr);
    /*
 * NOTE:  This implementation looks more true to what IMI-125 implies than
 *	  the one (above) it replaces.
 */
    fp = PRNTOFPERR(Hx(map, resfn), &err);
    if(err == noErr && (fp->fcflags & fcwriteperm))
    {
        needtowalk = true;
        if(HxX(map, resfatr) & (mapCompact))
        {
            compactdata(map);
            needtowalk = false;
        }
        if(HxX(map, resfatr) & (mapChanged))
        {
            if(needtowalk)
            {
                WALKTANDR(map, i, tr, j, rr)
                ROMlib_wr(map, rr);
                if(LM(ResErr) != noErr)
                    return;
                EWALKTANDR(tr, rr)
            }
            ROMlib_setreserr(writemap(map));
            if(LM(ResErr) != noErr)
                return;
        }
        iopb.ioRefNum = HxX(map, resfn);
        PBFlushFile((ParmBlkPtr)&iopb, false);
    }
}

void Executor::C_WriteResource(Handle res)
{
    resmaphand map;
    typref *tr;
    resref *rr;

    ROMlib_setreserr(ROMlib_findres(res, &map, &tr, &rr));
    if(LM(ResErr) != noErr)
        return;
    ROMlib_wr(map, rr);
}

void Executor::C_SetResPurge(BOOLEAN install)
{
    /* NOP */
    ROMlib_setreserr(noErr);
}
