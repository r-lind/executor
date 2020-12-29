#include <base/common.h>

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "sdlquit.h"
#include <prefs/options.h>
#include <rsys/toolevent.h>

using namespace Executor;

#if defined(_WIN32)

static bool
os_specific_really_quit(void)
{
    return MessageBox(nullptr, "Terminate application?", "Terminate?", MB_OKCANCEL)
        == IDOK;
}

#else

static bool
os_specific_really_quit(void)
{
    return true;
}

#endif

/* Query the user as to whether we should really quit */
int sdl_really_quit(void)
{
    int retval;

    if(!(ROMlib_options & ROMLIB_CLOSE_IS_QUIT_BIT))
        retval = os_specific_really_quit();
    else
    {
        ROMlib_send_quit();
        retval = false;
    }

    return retval;
}
