/* Copyright 1987 - 1996 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in ToolboxEvent.h (DO NOT DELETE THIS LINE) */

#include "rsys/common.h"
#include "QuickDraw.h"
#include "CQuickDraw.h"
#include "EventMgr.h"
#include "WindowMgr.h"
#include "OSUtil.h"
#include "ToolboxEvent.h"
#include "ToolboxUtil.h"
#include "FileMgr.h"
#include "MemoryMgr.h"
#include "DeskMgr.h"
#include "SysErr.h"
#include "BinaryDecimal.h"
#include "SegmentLdr.h"

#include "rsys/cquick.h"
#include "rsys/hfs.h"
#include "rsys/resource.h"
#include "rsys/futzwithdosdisks.h"
#include "rsys/prefs.h"
#include "rsys/options.h"
#include "rsys/prefpanel.h"
#include "rsys/sounddriver.h"
#include "rsys/segment.h"
#include "rsys/version.h"
#include "rsys/syncint.h"
#include "rsys/vbl.h"
#include "rsys/osutil.h"
#include "rsys/osevent.h"
#include "rsys/keyboard.h"
#include "rsys/parse.h"
#include "rsys/refresh.h"
#include "rsys/parseopt.h"
#include "rsys/vdriver.h"
#include "rsys/aboutbox.h"
#include "rsys/redrawscreen.h"
#include "rsys/toolevent.h"
#include "rsys/nextprint.h"
#include "rsys/scrap.h"
#include "rsys/time.h"
#include "rsys/menu.h"
#include <algorithm>

#if !defined(WIN32)
#include <sys/socket.h>
#endif

using namespace Executor;

/* #define	EVENTTRACE */

#if defined(EVENTTRACE)
LONGINT Executor::eventstate = 0;
#define TRACE(n) (eventstate = (n))
#else /* !defined(EVENTTRACE) */
#define TRACE(n)
#endif /* !defined(EVENTTRACE) */

static BOOLEAN ROMlib_alarmonmbar = false;

#define ALARMSICN -16385

BOOLEAN Executor::ROMlib_beepedonce = false;

static void ROMlib_togglealarm()
{
    Handle alarmh;
    static const unsigned char hard_coded_alarm[] = {
        0xFB, 0xBE, /* XXXXX XXX XXXXX  */
        0x84, 0x42, /* X    X   X    X  */
        0x89, 0x22, /* X   X  X  X   X  */
        0x91, 0x12, /* X  X   X   X  X  */
        0xA1, 0x0A, /* X X    X    X X  */
        0x41, 0x04, /*  X     X     X   */
        0x81, 0x02, /* X      X      X  */
        0x81, 0x02, /* X      X      X  */
        0x80, 0x82, /* X       X     X  */
        0x40, 0x44, /*  X       X   X   */
        0xA0, 0x0A, /* X X         X X  */
        0x90, 0x12, /* X  X       X  X  */
        0x88, 0x22, /* X   X     X   X  */
        0x84, 0x42, /* X    X   X    X  */
        0xFB, 0xBE, /* XXXXX XXX XXXXX  */
        0x00, 0x00, /*                  */
    };

    /* rectangle of alarm on the screen */
    static Rect screen_alarm_rect = { CWC(2), CWC(16), CWC(18), CWC(32) };

    BitMap src_alarm_bitmap = {
        /* the baseAddr field of the src_alarm bitmap will either be
	   the hard coded alarm, or the resource 'sicn' */
        nullptr,
        CWC(2),
        { CWC(0), CWC(0), CWC(16), CWC(16) }
    };

    static INTEGER alarm_bits[16];
    BitMap save_alarm_bitmap = {
        RM((Ptr)&alarm_bits[0]),
        CWC(2),
        { CWC(0), CWC(0), CWC(16), CWC(16) }
    };

    if(ROMlib_alarmonmbar)
    {
        CopyBits(&save_alarm_bitmap, (BitMap *)STARH(GD_PMAP(MR(LM(TheGDevice)))),
                 &save_alarm_bitmap.bounds, &screen_alarm_rect,
                 srcCopy, NULL);
        ROMlib_alarmonmbar = false;
    }
    else
    {
        if(ROMlib_shouldalarm())
        {
            if(!ROMlib_beepedonce)
            {
                SysBeep(5);
                ROMlib_beepedonce = true;
            }
            /* save the bits underneath */
            /* draw sicon -16385 up there */
            if((alarmh = ROMlib_getrestid(TICK("SICN"), ALARMSICN)))
                src_alarm_bitmap.baseAddr = *alarmh;
            else
                /* once again, we need to move the (Ptr) cast
		 inside the CL () because it confuses gcc */
                src_alarm_bitmap.baseAddr = RM((Ptr)hard_coded_alarm);

            /* save the screen to save_alarm_bitmap */
            CopyBits((BitMap *)STARH(GD_PMAP(MR(LM(TheGDevice)))), &save_alarm_bitmap,
                     &screen_alarm_rect, &save_alarm_bitmap.bounds,
                     srcCopy, NULL);
            /* and copy the new alarm to the screen */
            CopyBits(&src_alarm_bitmap, (BitMap *)STARH(GD_PMAP(MR(LM(TheGDevice)))),
                     &src_alarm_bitmap.bounds, &screen_alarm_rect,
                     srcCopy, NULL);
            ROMlib_alarmonmbar = true;
        }
    }
}

void Executor::ROMlib_alarmoffmbar()
{
    if(ROMlib_alarmonmbar)
        ROMlib_togglealarm();
}

LONGINT Executor::C_KeyTranslate(Ptr mapp, unsigned short code, LONGINT *state)
{
    LONGINT ascii;
    int table_num;
    unsigned char virt;
    kchr_ptr_t p;
    int table_num_index;

    p = (kchr_ptr_t)mapp;
    virt = code & VIRT_MASK;

    table_num_index = (code >> MODIFIER_SHIFT) & MODIFIER_MASK;
    table_num = CB((KCHR_MODIFIER_TABLE_X(p))[table_num_index]);
    ascii = (unsigned char)CB(KCHR_TABLE_X(p)[table_num][virt]);

    if(*state == 0)
    {
        if(ascii)
            *state = 0;
        else
        {
            int n_dead;
            dead_key_rec_t *deadp;

            n_dead = KCHR_N_DEAD_KEY_RECS(p);
            deadp = KCHR_DEAD_KEY_RECS_X(p);
            while(--n_dead >= 0
                  && (DEAD_KEY_TABLE_NUMBER(deadp) != table_num
                      || DEAD_KEY_VIRT_KEY(deadp) != virt))
                deadp = (dead_key_rec_t *)(&DEAD_KEY_NO_MATCH_X(deadp) + 1);
            if(n_dead >= 0)
                *state = (char *)deadp - (char *)&KCHR_N_TABLES_X(p);
            else
                *state = 0;
        }
    }
    else
    {
        dead_key_rec_t *deadp;
        completer_t *completep;
        int n_recs, i;

        deadp = (dead_key_rec_t *)((char *)&KCHR_N_TABLES_X(p) + *state);
        *state = 0;
        completep = &DEAD_KEY_COMPLETER_X(deadp);
        n_recs = COMPLETER_N_RECS(completep);
        for(i = 0;
            (i < n_recs
             && (CB((COMPLETER_COMPLETER_RECS_X(completep))[i].to_look_for)
                 != ascii));
            ++i)
            ;
        if(i < n_recs)
            ascii = (unsigned char)
                CB((COMPLETER_COMPLETER_RECS_X(completep)[i]).replacement);
        else
            ascii = (ascii << 16) | (unsigned short)DEAD_KEY_NO_MATCH(deadp);
    }
    return ascii;
}


void Executor::dofloppymount(void)
{
#if !defined(MSDOS) && !defined(LINUX) && !defined(CYGWIN32)
    SysBeep(5);
#else
    futzwithdosdisks();
#endif
}

static void doscreendumptoprinter(void)
{
    SysBeep(5);
}

static void doquitreallyquits(void)
{
    ROMlib_exit = !ROMlib_exit;
}

extern void ROMlib_updateworkspace(void);

static bool isSuspended = false;
static bool shouldBeSuspended = false;

static BOOLEAN doevent(INTEGER em, EventRecord *evt,
                       BOOLEAN remflag) /* no DA support */
{
    BOOLEAN retval;
    GUEST<ULONGINT> now_s;
    ULONGINT now;
    static int beenhere = 0;
    ALLOCABEGIN

    /* We tend to call this routine from various ROMlib modal loops, so this
     * is a good place to check for timer interrupts. */
    syncint_check_interrupt();

    hle_reset();

    evt->message = CLC(0);
    TRACE(2);
    if(LM(SPVolCtl) & 0x80)
    {
        TRACE(3);
        GetDateTime(&now_s);
        now = CL(now_s);
        TRACE(4);
        if(now >= (ULONGINT)Cx(LM(SPAlarm)))
        {
            TRACE(5);
            if(now & 1)
            {
                TRACE(6);
                if(ROMlib_alarmonmbar)
                    ROMlib_togglealarm();
            }
            else
            {
                TRACE(7);
                if(!ROMlib_alarmonmbar)
                    ROMlib_togglealarm();
            }
            TRACE(8);
        }
        else if(ROMlib_alarmonmbar)
        {
            TRACE(9);
            ROMlib_togglealarm();
        }
    }
    else if(ROMlib_alarmonmbar)
    {
        TRACE(10);
        ROMlib_togglealarm();
    }

    if(em & activMask)
    {
        TRACE(11);
        if(LM(CurDeactive))
        {
            TRACE(12);
            GetOSEvent(0, evt);
            TRACE(13);
            evt->what = CW(activateEvt);
            evt->message = guest_cast<LONGINT>(LM(CurDeactive));
            if(remflag)
                LM(CurDeactive) = nullptr;
            retval = true;
            /*-->*/ goto done;
        }
        if(LM(CurActivate))
        {
            TRACE(14);
            GetOSEvent(0, evt);
            TRACE(15);
            evt->what = CW(activateEvt);
            evt->message = guest_cast<LONGINT>(LM(CurActivate));
            evt->modifiers.raw_or(CW(activeFlag));
            if(remflag)
                LM(CurActivate) = nullptr;
            retval = true;
            /*-->*/ goto done;
        }
    }

    /*
 * NOTE: Currently (Sept. 15, 1993), we get different charCodes for
 *	 command-shift-1 on the NeXT and under DOS.  I suspect that
 *	 the DOS behaviour (the lower case stuff) is correct, my
 *	 clue is 1.15 vs 1.16 of osevent.c, but I don't want to mess
 *	 around with things just before releasing Executor/DOS 1.0.
 */

    if(remflag)
    {
        TRACE(16);
        retval = GetOSEvent(em, evt);
        TRACE(17);

        short fkeyModifiers = shiftKey | cmdKey;
#ifdef MACOSX
        fkeyModifiers = optionKey | cmdKey;
#endif
        if(retval && Cx(evt->what) == keyDown && LM(ScrDmpEnb) && (Cx(evt->modifiers) & fkeyModifiers) == fkeyModifiers)
        {
            TRACE(18);
            switch((Cx(evt->message) & keyCodeMask) >> 8)
            {
                case 0x12: /* command shift 1: About Box / Help */
                    retval = false;
                    do_about_box();
                    break;
                case 0x13: /* command shift 2: Floppy Stuff */
                    retval = false;
                    /* dofloppymount(); already done at a lower level */
                    break;
                case 0x14: /* command shift 3: Screen Dump to File */
                    retval = false;
                    do_dump_screen();
                    break;
                case 0x15: /* command shift 4: Screen Dump to Printer */
                    retval = false;
                    doscreendumptoprinter();
                    break;
                case 0x17: /* command shift 5: Preferences */
                    retval = false;
                    dopreferences();
                    break;
                case 0x16: /* command shift 6: Don't restart Executor */
                    retval = false;
                    doquitreallyquits();
                    break;
                case 0x1a:
                    retval = false;
                    /* Reset the video mode.  Seems to be needed under DOS
		 * sometimes when hotkeying around.
		 */
                    vdriver_set_mode(0, 0, 0, vdriver_grayscale_p);
                    redraw_screen();
                    break;
                // case 0x1c: // command shift 8
#if defined(SUPPORT_LOG_ERR_TO_RAM)
                case 0x19: /* command shift 9: Dump RAM error log */
                    retval = false;
                    error_dump_ram_err_buf("\n *** cmd-shift-9 pressed ***\n");
                    break;
#endif
            }
            if(!retval)
                evt->what = CW(nullEvent);
            /*-->*/ goto done;
        }
    }
    else
    {
        TRACE(24);
        retval = OSEventAvail(em, evt);
        TRACE(25);
    }
    /*
 * NOTE: I think the following block of code should probably be in SystemEvent,
 *	 rather than here.  It will probably make a difference when an event
 *	 call is made without diskMask set.  (I think SystemTask does the
 *	 mount anyway and it potentially gets lost if no one is looking for
 *	 it).
 */
    if(!retval && remflag && (em & diskMask))
    {
        TRACE(26);
        if(!beenhere)
        {
            TRACE(26);
            beenhere = 1;
            ROMlib_openharddisk("/tmp/testvol\0\0", &evt->message);
            if(evt->message)
            {
                TRACE(27);
                evt->what = CW(diskEvt);
                retval = true;
            }
        }
    }
    if(!retval && (em & updateMask))
    {
        TRACE(28);
        GetOSEvent(0, evt);
        TRACE(29);
        retval = CheckUpdate(evt);
    }

    /* check for high level events */
    if(!retval && (em & highLevelEventMask))
    {
        retval = hle_get_event(evt, remflag);
    }

    if(!retval && ROMlib_delay)
    {
        TRACE(30);
        Delay((LONGINT)ROMlib_delay, nullptr);
    }
done:
    ALLOCAEND
    TRACE(31);
    if(SystemEvent(evt))
    {
        TRACE(32);
        evt->what = CWC(nullEvent);
        retval = false;
    }
    TRACE(33);

    if(!retval)
    {
        if(isSuspended != shouldBeSuspended)
        {
            WindowPtr w = C_FrontWindow();
            if(w)
            {
                if(shouldBeSuspended)
                    LM(CurDeactive) = RM(w);
                else
                    LM(CurActivate) = RM(w);
                C_HiliteWindow(w, !shouldBeSuspended);
                isSuspended = shouldBeSuspended;
            }
        }
    }

    return retval;
}

BOOLEAN Executor::C_GetNextEvent(INTEGER em, EventRecord *evt)
{
    BOOLEAN retval;

    TRACE(1);
    retval = doevent(em, evt, true);
    TRACE(0);
    return retval;
}

/*
 * NOTE: the code for WaitNextEvent below is not really according to spec,
 *	 but it should do the job.  We could rig a fancy scheme that
 *	 communicates with the display postscript loop on the NeXT but that
 *	 wouldn't be general purpose and this *should* be good enough.  We
 *	 have to use Delay rather than timer calls ourself in case the timer
 *	 is already in use.  We also can't delay the full interval because
 *	 that would prevent window resizing from working on the NeXT (and
 *	 we'd never get the mouse moved messages we need)
 */

BOOLEAN Executor::C_WaitNextEvent(INTEGER mask, EventRecord *evp,
                                  LONGINT sleep, RgnHandle mousergn)
{
    BOOLEAN retval;
    Point p;

    do
    {
        retval = GetNextEvent(mask, evp);
        if(!retval)
        {
            static INTEGER saved_h, saved_v;

            /* TODO: see what PtInRgn does with 0 as a RgnHandle */
            p.h = CW(evp->where.h);
            p.v = CW(evp->where.v);
            if(mousergn && !EmptyRgn(mousergn) && !PtInRgn(p, mousergn)
               && (p.h != saved_h || p.v != saved_v))
            {
                evp->what = CWC(osEvt);
                evp->message = CLC(mouseMovedMessage << 24);
                retval = true;
            }
            else if(sleep > 0)
            {
                Delay(std::min(sleep, 4), nullptr);
                sleep -= 4;
            }
            saved_h = p.h;
            saved_v = p.v;
        }
    } while(!retval && sleep > 0);
    return retval;
}

BOOLEAN Executor::C_EventAvail(INTEGER em, EventRecord *evt)
{
    return (doevent(em, evt, false));
}

void Executor::C_GetMouse(GUEST<Point> *p)
{
    EventRecord evt;

    GetOSEvent(0, &evt);
    *p = evt.where;
    GlobalToLocal(p);
}

// FIXME: #warning Button not coded per IM Macintosh Toolbox Essentials 2-108
BOOLEAN Executor::C_Button()
{
    EventRecord evt;
    BOOLEAN retval;

    GetOSEvent(0, &evt);
    retval = (evt.modifiers & CWC(btnState)) ? false : true;
    return retval;
}

// FIXME: #warning StillDown not coded per IM Macintosh Toolbox Essentials 2-109
BOOLEAN Executor::C_StillDown() /* IMI-259 */
{
    EventRecord evt;

    return Button() ? !OSEventAvail((mDownMask | mUpMask), &evt) : false;
}

/*
 * The weirdness below is because Word 5.1a gets very unhappy if
 * TickCount makes large adjustments to LM(Ticks).  Even when we
 * just increase by one there are problems... The "no clock" option
 * might be retiring soon.
 */

BOOLEAN Executor::C_WaitMouseUp()
{
    EventRecord evt;
    int retval;

    retval = StillDown();
    if(!retval)
        GetOSEvent(mUpMask, &evt); /* just remove one ? */
    return (retval);
}

void Executor::C_GetKeys(unsigned char *keys)
{
    BlockMoveData((Ptr)LM(KeyMap), (Ptr)keys, (Size)sizeof_KeyMap);
}

LONGINT Executor::C_TickCount()
{
    unsigned long ticks;
    unsigned long new_time;

    ticks = msecs_elapsed() * 3.0 / 50; /* == 60 / 1000 */

    /* Update LM(Ticks) and LM(Time).  We only update LM(Ticks) if the clock is on;
   * this seems to be necessary to make Word happy.
   */

    if(ROMlib_clock)
        LM(Ticks) = CL(ticks);

    new_time = (UNIXTIMETOMACTIME(ROMlib_start_time.tv_sec)
                + (long)((ROMlib_start_time.tv_usec / (1000000.0 / 60) + ticks) / 60));

    LM(Time) = CL(new_time);
    return ticks;
}

LONGINT Executor::GetDblTime()
{
    return (Cx(LM(DoubleTime)));
}

LONGINT Executor::GetCaretTime()
{
    return (Cx(LM(CaretTime)));
}

/*
 * These routines used to live in the various OS-specific config directories,
 * which was a real crock, since only the NEXTSTEP implementation worked
 * well.  I'm moving them here as I attempt to implement cut and paste
 * properly under Win32.
 */

#define SANE_DEBUGGING
#if defined(SANE_DEBUGGING)
static int sane_debugging_on = 0; /* Leave this off and let the person doing the
			      debugging turn it on if he/she wants.  If this
			      is set to non-zero, it breaks code.  Not a
			      very nice thing to do. */
#endif /* SANE_DEBUGGING */

void
Executor::sendsuspendevent(void)
{
    Point p;

    shouldBeSuspended = true;
    if(
        (size_info.size_flags & SZacceptSuspendResumeEvents)
#if defined(SANE_DEBUGGING)
        && !sane_debugging_on
#endif /* SANE_DEBUGGING */
        /* NOTE: Since Executor can currently only run one app at a time,
	 suspending an app that can background causes trouble with apps
	 like StuffIt Expander, since it makes it impossible to do work
	 in another window while waiting for StuffIt Expander to expand
	 a file.  It's not clear that the next two lines are the best solution,
	 but it's *a* solution. */
        && (!(ROMlib_options & ROMLIB_NOSUSPEND_BIT) /* ||
	  !(size_info.size_flags & SZcanBackground) */))
    {
        p.h = CW(LM(MouseLocation).h);
        p.v = CW(LM(MouseLocation).v);
        ROMlib_PPostEvent(osEvt, SUSPENDRESUMEBITS | SUSPEND | CONVERTCLIPBOARD,
                          (GUEST<EvQElPtr> *)0, TickCount(), p, ROMlib_mods);
    }
}

void
Executor::sendresumeevent(bool cvtclip)
{
    LONGINT what;
    Point p;

    shouldBeSuspended = false;
    if(
        (size_info.size_flags & SZacceptSuspendResumeEvents)
#if defined(SANE_DEBUGGING)
        && !sane_debugging_on
#endif /* SANE_DEBUGGING */
        )
    {
        what = SUSPENDRESUMEBITS | RESUME;
        if(cvtclip)
            what |= CONVERTCLIPBOARD;
        p.h = CW(LM(MouseLocation).h);
        p.v = CW(LM(MouseLocation).v);
        ROMlib_PPostEvent(osEvt, what, (GUEST<EvQElPtr> *)0, TickCount(),
                          p, ROMlib_mods);
    }
}

void
sendcopy(void)
{
    Point p;

    p.h = CW(LM(MouseLocation).h);
    p.v = CW(LM(MouseLocation).v);
    ROMlib_PPostEvent(keyDown, 0x0863, /* 0x63 == 'c' */
                      (GUEST<EvQElPtr> *)0, TickCount(), p, cmdKey | btnState);
    ROMlib_PPostEvent(keyUp, 0x0863,
                      (GUEST<EvQElPtr> *)0, TickCount(), p, cmdKey | btnState);
}

void
sendpaste(void)
{
    Point p;

    p.h = CW(LM(MouseLocation).h);
    p.v = CW(LM(MouseLocation).v);
    ROMlib_PPostEvent(keyDown, 0x0976, /* 0x76 == 'v' */
                      (GUEST<EvQElPtr> *)0, TickCount(), p, cmdKey | btnState);
    ROMlib_PPostEvent(keyUp, 0x0976,
                      (GUEST<EvQElPtr> *)0, TickCount(), p, cmdKey | btnState);
}

/*
 * NOTE: the code for ROMlib_send_quit below is cleaner than the code for
 *       sendcopy and sendpaste above, but it was added after 2.1pr0
 *       was released but before 2.1 was released, so we can't tamper
 *       with the above code because there's a chance it would break
 *       something.  Ick.
 */

static void
post_helper(INTEGER code, uint8_t raw, uint8_t mapped, INTEGER mods)
{
    Point p;

    p.h = CW(LM(MouseLocation).h);
    p.v = CW(LM(MouseLocation).v);

    ROMlib_PPostEvent(code, (raw << 8) | mapped, (GUEST<EvQElPtr> *)0,
                      TickCount(), p, btnState | mods);
}

void
Executor::ROMlib_send_quit(void)
{
    post_helper(keyDown, MKV_CLOVER, 0, 0);
    post_helper(keyDown, MKV_q, 'q', cmdKey);
    post_helper(keyUp, MKV_q, 'q', cmdKey);
    post_helper(keyUp, MKV_CLOVER, 0, cmdKey);
}
