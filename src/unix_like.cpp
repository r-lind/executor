#include <base/common.h>

/*
 * This file is a quick hack to hoist code from config/os/linux to where it
 * can be shared with the Mac OS X port.
 *
 * Eventually everything should be rejiggered to use the GNU build system.
 */

#if defined(LINUX) || defined(MACOSX)

#include <sys/types.h>
#include <sys/mman.h>

#include <rsys/os.h>
#include <mman/memsize.h>
#include <mman/mman.h>
#include <error/system_error.h>
#include <base/lowglobals.h>

#include <Gestalt.h>
#include <SegmentLdr.h>

#include <rsys/lockunlock.h>

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <algorithm>

using namespace Executor;

static void
my_fault_proc(int sig)
{
    // FIXME:  Change this to an internal Executor dialog
    fprintf(stderr, "Unexpected Application Failure\n");

    // If we are already in the browser, does this exit the program?
    C_ExitToShell();
}

static int except_list[] = { SIGSEGV, SIGBUS, 0 };

void install_exception_handler(void)
{
    int i;

    for(i = 0; except_list[i]; ++i)
    {
        signal(except_list[i], my_fault_proc);
    }
}

void uninstall_exception_handler(void)
{
    int i;

    for(i = 0; except_list[i]; ++i)
    {
        signal(except_list[i], SIG_DFL);
    }
}

bool Executor::os_init(void)
{
#if defined(SDL)
    install_exception_handler();
#endif
    return true;
}

int
Executor::ROMlib_lockunlockrange(int fd, uint32_t begin, uint32_t count, lockunlock_t op)
{
    int retval;
    struct flock flock;

    warning_trace_info("fd = %d, begin = %d, count = %d, op = %d",
                       fd, begin, count, op);
    retval = noErr;
    switch(op)
    {
        case lock:
            flock.l_type = F_WRLCK;
            break;
        case unlock:
            flock.l_type = F_UNLCK;
            break;
        default:
            warning_unexpected("op = %d", op);
            retval = paramErr;
            break;
    }

    if(retval == noErr)
    {
        bool success;

        flock.l_whence = SEEK_SET;
        flock.l_start = begin;
        flock.l_len = count;

        success = fcntl(fd, F_SETLK, &flock) != -1;
        if(success)
            retval = noErr;
        else
        {
            switch(errno)
            {
                case EAGAIN:
                case EACCES:
                    retval = fLckdErr;
                    break;
#if 0
	    case ERROR_NOT_LOCKED:
	      retval = afpRangeNotLocked;
	      break;
#endif
#if 0
	    case ERROR_LOCK_FAILED:
	      retval = afpRangeOverlap;
	      break;
#endif
                default:
                    warning_unexpected("errno = %d", errno);
                    retval = noErr;
                    break;
            }
        }
    }
    return retval;
}

int
ROMlib_launch_native_app(int n_filenames, char **filenames)
{
    char **v;

    v = (char **)alloca(sizeof *v * (n_filenames + 1));
    memcpy(v, filenames, n_filenames * sizeof *v);
    v[n_filenames] = 0;
    if(fork() == 0)
        execv(filenames[0], v);

    return 0;
}

#endif /* defined (LINUX) || defined (MACOSX) */
