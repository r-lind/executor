#if !defined(_RSYS_OPTIONS_H_)
#define _RSYS_OPTIONS_H_

/*
 * Copyright 1995 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

#include <stdint.h>
#include <utility>
#include <string>

namespace Executor
{
extern int32_t ROMlib_options;
extern std::string ROMlib_WindowName;
extern std::string ROMlib_Comments;

extern int ROMlib_AppleChar;

extern int ROMlib_desired_bpp;


enum
{
    ROMLIB_NOCLOCK_BIT = (1 << 0),
    ROMLIB_REFRESH_BIT = (1 << 2),
    ROMLIB_PRETENDSOUND_BIT = (1 << 8),
    ROMLIB_PASSPOSTSCRIPT_BIT = (1 << 9),
    ROMLIB_NEWLINETOCR_BIT = (1 << 10),
    ROMLIB_DIRECTDISKACCESS_BIT = (1 << 11),

    ROMLIB_SOUNDOFF_BIT = (1 << 13),
    ROMLIB_SOUNDON_BIT = (1 << 14),
    ROMLIB_NOWARN32_BIT = (1 << 15),
    ROMLIB_FLUSHOFTEN_BIT = (1 << 16),
    ROMLIB_PRETEND_HELP_BIT = (1 << 17),
    ROMLIB_PRETEND_EDITION_BIT = (1 << 18),
    ROMLIB_PRETEND_SCRIPT_BIT = (1 << 19),
    ROMLIB_PRETEND_ALIAS_BIT = (1 << 20),

    ROMLIB_TEXT_DISABLE_BIT = (1 << 21),
    ROMLIB_RECT_SCREEN_BIT = (1 << 28),
};
}
#endif
