#if !defined(_RSYS_TIME_H_)
#define _RSYS_TIME_H_

/*
 * Copyright 1995 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

extern unsigned long msecs_elapsed(void);

class PowerCore;

namespace Executor
{
extern QHdr ROMlib_timehead;

extern syn68k_addr_t catchalarm(syn68k_addr_t pc, void *unused);
extern void catchalarmPowerPC(PowerCore&);
}

#endif /* _RSYS_TIME_H_ */
