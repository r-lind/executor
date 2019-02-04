/* Copyright 1986, 1989, 1990, 1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in ResourceMgr.h (DO NOT DELETE THIS LINE) */

#include <base/common.h>
#include <ResourceMgr.h>
#include <FileMgr.h>
#include <res/resource.h>
#include <MemoryMgr.h>
#include <OSUtil.h>
#include <mman/mman.h>

#include <ctl/ctl.h>
#include <list/list.h>
#include <menu/menu.h>
#include <wind/wind.h>
#include <sound/soundopts.h>
#include <file/file.h>
#include <rsys/aboutbox.h>
#include <rsys/stdfile.h> /* for unixmount */

#include <ctype.h>

using namespace Executor;

static INTEGER countmapresources(resmaphand, ResType);
static Handle getindmapresource(resmaphand, ResType, INTEGER *);
static Handle getnamedmapresource(resmaphand, ResType, StringPtr);

void Executor::C_SetResLoad(BOOLEAN load)
{
    LM(ResLoad) = load;
}

static INTEGER countmapresources(resmaphand map, ResType typ)
{
    INTEGER i, retval;
    typref *tr;

    retval = 0;
    WALKTR(map, i, tr)
    if(tr->rtyp == typ)
    {
        retval += 1 + tr->nres;
        break;
    }
    EWALKTR(tr)
    return (retval);
}

INTEGER Executor::C_CountResources(ResType typ)
{
    resmaphand map;
    INTEGER n;

    ROMlib_setreserr(noErr);
    n = 0;
    WALKMAPTOP(map)
    n += countmapresources(map, typ);
    EWALKMAP()
    return (n);
}

INTEGER Executor::C_Count1Resources(ResType typ) /* IMIV-15 */
{
    resmaphand map;

    map = ROMlib_rntohandl(LM(CurMap), (Handle *)0);
    if(!map)
    {
        ROMlib_setreserr(resFNotFound);
        /*-->*/ return 0;
    }
    return (countmapresources(map, typ));
}

static Handle getindmapresource(resmaphand map, ResType typ, INTEGER *indx)
{
    INTEGER i, j, nr;
    typref *tr;
    resref *rr;

    WALKTR(map, i, tr)
    if(tr->rtyp == typ)
    {
        nr = tr->nres + 1;
        WALKRR(map, tr, j, rr)
        if(--*indx == 0)
            /*-->*/ return ROMlib_mgetres(map, rr);
        EWALKRR(rr)
    }
    EWALKTR(tr)
    return ((Handle)0);
}

Handle Executor::C_GetIndResource(ResType typ, INTEGER indx)
{
    resmaphand map;
    Handle retval;

    if(indx <= 0)
    {
        ROMlib_setreserr(resNotFound);
        /*-->*/ return 0;
    }
    WALKMAPTOP(map)
    retval = getindmapresource(map, typ, &indx);
    if(retval)
    {
        ROMlib_setreserr(noErr);
        /*-->*/ return retval;
    }
    EWALKMAP()
    ROMlib_setreserr(resNotFound);
    return 0;
}

Handle Executor::C_Get1IndResource(ResType typ, INTEGER i) /* IMIV-15 */
{
    resmaphand map;
    Handle retval;
    INTEGER tempi;

    map = ROMlib_rntohandl(LM(CurMap), (Handle *)0);
    if(!map)
    {
        ROMlib_setreserr(resFNotFound);
        /*-->*/ return 0;
    }
    tempi = i;
    if(i <= 0 || !(retval = getindmapresource(map, typ, &tempi)))
    {
        ROMlib_setreserr(resNotFound);
        /*-->*/ return 0;
    }
    else
    {
        ROMlib_setreserr(noErr);
        return retval;
    }
}

/* ROMlib_maptypidtop: given a map, typ and an id,
				    *ptr is filled in appropriately */

OSErr Executor::ROMlib_maptypidtop(resmaphand map, ResType typ, INTEGER id,
                                   resref **ptr) /* INTERNAL */
{
    INTEGER i, j;
    typref *tr;
    resref *rr;

    WALKTR(map, i, tr)
    if(tr->rtyp == typ)
    {
        WALKRR(map, tr, j, rr)
        if(rr->rid == id)
        {
            *ptr = rr;
            return (noErr);
        }
        EWALKRR(rr)
    }
    EWALKTR(tr)
    return resNotFound;
}

/* ROMlib_typidtop:  given a type and an id,
		ROMlib_typidtop fills in *pth and *ptr
                with a resmap handle and a resref pointer */

OSErr Executor::ROMlib_typidtop(ResType typ, INTEGER id, resmaphand *pth,
                                resref **ptr) /* INTERNAL */
{
    resmaphand map;

    WALKMAPCUR(map)
    if(ROMlib_maptypidtop(map, typ, id, ptr) == noErr)
    {
        *pth = map;
        return noErr;
    }
    EWALKMAP()

    warn_resource_not_found(typ, id);
    return resNotFound;
}

#define MAELSTROM_HACK

#define NUM_ROMLIB_DEFS 10

#if !defined(MAELSTROM_HACK)
static GUEST<LONGINT> ROMlib_defs[NUM_ROMLIB_DEFS];
#endif

/*
 * Don't change the order of the routines below.
 * We have made stubs of the form:
 *
 *	0x20780058	movel	0x58:w,		a0
 *	0x206800nn	movel	a0@(nn),	a0
 *	0x4ED0		jmp			a0@
 * 
 * nn is the offset into ROMlib_defs (0, 4, 8, ...)
 */

static BOOLEAN acceptable(unsigned long addr)
{
    return !(islower(addr & 0xFF) || islower((addr >> 8) & 0xFF) || islower((addr >> 16) & 0xFF) || islower((addr >> 24) & 0xFF));
}

static void ROMlib_init_xdefs(void)
{
#if !defined(MAELSTROM_HACK)
    ROMlib_defs[0] = guest_cast<LONGINT>(&cdef0);
    ROMlib_defs[1] = guest_cast<LONGINT>(&cdef16);
    ROMlib_defs[2] = guest_cast<LONGINT>(&wdef0);
    ROMlib_defs[3] = guest_cast<LONGINT>(&wdef16);
    ROMlib_defs[4] = guest_cast<LONGINT>(&mdef0);
    ROMlib_defs[5] = guest_cast<LONGINT>(&ldef0);
    ROMlib_defs[6] = guest_cast<LONGINT>(&mbdf0);
    ROMlib_defs[7] = guest_cast<LONGINT>(&snth5);
    ROMlib_defs[8] = guest_cast<LONGINT>(&unixmount);
    ROMlib_defs[9] = guest_cast<LONGINT>(&cdef1008);

    *(LONGINT *)SYN68K_TO_US(0x58) = (LONGINT)ROMlib_defs;
#else
    GUEST<THz> save_zone;
    Handle oldhandle, newhandle;
    long timeout;
    GUEST<LONGINT> *ROMlib_defs;

    save_zone = LM(TheZone);

    LM(TheZone) = LM(SysZone);
    newhandle = 0;
    timeout = 64000;
    do
    {
        oldhandle = newhandle;
        newhandle = NewHandle(NUM_ROMLIB_DEFS * sizeof(ROMlib_defs[0]));
        if(oldhandle)
            DisposeHandle(oldhandle);
    } while(!acceptable((*newhandle).raw()) && --timeout);
#if !defined(NDEBUG)
    if(!timeout)
        warning_unexpected("Maelstrom hack didn't work");
#endif
    HLock(newhandle);
    ROMlib_defs = (GUEST<LONGINT> *)*newhandle;
    ROMlib_defs[0] = guest_cast<LONGINT>(&cdef0);
    ROMlib_defs[1] = guest_cast<LONGINT>(&cdef16);
    ROMlib_defs[2] = guest_cast<LONGINT>(&wdef0);
    ROMlib_defs[3] = guest_cast<LONGINT>(&wdef16);
    ROMlib_defs[4] = guest_cast<LONGINT>(&mdef0);
    ROMlib_defs[5] = guest_cast<LONGINT>(&ldef0);
    ROMlib_defs[6] = guest_cast<LONGINT>(&mbdf0);
    ROMlib_defs[7] = guest_cast<LONGINT>(&snth5);
    ROMlib_defs[8] = guest_cast<LONGINT>(&unixmount);
    ROMlib_defs[9] = guest_cast<LONGINT>(&cdef1008);
    *(LONGINT *)SYN68K_TO_US(0x58) = (LONGINT)(*newhandle).raw(); // ### use standard low mem access method
    LM(TheZone) = save_zone;
#endif
}

typedef struct
{
    ResType type;
    INTEGER id;
} pseudo_rom_entry_t;

static Handle
pseudo_get_rom_resource(ResType typ, INTEGER id)
{
    Handle retval;
    int i;
    static pseudo_rom_entry_t pseudo_rom_table[] = {
        { FOURCC('F', 'O', 'N', 'D'), 3 }
    };

    for(i = 0; (i < (int)NELEM(pseudo_rom_table) && (pseudo_rom_table[i].type != typ
                                                     || pseudo_rom_table[i].id != id));
        ++i)
        ;
    if(i < (int)NELEM(pseudo_rom_table))
    {
        GUEST<INTEGER> save_map;

        save_map = LM(CurMap);
        LM(CurMap) = LM(SysMap);
        retval = C_Get1Resource(typ, id);
        LM(CurMap) = save_map;
    }
    else
        retval = 0;
    return retval;
}

#if defined(ULTIMA_III_HACK)
bool Executor::ROMlib_ultima_iii_hack;
#endif

Handle Executor::C_GetResource(ResType typ, INTEGER id)
{
    resmaphand map;
    resref *rr;
    static int beenhere = 0;
    Handle retval;

    if(!beenhere)
    {
        beenhere = 1;
        ROMlib_init_xdefs();
    }

    EM_D0 = 0; // apparently, somebody is relying on D0 being reset to 0 on exit from GetResource...
               // (this used to be in emustubs.cpp)

    retval = pseudo_get_rom_resource(typ, id);
    if(retval)
        /*-->*/ return retval;

#define BOBSEYESHACK
#if defined(BOBSEYESHACK)
    /*
 * This gets around a bug in Bob's Eyes that references stray memory.
 * I'm trying to find the author of the program and get him to fix it.
 *  	--Cliff Sat Sep  3 08:02:51 MDT 1994
 */
    if(typ == TICK("rAnd") && id == 128)
        return 0;
#endif

    // "Icky hack"
    // Truer words were never spoken.
    // Instead of looking for 'PACK' resources, which aren't there
    // in Executor, return a completely unrelated ALRT instead.
    // For some reason, if this is not done, ResEdit's Pixel editor fails
    // with memFullErr (-108).
    // My current theory is that some programs are checking
    // GetResource('PACK', ...) to check whether some packages can be 
    // loaded successfully.
    // The hack had to be disabled for id 1 to support ResEdit's use of PACK 1.
    // TODO: Verify theory and add some 'PACK' resources to system file instead of this hack.
    switch(typ)
    { /* fake out code resources */
#define ICKYHACK
#if defined(ICKYHACK)
        case FOURCC('P', 'A', 'C', 'K'):
            if(id != 1)
                return GetResource(TICK("ALRT"), -3995);
#endif /* ICKYHACK */
    }

    ROMlib_setreserr(ROMlib_typidtop(typ, id, &map, &rr));
    if(LM(ResErr) == resNotFound)
    {
        ROMlib_setreserr(noErr);
        retval = 0; /* IMIV */
    }
    else
    {
        if(LM(ResErr) == noErr)
            retval = ROMlib_mgetres(map, rr);
        else
            retval = 0;
    }
#if defined(ULTIMA_III_HACK)
    if(ROMlib_ultima_iii_hack && typ == TICK("PREF") && retval)
    {
        *((char *)*retval + 1) = 0; /* turn off Music prefs */
    }
#endif
    return retval;
}

Handle Executor::C_Get1Resource(ResType typ, INTEGER id) /* IMIV-16 */
{
    Handle retval;
    resmaphand map;
    resref *rr;

    map = ROMlib_rntohandl(LM(CurMap), nullptr);
    if(!map)
    {
        ROMlib_setreserr(resFNotFound);
        return nullptr;
    }
    ROMlib_setreserr(ROMlib_maptypidtop(map, typ, id, &rr));
    if(LM(ResErr) == noErr)
        retval = ROMlib_mgetres(map, rr);
    else
    {
        warn_resource_not_found(typ, id);
        retval = nullptr;
    }
    if(LM(ResErr) == resNotFound)
        /* IMIV */
        LM(ResErr) = noErr;
    return retval;
}

static Handle getnamedmapresource(resmaphand map, ResType typ, StringPtr nam)
{
    INTEGER i, j;
    typref *tr;
    resref *rr;

    WALKTR(map, i, tr)
    if(tr->rtyp == typ)
    {
        WALKRR(map, tr, j, rr)
        if(rr->noff != -1 && EqualString((StringPtr)(
                                                 (char *)*map + (*map)->namoff + rr->noff),
                                             (StringPtr)nam, 0, 1))
            /*-->*/ return ROMlib_mgetres(map, rr);
        EWALKRR(rr)
    }
    EWALKTR(tr)
    ROMlib_setreserr(resNotFound);
    return (0);
}

Handle Executor::C_GetNamedResource(ResType typ, StringPtr nam)
{
    Handle retval;

    retval = nullptr;
    if(EqualString(nam, about_box_menu_name_pstr, true, true))
    {
        static Handle phoney_hand;

        if(!phoney_hand)
            phoney_hand = NewHandleSys(0);
        retval = phoney_hand;
    }
    else
    {
        resmaphand map;

        WALKMAPCUR(map)
        if((retval = getnamedmapresource(map, typ, nam)))
        {
            ROMlib_setreserr(noErr);
            goto DONE;
        }
        EWALKMAP()
        warn_resource_not_found_name(typ, nam);
        ROMlib_setreserr(resNotFound);
    }
DONE:
    return retval;
}

Handle Executor::C_Get1NamedResource(ResType typ, StringPtr s) /* IMIV-16 */
{
    resmaphand map;

    map = ROMlib_rntohandl(LM(CurMap), (Handle *)0);
    if(!map)
    {
        ROMlib_setreserr(resFNotFound);
        /*-->*/ return 0;
    }
    return (getnamedmapresource(map, typ, s));
}

void Executor::C_LoadResource(Handle volatile res)
{
    resmaphand map;
    typref *tr;
    resref *rr;
    GUEST<int16_t> savemap;

    volatile LONGINT savea0d0[2];
    savea0d0[0] = EM_D0;
    savea0d0[1] = EM_A0;

    if(!res)
    {
        ROMlib_setreserr(nilHandleErr);
        return;
    }

    if(*res)
    {
        ROMlib_setreserr(noErr);
    }
    else
    {
        savemap = LM(CurMap);
        LM(CurMap) = ((resmap *)*LM(TopMapHndl))->resfn;
        ROMlib_setreserr(ROMlib_findres(res, &map, &tr, &rr));
        LM(CurMap) = savemap;
        if(LM(ResErr) == noErr)
        {
            BOOLEAN save_resload;

            save_resload = LM(ResLoad);
            SetResLoad(true);
            ROMlib_mgetres(map, rr);
            LM(ResLoad) = save_resload;
        }
    }

    EM_D0 = savea0d0[0];
    EM_A0 = savea0d0[1];
}

void Executor::C_ReleaseResource(Handle res)
{
    resmaphand map;
    typref *tr;
    resref *rr;
    Handle h;

    if(ROMlib_setreserr(ROMlib_findres(res, &map, &tr, &rr)))
        /*-->*/ return;
    if(rr->ratr & resChanged)
        ROMlib_wr(map, rr);
    if(LM(ResErr) != noErr || !rr->rhand)
        return;
    h = rr->rhand;
    if(*h)
        HClrRBit(h);
    DisposeHandle(h);
    rr->rhand = 0;
}

void Executor::C_DetachResource(Handle res)
{
    resmaphand map;
    typref *tr;
    resref *rr;
    Handle h;

    if(ROMlib_setreserr(ROMlib_findres(res, &map, &tr, &rr)))
        /*-->*/ return;
    if(LM(ResErr) != noErr || !(h = (Handle)rr->rhand))
        return;
    if(rr->ratr & resChanged)
    {
        ROMlib_setreserr(resAttrErr); /* IV-18 */
        return;
    }
    rr->rhand = 0;
    if(*h)
        HClrRBit(h);
}
