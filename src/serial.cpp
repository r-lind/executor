/*
 * Copyright 1989, 1990, 1995, 1997 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/*
 * This file is an ugly mess of UNIX specific code portable Mac-side code.
 * The UNIX specific stuff should be put in its appropriate place sometime.
 */

#include "rsys/common.h"

#if defined(MACOSX)
// FIXME: #warning bad serial support right now
//TODO: this seems to use sgtty functions instead of termios.
#include <sgtty.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#endif /* !defined (MACOSX) */

#include "Serial.h"
#include "DeviceMgr.h"
#include "FileMgr.h"
#include "MemoryMgr.h"
#include "OSUtil.h"

#include "rsys/file.h"
#include "rsys/hfs.h"
#include "rsys/serial.h"

#if defined(CYGWIN32) || defined(WIN32)
#include "win_serial.h"
#endif

#if !defined(WIN32)
#include <termios.h>
#endif

#if defined(LINUX) || defined(MACOSX)
#include <sys/ioctl.h>
#endif

using namespace Executor;

#if defined(__alpha) || defined(LINUX)
#define TERMIO
/* #define	MSDOS	ick! */
#endif

/*
 * TODO:
 *
 * Post events on break action or hardware handshake changes IMII-247
 * Support KillIO from the device manager IMII-248
 * Hardware handshaking on machines that support it IMII-252
 *
 * Finish up the get status routine?
 */

#define AINNAME "\004.AIn"
#define AOUTNAME "\005.AOut"
#define BINNAME "\004.BIn"
#define BOUTNAME "\005.BOut"
#define AINREFNUM (-6)
#define AOUTREFNUM (-7)
#define BINREFNUM (-8)
#define BOUTREFNUM (-9)

#if !defined(XONC)
#define XONC '\21'
#define XOFFC '\23'
#endif

#define SER_START (1)
#define SER_STOP (0)

OSErr Executor::RAMSDOpen(SPortSel port) /* IMII-249 */
{
    OSErr err;
    GUEST<INTEGER> rn;

    switch(port)
    {
        case sPortA:
            if((err = OpenDriver((StringPtr)AINNAME, &rn)) == noErr)
                err = OpenDriver((StringPtr)AOUTNAME, &rn);
            break;
        case sPortB:
            if((err = OpenDriver((StringPtr)BINNAME, &rn)) == noErr)
                err = OpenDriver((StringPtr)BOUTNAME, &rn);
            break;
        default:
            err = openErr;
            break;
    }
    return err;
}

void Executor::RAMSDClose(SPortSel port) /* IMII-250 */
{
    switch(port)
    {
        case sPortA:
            (void)CloseDriver(AINREFNUM);
            (void)CloseDriver(AOUTREFNUM);
            break;
        case sPortB:
            (void)CloseDriver(BINREFNUM);
            (void)CloseDriver(BOUTREFNUM);
            break;
    }
}

#define SERSET 8 /* IMII-250 */

OSErr Executor::SerReset(INTEGER rn, INTEGER config) /* IMII-250 */
{
    GUEST<INTEGER> config_s = config;

    return Control(rn, SERSET, (Ptr)&config_s);
}

#define SERSETBUF 9 /* IMII-251 */

OSErr Executor::SerSetBuf(INTEGER rn, Ptr p, INTEGER len) /* IMII-251 */
{
    sersetbuf_t temp;

    temp.p = p;
    temp.i = len;

    return Control(rn, SERSETBUF, (Ptr)&temp);
}

#define SERHSHAKE 10 /* IMII-251 */

OSErr Executor::SerHShake(INTEGER rn, const SerShk *flags) /* IMII-251 */
{
    return Control(rn, SERHSHAKE, (Ptr)flags);
}

#define SERSETBRK 12 /* IMII-252 */

OSErr Executor::SerSetBrk(INTEGER rn) /* IMII-252 */
{
    return Control(rn, SERSETBRK, (Ptr)0);
}

#define SERCLRBRK 11 /* IMII-253 */

OSErr Executor::SerClrBrk(INTEGER rn) /* IMII-253 */
{
    return Control(rn, SERCLRBRK, (Ptr)0);
}

#define SERGETBUF 2 /* IMII-253 */

OSErr Executor::SerGetBuf(INTEGER rn, LONGINT *lp) /* IMII-253 */
{
    INTEGER status[11];
    OSErr err;

    if((err = Status(rn, SERGETBUF, (Ptr)status)) == noErr)
        *lp = *(LONGINT *)status;
    return err;
}

#define SERSTATUS 8 /* IMII-253 */

OSErr Executor::SerStatus(INTEGER rn, SerStaRec *serstap) /* IMII-253 */
{
    INTEGER status[11];
    OSErr err;

    if((err = Status(rn, SERSTATUS, (Ptr)status)) == noErr)
        BlockMoveData((Ptr)status, (Ptr)serstap, (Size)sizeof(*serstap));
    return err;
}

#define OPENBIT (1 << 0)

#if !defined(MSDOS) && !defined(CYGWIN32) && !defined(WIN32)
#if defined(TERMIO)

typedef struct
{
    LONGINT fd; /* will be the same for both In and Out */
    struct termio state;
} hidden, *hiddenp;

#else

typedef struct
{
    LONGINT fd; /* will be the same for both In and Out */
    struct sgttyb sgttyb;
    struct tchars tchars;
    ULONGINT lclmode;
} hidden, *hiddenp;

#endif
#else

typedef struct
{
    LONGINT fd; /* probably need to save more state information */
    /* right now we just use fd to keep track of which port we're talking to */
} hidden, *hiddenp;

#endif

typedef GUEST<hiddenp> *hiddenh;

static DCtlPtr otherdctl(ParmBlkPtr);
static OSErr serset(LONGINT fd, INTEGER param);
static OSErr serxhshake(LONGINT fd, SerShk *sershkp);
static OSErr setbaud(LONGINT fd, INTEGER baud);
static OSErr flow(LONGINT fd, LONGINT flag);
static OSErr ctlbrk(LONGINT fd, INTEGER flag);

static DCtlPtr otherdctl(ParmBlkPtr pbp)
{
    DCtlHandle h;

    h = 0;
    switch(pbp->cntrlParam.ioCRefNum)
    {
        case AINREFNUM:
            h = GetDCtlEntry(AOUTREFNUM);
            break;
        case AOUTREFNUM:
            h = GetDCtlEntry(AINREFNUM);
            break;
        case BINREFNUM:
            h = GetDCtlEntry(BOUTREFNUM);
            break;
        case BOUTREFNUM:
            h = GetDCtlEntry(BINREFNUM);
            break;
    }
    return h ? *h : 0;
}

#if defined(LINUX) || defined(MACOSX)

/*
 * NOTE:  Currently we're using cufa and cufb; we really should
 *	  delay the opening until we know just what type of flow
 *	  control the user wants to do.
 */
static const char *specialname(ParmBlkPtr pbp)
{
    const char *retval;
    retval = 0;

    switch(pbp->cntrlParam.ioCRefNum)
    {
        case AINREFNUM:
        case AOUTREFNUM:
            retval = "/dev/ttyUSB1";
            break;
        case BINREFNUM:
        case BOUTREFNUM:
            retval = "/dev/ttyUSB2";
            break;
    }
    return retval;
}
#endif /* defined (LINUX) || defined (MACOSX) */

typedef void (*compfuncp)(void);


void callcomp(ParmBlkPtr pbp, ProcPtr comp, OSErr err)
{
    EM_A0 = US_TO_SYN68K(pbp);
    EM_A1 = US_TO_SYN68K(comp);
    EM_D0 = (unsigned short)err; /* TODO: unsigned short ? */
    CALL_EMULATOR((syn68k_addr_t)(uintptr_t)comp);
}

#define DOCOMPLETION(pbp, err)                                                   \
    (pbp)->ioParam.ioResult = err;                                           \
    if(((pbp)->ioParam.ioTrap & asyncTrpBit) && (pbp)->ioParam.ioCompletion) \
        callcomp(pbp, (pbp)->ioParam.ioCompletion, err);                     \
    return err


#define SERIALDEBUG

OSErr Executor::C_ROMlib_serialopen(ParmBlkPtr pbp, DCtlPtr dcp) /* INTERNAL */
{
    OSErr err;
    DCtlPtr otherp; /* auto due to old compiler bug */
    hiddenh h;
#if defined(LINUX) || defined(MACOSX)
    const char *devname;
    LONGINT fd, ourpid, theirpid, newfd;
#endif

    err = noErr;
    if(!(dcp->dCtlFlags & OPENBIT))
    {
        h = (hiddenh)NewHandle(sizeof(hidden));
        dcp->dCtlStorage = (Handle)h;
        otherp = otherdctl(pbp);
        if(otherp && (otherp->dCtlFlags & OPENBIT))
        {
            **h = **(hiddenh)otherp->dCtlStorage;
            dcp->dCtlFlags |= OPENBIT;
        }
        else
        {
#if defined(LINUX) || defined(MACOSX)
            err = permErr;
            if((devname = specialname(pbp)))
            {
                err = noErr;
            }
#endif
            if(err == noErr)
            {
#if defined(LINUX) || defined(MACOSX)
                (*h)->fd = ROMlib_priv_open(devname, O_BINARY | O_RDWR);
                if((*h)->fd < 0)
                    err = (*h)->fd; /* error return piggybacked */
                else
                {
#if defined(TERMIO)
                    err = ioctl((*h)->fd, TCGETA, &(*h)->state) < 0 ? ROMlib_maperrno() : noErr;
#else
                    if(ioctl((*h)->fd, TIOCGETP, &(*h)->sgttyb) < 0 || ioctl((*h)->fd, TIOCGETC, &(*h)->tchars) < 0 || ioctl((*h)->fd, TIOCLGET, &(*h)->lclmode) < 0)
                        err = ROMlib_maperrno();
#endif
#else
                (*h)->fd = (pbp->cntrlParam.ioCRefNum == AINREFNUM || pbp->cntrlParam.ioCRefNum == AOUTREFNUM) ? 0 : 1;
#endif
                    dcp->dCtlFlags |= OPENBIT;
                    SerReset(pbp->cntrlParam.ioCRefNum,
                             (pbp->cntrlParam.ioCRefNum == AINREFNUM || pbp->cntrlParam.ioCRefNum == AOUTREFNUM) ? LM(SPPortA) : LM(SPPortB));
#if defined(LINUX) || defined(MACOSX)
                }
#endif
            }
        }
    }
#if defined(SERIALDEBUG)
    warning_trace_info("serial open returning %d", (LONGINT)err);
#endif
    DOCOMPLETION(pbp, err);
}

OSErr Executor::C_ROMlib_serialprime(ParmBlkPtr pbp, DCtlPtr dcp) /* INTERNAL */
{
    OSErr err;
    hiddenh h;
    char *buf;
    size_t req_count;

    buf = (char *)pbp->ioParam.ioBuffer;
    req_count = pbp->ioParam.ioReqCount;

    err = noErr;
    if(!(dcp->dCtlFlags & OPENBIT))
        err = notOpenErr;
    else
    {
#if defined(SERIALDEBUG)
        warning_trace_info("serial prime code %d", (LONGINT)(pbp->ioParam.ioTrap & 0xFF));
#endif
        h = (hiddenh)dcp->dCtlStorage;
        switch(pbp->ioParam.ioTrap & 0xFF)
        {
            case aRdCmd:
                if(pbp->cntrlParam.ioCRefNum != AINREFNUM && pbp->cntrlParam.ioCRefNum != BINREFNUM)
                    err = readErr;
                else
                {
/* this may have to be changed since we aren't looking for
		   parity and framing errors */
#if defined(LINUX) || defined(MACOSX)
                    pbp->ioParam.ioActCount = read((*h)->fd, buf, req_count);
#elif defined(MSDOS) || defined(CYGWIN32) || defined(WIN32)
                    pbp->ioParam.ioActCount = serial_bios_read((*h)->fd, buf,
                                                                  req_count);
#else
// FIXME: #warning not sure what to do here
#endif
#if defined(SERIALDEBUG)
                    warning_trace_info("serial prime read %d bytes, first is 0x%0x",
                                       (LONGINT)pbp->ioParam.ioActCount,
                                       (LONGINT)(unsigned char)buf[0]);
#endif
                }
                break;
            case aWrCmd:
                if(pbp->cntrlParam.ioCRefNum != AOUTREFNUM && pbp->cntrlParam.ioCRefNum != BOUTREFNUM)
                    err = writErr;
                else
                {
#if defined(SERIALDEBUG)
                    warning_trace_info("serial prime writing %d bytes, first is 0x%0x",
                                       (LONGINT)req_count,
                                       (LONGINT)(unsigned char)buf[0]);
#endif
#if defined(LINUX) || defined(MACOSX)
                    pbp->ioParam.ioActCount = write((*h)->fd,
                                                       buf, req_count);
#elif defined(MSDOS) || defined(CYGWIN32) || defined(WIN32)
                    pbp->ioParam.ioActCount = serial_bios_write((*h)->fd,
                                                                   buf, req_count);
#else
// FIXME: #warning not sure what to do here
#endif
                }
                break;
            default:
                err = badUnitErr;
                break;
        }
    }
#if defined(SERIALDEBUG)
    warning_trace_info("serial prime returning %d", (LONGINT)err);
#endif
    DOCOMPLETION(pbp, err);
}

#if defined(TERMIO)

#define OURCS7 (CS7)
#define OURCS8 (CS8)
#define OURODDP (PARENB | PARODD)
#define OUREVENP (PARENB)

#else

#define OURCS7 (0)

#if defined(NEXTSTEP)
#define OURCS8 (LPASS8 | LPASS8OUT)
#else
#define OURCS8 (LPASS8)
#endif

#define OURODDP (ODDP)
#define OUREVENP (EVENP)

#endif

static OSErr serset(LONGINT fd, INTEGER param)
{
#if defined(MSDOS) || defined(CYGWIN32) || defined(WIN32)
    OSErr retval;

    retval = serial_bios_serset(fd, param);
    return retval;
#else

#if defined(TERMIO)
    struct termio settings;
#else
    struct sgttyb sgttyb;
    struct tchars tchars;
    ULONGINT lclmode;
#endif

    OSErr err;
    LONGINT baud, csize, stopbits, parity;

#if !defined(LETGCCWAIL)
    baud = 0;
    csize = 0;
    stopbits = 0;
#endif

    err = noErr;

    switch(param & 0x3FF)
    {
        case baud300:
            baud = B300;
            break;
        case baud600:
            baud = B600;
            break;
        case baud1200:
            baud = B1200;
            break;
        case baud1800:
            baud = B1800;
            break;
        case baud2400:
            baud = B2400;
            break;
        case baud4800:
            baud = B4800;
            break;
        case baud9600:
            baud = B9600;
            break;
        case baud19200:
            baud = EXTA;
            break;
        case 1: /* really 38400 according to Terminal 2.2 source */
            baud = EXTB;
            break;
        case baud3600: /* these case labels aren't necessary since they */
        case baud7200: /* fall into default anyway, just thought you */
        case baud57600: /* might like to know what you're missing */
        default:
/*
 * NOTE: setting controlErr here isn't probably a good idea, since
 *       Compuserve Signup does a serset with baud57600 and then later
 *	 adjusts the baud with a "setbaud"
 */
#if 0
	err = controlErr;
#else
            baud = B2400;
#endif
            break;
    }

    switch(param & 0xC00)
    {

        case data5:
        case data6:
            err = controlErr;
            break;

        case data7:
            csize = OURCS7;
            break;

        case data8:
            csize = OURCS8;
            break;
    }

    switch(param & 0x3000)
    {

        case oddParity:
            parity = OURODDP;
            break;

        case evenParity:
            parity = OUREVENP;
            break;

        default:
            parity = 0;
            break;
    }

    switch((param >> 14) & 0x3)
    {
        case 1:
            stopbits = 0;
            break;
        case 3:
#if defined(TERMIO)
            stopbits = CSTOPB;
            break;
#endif
        case 0:
        case 2:
            err = controlErr;
            break;
    }

    if(err == noErr)
    {
#if defined(TERMIO)
        settings.c_iflag = IGNPAR | IXON | IXOFF;
        settings.c_oflag = 0;
        settings.c_cflag = baud | csize | stopbits | parity;
        settings.c_lflag = 0;
        settings.c_line = 0; /* TODO: find out real value here */
        err = ioctl(fd, TCSETAW, &settings) < 0 ? ROMlib_maperrno() : noErr;
#else
        sgttyb.sg_ispeed = sgttyb.sg_ospeed = baud;
        sgttyb.sg_erase = -1;
        sgttyb.sg_kill = -1;
        sgttyb.sg_flags = TANDEM | CBREAK | parity;
        tchars.t_intrc = -1;
        tchars.t_quitc = -1;
#if 0 /* This isn't the place to mess with flow control */
	if (sershkp->fXOn || sershkp->fInX) {
	    tchars.t_startc = sershkp->xOn;
	    tchars.t_stopc = sershkp->xOff;
	} else {
	    tchars.t_startc = sershkp->xOn;
	    tchars.t_stopc = sershkp->xOff;
	}
	/* Cliff, LDECCTQ restricts XON to startc ONLY! --Lee */
	lclmode = sershkp->fXOn ? LDECCTQ : 0;
#else
        lclmode = 0;
#endif
        tchars.t_eofc = -1;
        tchars.t_brkc = -1;
        lclmode |= LLITOUT | LNOHANG | csize | LNOFLSH;
        if(ioctl(fd, TIOCSETP, &sgttyb) < 0 || ioctl(fd, TIOCSETC, &tchars) < 0 || ioctl(fd, TIOCLSET, &lclmode) < 0)
            err = ROMlib_maperrno();
#endif
    }
    return err;
#endif
}

static OSErr serxhshake(LONGINT fd, SerShk *sershkp)
{
#if defined(MSDOS) || defined(CYGWIN32) || defined(WIN32)
    OSErr retval;

    retval = serial_bios_serxhshake(fd, sershkp);
    return retval;
#else

#if defined(TERMIO)
    struct termio settings;
#else
    struct sgttyb sgttyb;
    struct tchars tchars;
#endif
    OSErr err;

    err = noErr;
#if defined(TERMIO)
    if(ioctl(fd, TCGETA, &settings) < 0)
        err = ROMlib_maperrno();
#else
    if(ioctl(fd, TIOCGETP, &sgttyb) < 0 || ioctl(fd, TIOCGETC, &tchars) < 0)
        err = ROMlib_maperrno();
#endif
    else
    {
        if(sershkp->fXOn)
        {
#if defined(TERMIO)
            settings.c_iflag |= IXON;
        }
        else
            settings.c_iflag &= ~IXON;
#else
            /* NB: LDECCTQ is left (un)set in the local mode word. */
            tchars.t_startc = -1;
            tchars.t_stopc = -1;
        }
        else
        {
            tchars.t_startc = sershkp->xOn;
            tchars.t_stopc = sershkp->xOff;
        }
#endif
        if(sershkp->fInX)
        {
#if defined(TERMIO)
            settings.c_iflag |= IXOFF;
        }
        else
            settings.c_iflag &= ~IXOFF;
        err = ioctl(fd, TCSETAW, &settings) < 0 ? ROMlib_maperrno() : noErr;
#else
            /* Can't enable TANDEM without setting startc & stopc. If I
	     * do that, input flow control is also enabled. This is what
	     * has been implemented here. The alternative is to refuse the
	     * request.
	     */
            sgttyb.sg_flags |= TANDEM;
            tchars.t_startc = sershkp->xOn;
            tchars.t_stopc = sershkp->xOff;
        }
        else
            sgttyb.sg_flags &= ~TANDEM;
        if(ioctl(fd, TIOCSETP, &sgttyb) < 0 || ioctl(fd, TIOCSETC, &tchars) < 0)
            err = ROMlib_maperrno();
#endif
    }
    return err;
#endif
}

static OSErr setbaud(LONGINT fd, INTEGER baud)
{
#if defined(MSDOS) || defined(CYGWIN32) || defined(WIN32)
    OSErr retval;

    retval = serial_bios_setbaud(fd, baud);
    return retval;
#else

#if defined(TERMIO)
    struct termio settings;
#else
    struct sgttyb sgttyb;
#endif
    static INTEGER rates[] = {
        50, 75, 110, 134, 150, 200, 300,
        600, 1200, 1800, 2400, 4800, 9600, 19200,
        (INTEGER)38400, (INTEGER)32767,
    },
                   *ip;
    OSErr err;

    for(ip = rates; *ip < baud; ip++)
        ;
    if(*ip == 32767 || ip[0] - baud > baud - ip[-1])
        --ip;
#if defined(TERMIO)
    if(ioctl(fd, TCGETA, &settings) < 0)
#else
    if(ioctl(fd, TIOCGETP, &sgttyb) < 0)
#endif
        err = ROMlib_maperrno();
    else
    {
#if defined(TERMIO)
        settings.c_cflag &= ~CBAUD;
        settings.c_cflag |= ip - rates + 1;
        err = ioctl(fd, TCSETAW, &settings) < 0 ? ROMlib_maperrno() : noErr;
#else
        sgttyb.sg_ispeed = sgttyb.sg_ospeed = ip - rates + 1;
        err = ioctl(fd, TIOCGETP, &sgttyb) < 0 ? ROMlib_maperrno() : noErr;
#endif
    }
    return err;
#endif
}

static OSErr ctlbrk(LONGINT fd, INTEGER flag)
{
#if defined(MSDOS) || defined(CYGWIN32) || defined(WIN32)
    OSErr retval;

    retval = serial_bios_ctlbrk(fd, flag);
    return retval;
#else
    OSErr err;

#if defined(TERMIO)
    if(flag == SER_START)
    {
        /* NOTE:  This will send a break for 1/4 sec */
        err = ioctl(fd, TCSBRK, 0);
    }
    else
        err = noErr;
#else
    err = ioctl(fd, flag == SER_START ? TIOCSBRK : TIOCCBRK, 0);
#endif
    return err < 0 ? ROMlib_maperrno() : noErr;
#endif
}

static OSErr flow(LONGINT fd, LONGINT flag)
{
#if defined(MSDOS) || defined(CYGWIN32) || defined(WIN32)
    OSErr retval;

    retval = serial_bios_setflow(fd, flag);
    return retval;
#else
    OSErr err;

#if defined(TERMIO)
#if !defined(__alpha)
    err = ioctl(fd, TCXONC, flag == SER_STOP ? 0 : 1);
#else
    err = ioctl(fd, TCXONC, (void *)(flag == SER_STOP ? 0L : 1L));
#endif
#else
    err = ioctl(fd, flag == SER_STOP ? TIOCSTOP : TIOCSTART, 0);
#endif
    return err < 0 ? ROMlib_maperrno() : noErr;
#endif
}

#define SERBAUDRATE 13

#define SERXHSHAKE 14 /* IMIV-226 */
#define SERMISC 16 /* IMIV-226 */
#define SERSETDTR 17 /* IMIV-226 */
#define SERCLRDTR 18 /* IMIV-226 */

#define SERCHAR 19

#define SERXCHAR 20 /* IMIV-226 */

#define SERUSETXOFF 21
#define SERUCLRXOFF 22
#define SERCXMITXON 23
#define SERUXMITXON 24
#define SERCXMITXOFF 25
#define SERUXMITXOFF 26
#define SERRESET 27

#define SERKILLIO 1

OSErr Executor::C_ROMlib_serialctl(ParmBlkPtr pbp, DCtlPtr dcp) /* INTERNAL */
{
    OSErr err;
    hiddenh h;
    char c;

    if(!(dcp->dCtlFlags & OPENBIT))
        err = notOpenErr;
    else
    {
#if defined(SERIALDEBUG)
        warning_trace_info("serial control code = %d, param0 = 0x%x",
                           (LONGINT)pbp->cntrlParam.csCode,
                           (LONGINT)(unsigned short)pbp->cntrlParam.csParam[0]);
#endif
        h = (hiddenh)dcp->dCtlStorage;
        switch(pbp->cntrlParam.csCode)
        {
            case SERKILLIO:
                err = noErr; /* All I/O done synchronously */
                break;
            case SERSET:
                err = serset((*h)->fd, pbp->cntrlParam.csParam[0]);
                break;
            case SERSETBUF:
                err = noErr; /* ignored */
                break;
            case SERHSHAKE:
            case SERXHSHAKE: /* NOTE:  DTR handshake isn't supported  */
                err = serxhshake((*h)->fd, (SerShk *)pbp->cntrlParam.csParam);
                break;
            case SERSETBRK:
                err = ctlbrk((*h)->fd, SER_START);
                break;
            case SERCLRBRK:
                err = ctlbrk((*h)->fd, SER_STOP);
                break;
            case SERBAUDRATE:
                err = setbaud((*h)->fd, pbp->cntrlParam.csParam[0]);
                break;
            case SERMISC:
#if defined(MSDOS) || defined(CYGWIN32) || defined(WIN32)
                err = serial_bios_setdtr((*h)->fd);
#else
                err = controlErr; /* not supported */
#endif
                break;
            case SERSETDTR:
#if defined(MSDOS) || defined(CYGWIN32) || defined(WIN32)
                err = serial_bios_clrdtr((*h)->fd);
#else
                err = controlErr; /* not supported */
#endif
                break;
            case SERCLRDTR:
                err = controlErr; /* not supported */
                break;
            case SERCHAR:
                err = controlErr; /* not supported */
                break;
            case SERXCHAR:
                err = controlErr; /* not supported */
                break;
            case SERUSETXOFF:
                err = flow((*h)->fd, SER_STOP);
                break;
            case SERUCLRXOFF:
                err = flow((*h)->fd, SER_START);
                break;
            case SERCXMITXON:
                err = controlErr; /* not supported */
                break;
            case SERUXMITXON:
                c = XONC;
                err = write((*h)->fd, &c, 1) != 1 ? ROMlib_maperrno() : noErr;
                break;
            case SERCXMITXOFF:
                err = controlErr; /* not supported */
                break;
            case SERUXMITXOFF:
                c = XOFFC;
                err = write((*h)->fd, &c, 1) != 1 ? ROMlib_maperrno() : noErr;
                break;
            case SERRESET:
                err = controlErr; /* not supported */
                break;
            default:
                err = controlErr;
                break;
        }
    }
#if defined(SERIALDEBUG)
    warning_trace_info("serial control returning %d", (LONGINT)err);
#endif
    DOCOMPLETION(pbp, err);
}

/*
 * NOTE:  SERSTATUS lies about everything except rdPend.
 */

OSErr Executor::C_ROMlib_serialstatus(ParmBlkPtr pbp, DCtlPtr dcp) /* INTERNAL */
{
    OSErr err;
    hiddenh h;
    LONGINT n;

    if(!(dcp->dCtlFlags & OPENBIT))
        err = notOpenErr;
    else
    {
#if defined(SERIALDEBUG)
        warning_trace_info("serial status csCode = %d", (LONGINT)pbp->cntrlParam.csCode);
#endif
        h = (hiddenh)dcp->dCtlStorage;
        switch(pbp->cntrlParam.csCode)
        {
            case SERGETBUF:
#if defined(LINUX) || defined(MACOSX)
                if(ioctl((*h)->fd, FIONREAD, &n) < 0)
#else
                if(serial_bios_fionread((*h)->fd, &n) < 0)
#endif
                    err = ROMlib_maperrno();
                else
                {
#if defined(SERIALDEBUG)
                    warning_trace_info("serial status getbuf = %d", (LONGINT)n);
#endif
                    *(GUEST<LONGINT> *)pbp->cntrlParam.csParam = n;
                    err = noErr;
                }
                break;
            case SERSTATUS:
#if defined(LINUX) || defined(MACOSX)
                if(ioctl((*h)->fd, FIONREAD, &n) < 0)
#else
                if(serial_bios_fionread((*h)->fd, &n) < 0)
#endif
                    err = ROMlib_maperrno();
                else
                {
                    ((SerStaRec *)pbp->cntrlParam.csParam)->cumErrs = 0;
                    ((SerStaRec *)pbp->cntrlParam.csParam)->xOffSent = 0;
                    ((SerStaRec *)pbp->cntrlParam.csParam)->rdPend = n > 0 ? 1 : 0;
                    ((SerStaRec *)pbp->cntrlParam.csParam)->wrPend = 0;
                    ((SerStaRec *)pbp->cntrlParam.csParam)->ctsHold = 0;
                    ((SerStaRec *)pbp->cntrlParam.csParam)->xOffHold = 0;
                    err = noErr;
                }
                break;
            default:
                err = statusErr;
                break;
        }
    }
#if defined(SERIALDEBUG)
    warning_trace_info("serial status returning %d", (LONGINT)err);
#endif
    DOCOMPLETION(pbp, err);
}

static void restorecloseanddispose(hiddenh h)
{
#if defined(LINUX) || defined(MACOSX)
#if defined(TERMIO)
    ioctl((*h)->fd, TCSETAW, &(*h)->state);
#else
    ioctl((*h)->fd, TIOCSETP, &(*h)->sgttyb);
    ioctl((*h)->fd, TIOCSETC, &(*h)->tchars);
    ioctl((*h)->fd, TIOCLSET, &(*h)->lclmode);
#endif
    close((*h)->fd);
#endif
    DisposeHandle((Handle)h);
}

OSErr Executor::C_ROMlib_serialclose(ParmBlkPtr pbp, DCtlPtr dcp) /* INTERNAL */
{
    OSErr err;
    hiddenh h;

    if(dcp->dCtlFlags & OPENBIT)
    {
        h = (hiddenh)dcp->dCtlStorage;
        restorecloseanddispose(h);
        dcp->dCtlFlags &= ~OPENBIT;
        err = noErr;
    }
    else
        err = notOpenErr;
#if defined(SERIALDEBUG)
    warning_trace_info("serial close returning %d", (LONGINT)err);
#endif
    DOCOMPLETION(pbp, err);
}
