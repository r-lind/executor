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

#include <base/common.h>
#include <rsys/slash.h>
#include <unistd.h>

using namespace Executor;

#if defined(_WIN32)
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

FILE *Ufopen(const char *path, const char *type)
{
    path = DOUBLE_SLASH_REMOVE(path);
    return fopen(path, type);
}

int Ustat(const char *path, struct stat *buf)
{
    int retval;

    path = DOUBLE_SLASH_REMOVE(path);
    retval = stat(path, buf);
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
    return retval;
}

int Uclose(int fd)
{
    int retval;

    retval = close(fd);
    return retval;
}
