#if !defined(_SLASH_H_)
#define _SLASH_H_

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#if defined(_WIN32)

extern int Uaccess(const char *path, int mode);
extern FILE *Ufopen(const char *path, const char *type);
extern int Uopen(const char *path, int flags, int mode);
extern int Uclose(int fd);

extern int Ustat(const char *path, struct stat *buf);

#else


#define Uaccess access
#define Ufopen fopen
#define Ustat stat

extern int Uopen(const char *path, int flags, int mode);
extern int Uclose(int fd);

#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#endif /* !_SLASH_H_ */
