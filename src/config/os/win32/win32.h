#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <dirent.h>
#include <io.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#ifndef _MSC_VER
#include <unistd.h>
//#include <sys/vfs.h>
#include <sys/param.h>
//#include <sys/errno.h>
#endif

typedef int ssize_t;

#include <malloc.h>