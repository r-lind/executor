#if !defined(_RSYS_PARSE_H_)
#define _RSYS_PARSE_H_

/*
 * Copyright 1995 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

#include "rsys/options.h"

extern int yyparse(void); /* ick -- that's what yacc produces */
namespace Executor
{
extern void ROMlib_SetTitle(const char *name);
extern char *ROMlib_GetTitle(void);
extern void ROMlib_FreeTitle(char *title);
}
#endif
