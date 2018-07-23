/* Copyright 1994, 1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/*
 * The DOS file manipulation routines don't like to see "//", which is
 * a pain for us because we like the format of a file to be the filesystem
 * (volume) name, a slash, and then the rest of the path.  That means that
 * anything that is in the root directory gets a leading slash.  Stubbing
 * out these routines is much easier than rewriting the other routines.
 */

#include "rsys/common.h"
#include "rsys/lockunlock.h"
#include "rsys/slash.h"

using namespace Executor;

#if defined(WIN32)
#include "winfs.h"
#include "win_stat.h"

#define DOUBLE_SLASH_REMOVE(str)                                      \
    ({                                                                \
        char *retval, *op;                                            \
        const char *ip;                                               \
        bool last_was_slash_p;                                        \
                                                                      \
        /* char *from;                                                \
        asm ("movl %%ebp, %0" : "=g" (from));                         \
        from = *(char **)(from+4);                                    \
        warning_unexpected ("name = \"%s\" from = %p", str, from); */ \
        retval = (char *)alloca(strlen(str) + 1);                     \
        last_was_slash_p = false;                                     \
        for(ip = str, op = retval; *ip; ++ip)                         \
        {                                                             \
            if(*ip != '/' || !last_was_slash_p)                       \
                *op++ = *ip;                                          \
            last_was_slash_p = *ip == '/';                            \
        }                                                             \
        *op = 0;                                                      \
        retval;                                                       \
    })

int Uaccess(const char *path, int mode)
{
    path = DOUBLE_SLASH_REMOVE(path);
    return access(path, mode);
}

int Uchdir(const char *path)
{
    path = DOUBLE_SLASH_REMOVE(path);
    return chdir(path);
}

FILE *Ufopen(const char *path, const char *type)
{
    path = DOUBLE_SLASH_REMOVE(path);
    return fopen(path, type);
}

DIR *Uopendir(const char *path)
{
    path = DOUBLE_SLASH_REMOVE(path);
    return opendir((char *)path);
}

int Ustat(const char *path, struct stat *buf)
{
    int retval;

    path = DOUBLE_SLASH_REMOVE(path);
    retval = stat(path, buf);
    if(strlen(path) == 2 && path[1] == ':')
    {
#if defined(WIN32)
        buf->st_ino = 0;
        buf->st_rdev = 1;
#else
        buf->st_ino = 1;
#endif
    }
#if defined(WIN32)
    else
    {
        uint32_t ino;

        ino = ino_from_name(path);
        buf->st_ino = ino >> 16; /* See definition of ST_INO */
        buf->st_rdev = ino;
    }
#endif
    return retval;
}

#else

#define DOUBLE_SLASH_REMOVE(path) path

#endif

int Uopen(const char *path, int flags, int mode)
{
    int retval;

    path = DOUBLE_SLASH_REMOVE(path);
    retval = open(path, flags, mode);
    ROMlib_fd_clear_locks_after_open(retval, true);
    return retval;
}

int Uclose(int fd)
{
    int retval;

    ROMlib_fd_release_locks_for_close(fd);
    retval = close(fd);
    return retval;
}
