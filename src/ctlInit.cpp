/* Copyright 1986, 1988, 1989, 1990, 1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in ControlMgr.h (DO NOT DELETE THIS LINE) */

#include "rsys/common.h"
#include "QuickDraw.h"
#include "WindowMgr.h"
#include "ControlMgr.h"
#include "ToolboxUtil.h"
#include "ResourceMgr.h"
#include "MemoryMgr.h"
#include "OSUtil.h"

#include "rsys/ctl.h"
#include "rsys/wind.h"
#include "rsys/cquick.h"
#include "rsys/resource.h"

using namespace Executor;

ControlHandle Executor::C_NewControl(WindowPtr wst, Rect *r, StringPtr title,
                                     BOOLEAN vis, INTEGER value, INTEGER min,
                                     INTEGER max, INTEGER procid,
                                     LONGINT rc) /* IMI-319 */
{
    ControlHandle retval;
    GrafPtr gp;
    AuxCtlHandle tmp, auxctlhead;
    GUEST<Handle> temph;

    retval = (ControlHandle)NewHandle((Size)sizeof(ControlRecord));
    temph = GetResource(TICK("CDEF"), procid >> 4);
    if(!(HxX(retval, contrlDefProc) = temph))
    {
        DisposeHandle((Handle)retval);
        return 0;
    }

    if(!title)
        title = (StringPtr) ""; /* nothing ("\p" == "") */
    gp = qdGlobals().thePort;
    SetPort((GrafPtr)wst);

    tmp = LM(AuxCtlHead);
    auxctlhead = (AuxCtlHandle)NewHandle(sizeof(AuxCtlRec));
    LM(AuxCtlHead) = auxctlhead;

    HASSIGN_6(auxctlhead,
              acNext, tmp,
              acOwner, retval,
              acCTable, (CCTabHandle)GetResource(TICK("cctb"), 0),
              acFlags, procid & 0x0F,
              acReserved, 0,
              acRefCon, 0);
    str255assign(CTL_TITLE(retval), title);

    if(!HxX(auxctlhead, acCTable))
    {
        warning_unexpected("no 'cctb', 0, probably old system file ");
        HxX(auxctlhead, acCTable) = (CCTabHandle)default_ctl_ctab;
    }

    HASSIGN_11(retval,
               contrlRect, *r,
               nextControl, WINDOW_CONTROL_LIST_X(wst),
               contrlOwner, wst,
               contrlVis, vis ? 255 : 0,
               contrlHilite, 0,
               contrlValue, value,
               contrlMin, min,
               contrlMax, max,
               contrlData, nullptr,
               contrlAction, nullptr,
               contrlRfCon, rc);

    WINDOW_CONTROL_LIST_X(wst) = retval;

    {
        CtlCallGuard guard(retval);

        CTLCALL(retval, initCntl, 0);
        if(WINDOW_VISIBLE_X(wst) && vis)
            CTLCALL(retval, drawCntl, 0);
    }

    SetPort(gp);
    return retval;
}

ControlHandle Executor::C_GetNewControl(INTEGER cid, WindowPtr wst) /* IMI-321 */
{
    typedef contrlrestype *wp;

    GUEST<wp> *wh;
    ControlHandle retval;
    Handle ctab_res_h;

    wh = (GUEST<wp> *)GetResource(TICK("CNTL"), cid);
    if(!wh)
        return 0;
    if(!(*wh))
        LoadResource((Handle)wh);
    ctab_res_h = ROMlib_getrestid(TICK("cctb"), cid);
    retval = NewControl(wst, &(HxX(wh, _crect)),
                        (StringPtr)((char *)&HxX(wh, _crect) + 22), /* _ctitle */
                        Hx(wh, _cvisible) != 0, Hx(wh, _cvalue), Hx(wh, _cmin),
                        Hx(wh, _cmax), Hx(wh, _cprocid),
                        *(GUEST<LONGINT> *)((char *)&HxX(wh, _crect) + 18)); /* _crefcon */
    if(ctab_res_h)
        SetControlColor(retval, (CCTabHandle)ctab_res_h);
    return retval;
}

void Executor::C_SetControlColor(ControlHandle ctl, CCTabHandle ctab)
{
    AuxCtlHandle aux_c;

    if(ctl)
    {
        aux_c = *lookup_aux_ctl(ctl);
        if(!aux_c)
        {
            AuxCtlHandle t_aux_c;

            /* allocate one */
            t_aux_c = LM(AuxCtlHead);
            aux_c = (AuxCtlHandle)NewHandle(sizeof(AuxCtlRec));
            LM(AuxCtlHead) = aux_c;
            HxX(aux_c, acNext) = t_aux_c;
            HxX(aux_c, acOwner) = ctl;

            HxX(aux_c, acCTable) = ctab;

            HxX(aux_c, acFlags) = 0;
            HxX(aux_c, acReserved) = 0;
            HxX(aux_c, acRefCon) = 0;
        }
        else
            HxX(aux_c, acCTable) = ctab;

        if(CTL_VIS(ctl))
            Draw1Control(ctl);
    }
    else
    {
        abort();
    }
}

void Executor::C_DisposeControl(ControlHandle c) /* IMI-321 */
{

    GUEST<ControlHandle> *t;

    GUEST<AuxCtlHandle> *auxhp;
    AuxCtlHandle saveauxh;

    HideControl(c);
    {
        CtlCallGuard guard(c);

        CTLCALL(c, dispCntl, 0);
    }

    for(t = (GUEST<ControlHandle> *)&(((WindowPeek)(HxP(c, contrlOwner)))->controlList);
        (*t) && STARH(t) != c; t = (GUEST<ControlHandle> *)&((STARH(STARH(t)))->nextControl))
        ;
    if((*t))
        (*t) = HxX(c, nextControl);
    for(auxhp = (GUEST<AuxCtlHandle> *)&LM(AuxCtlHead); (*auxhp) && (STARH(STARH(auxhp)))->acOwner != c;
        auxhp = (GUEST<AuxCtlHandle> *)&(STARH(STARH(auxhp)))->acNext)
        ;
    if((*auxhp))
    {
        saveauxh = STARH(auxhp);
        (*auxhp) = STARH(STARH(auxhp))->acNext;
        DisposeHandle((Handle)saveauxh);
    }

    DisposeHandle((Handle)c);
}

void Executor::C_KillControls(WindowPtr w) /* IMI-321 */
{
    ControlHandle c, t;

    for(c = ((WindowPeek)w)->controlList; c;)
    {
        t = c;
        c = HxP(c, nextControl);
        DisposeControl(t);
    }
}
