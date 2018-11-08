namespace Executor
{
/* false if we are blitting straight to screen memory, true if we are
   blitting to a shadow screen */
extern bool ROMlib_shadow_screen_p;

/* 0 means "use default". */
extern int flag_width, flag_height;

/* 0 means "use default". */
extern int flag_bpp;

extern bool flag_grayscale;

/* Approximate command line; argv[] elements separated by spaces. */
extern const char *ROMlib_command_line;

/* 0 means try running browser, 1 means don't */
extern bool ROMlib_nobrowser;

/* true if there is a version skew between the system file version and
   the required system version.  set by `InitResources ()', and used
   by `InitWindows ()' */
extern bool system_file_version_skew_p;
}
