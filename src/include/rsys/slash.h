#if !defined(_SLASH_H_)
#define _SLASH_H_


#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>


#define Uaccess access
#define Ufopen fopen
#define Ustat stat
#define Uopen open
#define Uclose closse

#ifndef O_BINARY
#define O_BINARY 0
#endif

#endif /* !_SLASH_H_ */
