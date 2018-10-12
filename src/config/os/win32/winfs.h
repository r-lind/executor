#if !defined(_winfs_h_)
#define _winfs_h_

extern int sync(void);
extern int link(const char *oldpath, const char *newpath);
extern char *getwd(char *buf);
extern int fsync(int fd);

#endif /* !defined (_winfs_h_) */
