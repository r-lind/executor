#if !defined(__OS_MACOSX_H_)
#define __OS_MACOSX_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dirent.h>
#include <sys/param.h>
#include <sys/errno.h>

#include <signal.h>

#include <stdint.h>


#if !defined(MACOSX)
#define MACOSX
#endif

#if !defined(O_BINARY)
#define O_BINARY 0
#endif
#if !defined(REINSTALL_SIGNAL_HANDLER)
/* define `REINSTALL_SIGNAL_HANDLER' if signal handlers are
   de-installed after the signals occur, and require reinstallation */
#define REINSTALL_SIGNAL_HANDLER
#endif /* !REINSTALL_SIGNAL_HANDLER */

/* These functions don't exist in the math library, so use some
 * approximately correct versions of our own.
 */
#define NEED_SCALB
#define NEED_LOGB

#define HAVE_MMAP

#define CONFIG_OFFSET_P 1 /* Use offset memory, at least for the first port */

extern int ROMlib_launch_native_app(int n_filenames, char **filenames);

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/disk.h>

#endif /* !defined(__OS_MACOSX_H_) */
