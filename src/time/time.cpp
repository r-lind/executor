/* Copyright 1989, 1990, 1995, 1996 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in TimeMgr.h (DO NOT DELETE THIS LINE) */

#include <base/common.h>

#include <OSUtil.h>
#include <TimeMgr.h>

#include <rsys/osutil.h>
#include <time/vbl.h>
#include <time/time.h>
#include <time/syncint.h>
#include <rsys/hook.h>
#include <vdriver/refresh.h>
#include <sound/soundopts.h>
#include <base/cpu.h>
#include <PowerCore.h>
#include <base/debugger.h>
#include <chrono>

using namespace Executor;


QHdr Executor::ROMlib_timehead;

/* Actual time at which Executor started running, GMT. */
struct timeval ROMlib_start_time;

/* Msecs during last interrupt. */
static unsigned long last_interrupt_msecs;

/* Msecs during next anticipated interrupt. */
static unsigned long next_interrupt_msecs;


unsigned long
msecs_elapsed()
{
    static auto startTime = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
}

/*
 * catchalarm has been written with an eye toward not having errors accumulate.
 * In the UNIX world it is going to be hard to give great response time, but
 * it is unexcusable for repeated calls to catchalarm to distort the timing
 * of events that are far in the future.
 *
 * Unfortunately, unless we change the data structure that is in the linked
 * list, errors are bound to accumulate.  Each time we take an interrupt
 * our idea of how much time has passed could be off by as much as a
 * millisecond.  We have to take interrupts 60 times a second so we will tend
 * to drift a couple ticks each second.  If this proves unacceptable, the
 * datastructure will have to be expanded so we keep a timeval around that
 * tells us when to expire.
 */

#define REALLONGTIME 0x7FFFFFFF

static void catchalarmCommon()
{
    ULONGINT diff;
    TMTask *qp;
    LONGINT min;
    LONGINT tm_count;
    M68kReg saved_regs[16];
    CCRElement saved_ccnz, saved_ccn, saved_ccc, saved_ccv, saved_ccx;
    unsigned long now_msecs;

    cpu_state.interrupt_pending[M68K_TIMER_PRIORITY] = false;
    getPowerCore().cancelInterrupt();

    /* We save the 68k registers and cc bits away. */
    memcpy(saved_regs, &cpu_state.regs, sizeof saved_regs);
    saved_ccnz = cpu_state.ccnz;
    saved_ccn = cpu_state.ccn;
    saved_ccc = cpu_state.ccc;
    saved_ccv = cpu_state.ccv;
    saved_ccx = cpu_state.ccx;

    /* There's no reason to think we need to decrement A7 by 32;
   * it's just a paranoid thing to do.
   */
    EM_A7 = (EM_A7 - 32) & ~3; /* Might as well long-align it. */

    /* Loop while it's still time to do stuff sitting in the queue. */
    do
    {
        unsigned long msecs;

        msecs = msecs_elapsed();
        diff = msecs - last_interrupt_msecs;
        last_interrupt_msecs = msecs;

        for(qp = (TMTask *)ROMlib_timehead.qHead;
            qp;
            qp = (TMTask *)qp->qLink)
        {
            tm_count = qp->tmCount;

            if(tm_count > 0)
            {
                tm_count -= diff;
                qp->tmCount = tm_count;
                if(tm_count <= 0)
                {

                    if(ProcPtr tm_addr = qp->tmAddr)
                    {
                        ROMlib_hook(time_number);

                        /* No need to save and restore regs here; we
                         * save and restore all of them outside this
                         * loop. */
                        EM_A0 = US_TO_SYN68K(tm_addr);
                        EM_A1 = US_TO_SYN68K(qp);

                        execute68K((syn68k_addr_t)EM_A0);
                    }
                }
            }
        }

        /* Find the next imminent timer event in the queue. */
        min = REALLONGTIME;
        for(qp = (TMTask *)ROMlib_timehead.qHead;
            qp;
            qp = (TMTask *)qp->qLink)
        {
            tm_count = qp->tmCount;
            if(tm_count > 0 && tm_count < min)
                min = tm_count;
        }

        /* Fetch the current time and move the nearest event even closer
       * to compensate for time spent in this procedure.
       */
        now_msecs = msecs_elapsed();
        if(min < REALLONGTIME)
            min -= now_msecs - last_interrupt_msecs;
    } while(min <= 0);

    if(min < REALLONGTIME)
    {
        /* If there's anything left in the queue, set up another
       * timer interrupt to come in at the appropriate time.
       */
        syncint_post(std::chrono::milliseconds(min));

        next_interrupt_msecs = now_msecs + min;
    }
    else
    {
        /* Note that there's no interrupt queued up. */
        next_interrupt_msecs = 0;
    }

    memcpy(&cpu_state.regs, saved_regs, sizeof saved_regs);
    cpu_state.ccnz = saved_ccnz;
    cpu_state.ccn = saved_ccn;
    cpu_state.ccc = saved_ccc;
    cpu_state.ccv = saved_ccv;
    cpu_state.ccx = saved_ccx;
}

syn68k_addr_t Executor::catchalarm(syn68k_addr_t interrupt_pc, void *unused)
{
    currentCPUMode = CPUMode::m68k;
    currentM68KPC = *ptr_from_longint<GUEST<uint32_t>*>(EM_A7 + 2);

    catchalarmCommon();

    if(base::Debugger::instance && base::Debugger::instance->interruptRequested())
        return base::Debugger::instance->nmi68K(interrupt_pc);

    return MAGIC_RTE_ADDRESS;
}

void Executor::catchalarmPowerPC(PowerCore&)
{
    currentCPUMode = CPUMode::ppc;

    auto& cpu = getPowerCore();

    PowerCoreState saveState = (PowerCoreState&)cpu;

    // TODO: save registers, and so on

    cpu.r[1] -= 128;
    EM_A7 = cpu.r[1];
    catchalarmCommon();
    cpu.r[1] += 128;
    EM_A7 = cpu.r[1];

    (PowerCoreState&)cpu = saveState;

    if(base::Debugger::instance && base::Debugger::instance->interruptRequested())
        base::Debugger::instance->nmiPPC();
}


static void ROMlib_PrimeTime(QElemPtr taskp, LONGINT count)
{
    static char beenhere = false;
    LONGINT msecs_until_next;
    unsigned long now_msecs;

/*
 * We introduce this fudge factor because we are nervous that our time
 * calculations will mess up and result in a negative number under Executor
 * when on a real Mac the number wouldn't be negative.  This is paranoia,
 * but small timing stuff just isn't going to work properly under Executor
 * anyway.
 */

#define FUDGE_FACTOR (-30)

    if(count < FUDGE_FACTOR)
        count = -count / 1000; /* IM-Processes 3-20 */

    if(count <= 0)
        count = 1;

    now_msecs = msecs_elapsed();
    if(!beenhere)
    {
        last_interrupt_msecs = now_msecs; /* actually there haven't been any */
        msecs_until_next = 0x7FFF0000; /* Arbitrary large value. */
        next_interrupt_msecs = now_msecs + msecs_until_next;
        beenhere = true;
    }
    else
    {
        msecs_until_next = next_interrupt_msecs - now_msecs;
    }

    /* catchalarm works by subtracting off the msecs _since the last
   * interrupt_ from each entry in the queue.  Since we might be
   * getting posted sometime between interrupts, we have to compensate
   * by adding the time since the last interrupt to our count.  That
   * way the extra time subtracted off during the catchalarm will
   * exactly match the extra time we added here.
   */
    ((TMTask *)taskp)->tmCount = count + now_msecs - last_interrupt_msecs;

    if(count < msecs_until_next || msecs_until_next <= 0)
    {
        syncint_post(std::chrono::milliseconds(count));

        next_interrupt_msecs = now_msecs + count;
    }
}

void Executor::InsTime(QElemPtr taskp)
{
    ((TMTask *)taskp)->tmCount = -1;
    Enqueue(taskp, &ROMlib_timehead);
}

void Executor::RmvTime(QElemPtr taskp)
{
    Dequeue(taskp, &ROMlib_timehead);
}

void Executor::PrimeTime(QElemPtr taskp, LONGINT count)
{
    ROMlib_PrimeTime(taskp, count);
}
