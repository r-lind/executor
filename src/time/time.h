#if !defined(_RSYS_TIME_H_)
#define _RSYS_TIME_H_

/*
 * Copyright 1995 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */


class PowerCore;

namespace Executor
{
unsigned long msecs_elapsed(void);

extern QHdr ROMlib_timehead;

syn68k_addr_t catchalarm(syn68k_addr_t pc, void *unused);
void catchalarmPowerPC(PowerCore&);
}

#endif /* _RSYS_TIME_H_ */
