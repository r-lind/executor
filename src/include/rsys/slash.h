#if !defined(_SLASH_H_)
#define _SLASH_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)

extern int Uaccess(const char *path, int mode);
extern FILE *Ufopen(const char *path, const char *type);
extern int Uopen(const char *path, int flags, int mode);
extern int Uclose(int fd);
extern DIR *Uopendir(const char *path);

extern int Ustat(const char *path, struct stat *buf);

#else /* !MSDOS && !defined (CYGWIN32) */

#define Uaccess access
#define Ufopen fopen
#define Ulstat lstat
#define Uopendir opendir
#define Ustat stat

extern int Uopen(const char *path, int flags, int mode);
extern int Uclose(int fd);

#endif /* !MSDOS && !defined (CYGWIN32) */

#ifdef __cplusplus
}
#endif

#endif /* !_SLASH_H_ */
