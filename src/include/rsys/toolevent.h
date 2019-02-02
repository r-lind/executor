#if !defined(_TOOLEVENT_H_)
#define _TOOLEVENT_H_

/*
 * Copyright 1998 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

namespace Executor
{
extern void dofloppymount(void);
extern BOOLEAN ROMlib_beepedonce;
extern void ROMlib_send_quit(void);

extern int ROMlib_right_button_modifier;   /* in parse.ypp */
}

#endif /* !_TOOLEVENT_H_ */
