/* Copyright 1994, 1995, 1996 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */


#include "x.h"
#include <base/common.h>
#include <QuickDraw.h>
#include <ScrapMgr.h>
#include <OSUtil.h>

#include <vdriver/dirtyrect.h>
#include <rsys/keyboard.h>
#include <rsys/scrap.h>

#include <assert.h>

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>

#include <X11/X.h>
#include <X11/Xlib.h>
/* declares a data type `Region' */
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

#include <CQuickDraw.h>
#include <MemoryMgr.h>
#include <OSEvent.h>
#include <EventMgr.h>
#include <ToolboxEvent.h>
#include <rsys/adb.h>

#include <quickdraw/cquick.h>
#include <prefs/prefs.h>
#include <mman/mman.h>
#include <vdriver/refresh.h>
#include <vdriver/vdriver.h>
#include <syn68k_public.h>
#include <base/m68kint.h>
#include <quickdraw/depthconv.h>
#include <quickdraw/rgbutil.h>
#include <commandline/option.h>
#include <commandline/flags.h>
#include <osevent/osevent.h>

#include "x_keycodes.h"

static int use_scan_codes = false;

namespace Executor {
        // FIXME: not including rsys/paths.h because that pulls in boost/filesystem, which CMake doesn't know about yet
        // (the dependencies are the wrong way aroudn anyway)
    extern std::string ROMlib_appname;
}

using namespace Executor;

/* save the original sigio flag; used when we shutdown */
static int orig_sigio_flag;
static int orig_sigio_owner;
static int x_fd = -1;

#define EXECUTOR_WINDOW_EVENT_MASK (KeyPressMask | KeyReleaseMask         \
                                    | ButtonPressMask | ButtonReleaseMask \
                                    | FocusChangeMask   \
                                    | ExposureMask | PointerMotionMask    \
                                    | ColormapChangeMask)

void cursor_init(void);

/* visual we are using the for x_window */
XVisualInfo *visual;

/* x window being driven */
static Window x_window;

static XImage *x_x_image;
static unsigned char *x_fbuf;
static int x_fbuf_bpp;
static int x_fbuf_row_bytes;

static int (*x_put_image)(Display *x_dpy, Window x_window, GC copy_gc,
                          XImage *x_image, int src_x, int src_y, int dst_x,
                          int dst_y, unsigned int width,
                          unsigned int height)
    = nullptr;

/* max dimensions */
static int max_width, max_height, max_bpp;

/* x cursor stuff */
static ::Cursor x_hidden_cursor, x_cursor = -1;

static XImage *x_image_cursor_data, *x_image_cursor_mask;
static Pixmap pixmap_cursor_data, pixmap_cursor_mask;

static GC copy_gc, cursor_data_gc, cursor_mask_gc;

static int cursor_visible_p = false;

static Atom x_selection_atom;

static char *selectiontext = nullptr;
static int selectionlength;

/* true if we should turn of autorepeat when the pointer is the
   executor window */
static int frob_autorepeat_p = false;

static XrmOptionDescRec opts[] = {
    { "-synchronous", ".synchronous", XrmoptionNoArg, "off" },
    { "-geometry", ".geometry", XrmoptionSepArg, 0 },
    { "-privatecmap", ".privateColormap", XrmoptionNoArg, "on" },
    { "-truecolor", ".trueColor", XrmoptionNoArg, "on" },
    { "-scancodes", ".scancodes", XrmoptionNoArg, "on" },

/* options that are transfered from the x resource database to the
     `common' executor options database */
#define FIRST_COMMON_OPT 5
    { "-debug", ".debug", XrmoptionSepArg, 0 },
};

void X11VideoDriver::registerOptions(void)
{
    opt_register("vdriver", {
            {"synchronous", "run in synchronous mode", opt_no_arg},
            {"geometry", "specify the executor window geometry", opt_sep},
            {"privatecmap", "have executor use a private x colormap", opt_no_arg},
            {"truecolor", "have executor use a TrueColor visual", opt_no_arg},
            {"scancodes", "different form of key mapping (may be useful in "
                "conjunction with -keyboard)", opt_no_arg},
        });
}

static XrmDatabase xdb;

Display *x_dpy;
static int x_screen;

int get_string_resource(char *resource, char **retval)
{
    char *res_type, res_name[256], res_class[256];
    XrmValue v;

    sprintf(res_name, "executor.%s", resource);
    sprintf(res_class, "Executor.%s", resource);

    if(!XrmGetResource(xdb, res_name, res_class, &res_type, &v))
        return false;
    *retval = v.addr;
    return true;
}

int get_bool_resource(char *resource, int *retval)
{
    char *res_type, res_name[256], res_class[256];
    XrmValue v;

    sprintf(res_name, "executor.%s", resource);
    sprintf(res_class, "Executor.%s", resource);

    if(!XrmGetResource(xdb, res_name, res_class, &res_type, &v))
        return false;
    if(!strcasecmp(v.addr, "on")
       || !strcasecmp(v.addr, "true")
       || !strcasecmp(v.addr, "yes")
       || !strcasecmp(v.addr, "1"))
    {
        *retval = true;
        return true;
    }
    if(!strcasecmp(v.addr, "off")
       || !strcasecmp(v.addr, "false")
       || !strcasecmp(v.addr, "no")
       || !strcasecmp(v.addr, "0"))
    {
        *retval = false;
        return true;
    }

    /* FIXME: print warning */
    *retval = false;
    return true;
}

static int have_shm, shm_err = false;

static int shm_major_opcode;
static int shm_first_event;
static int shm_first_error;

int x_error_handler(Display *err_dpy, XErrorEvent *err_evt)
{
    if(err_evt->request_code == shm_major_opcode)
        shm_err = true;
    else
    {
        char error_text[256];
        char t[256];

        XGetErrorText(err_dpy, err_evt->error_code, error_text, 256);
        fprintf(stderr, "%s: XError `%d': %s\n", ROMlib_appname.c_str(),
                err_evt->error_code, error_text);

        sprintf(t, "%d", err_evt->request_code);
        XGetErrorDatabaseText(x_dpy, "XRequest", t, "",
                              error_text, sizeof error_text);
        fprintf(stderr, "%s\n", error_text);

        exit(EXIT_FAILURE);
    }
    /* what do x error handlers return? */
    return 0;
}

int x_normal_put_image(Display *x_dpy, Drawable x_window, GC copy_gc, XImage *x_image,
                       int src_x, int src_y, int dst_x, int dst_y,
                       unsigned int width, unsigned int height)
{
    int retval;

    retval = XPutImage(x_dpy, x_window, copy_gc, x_image,
                       src_x, src_y, dst_x, dst_y, width, height);
    /* flush the output buffers */
    XFlush(x_dpy);

    return retval;
}

int x_shm_put_image(Display *x_dpy, Drawable x_window, GC copy_gc, XImage *x_image,
                    int src_x, int src_y, int dst_x, int dst_y,
                    unsigned int width, unsigned int height)
{
    int retval;

    retval = XShmPutImage(x_dpy, x_window, copy_gc, x_image,
                          src_x, src_y, dst_x, dst_y, width, height, False);
    /* flush the output buffers */
    XFlush(x_dpy);

    return retval;
}

void alloc_x_image(int bpp, int width, int height,
                   int *row_bytes_return,
                   XImage **x_image_return, unsigned char **fbuf_return,
                   int *bpp_return)
{
    /* return value */
    /* assignments to shut up gcc */
    XImage *x_image = nullptr;
    int row_bytes = 0;
    unsigned char *fbuf = nullptr;
    int resultant_bpp = 0;

    if(have_shm)
    {
        XShmSegmentInfo *shminfo;

        /* note: this memory doesn't get reclaimed */
        shminfo = (XShmSegmentInfo *)malloc(sizeof *shminfo);
        memset(shminfo, '\0', sizeof *shminfo);

        x_image = XShmCreateImage(x_dpy, visual->visual,
                                  bpp, ZPixmap, (char *)0,
                                  shminfo, width, height);
        if(x_image)
        {
            row_bytes = x_image->bytes_per_line;
            resultant_bpp = x_image->bits_per_pixel;
            shminfo->shmid = shmget(IPC_PRIVATE,
                                    row_bytes * height,
                                    IPC_CREAT | 0777);
            if(shminfo->shmid < 0)
            {
                XDestroyImage(x_image);
                free(shminfo);
                x_image = 0;
            }
            else
            {
                fbuf = (unsigned char *)(shminfo->shmaddr = x_image->data
                                         = (char *)shmat(shminfo->shmid, 0, 0));
                if(fbuf == (unsigned char *)-1)
                {
                    /* do we need to delete the shmid here? */
                    free(shminfo);
                    XDestroyImage(x_image);
                    fbuf = nullptr;
                }
                else
                {
                    shminfo->readOnly = False;
                    XShmAttach(x_dpy, shminfo);
                    XSync(x_dpy, False);
                    if(shm_err)
                    {
                        /* do we need to delete the shmid here? */
                        free(shminfo);
                        XDestroyImage(x_image);
                        fbuf = nullptr;
                        /* reset the shm_err flag */
                        shm_err = false;
                    }
                    else
                    {
                        shmctl(shminfo->shmid, IPC_RMID, 0);
                        if(x_put_image == nullptr)
                            x_put_image = x_shm_put_image;
                        else if(x_put_image != x_shm_put_image)
                            gui_abort();
                    }
                }
            }
        }
    }

    if(fbuf == nullptr)
    {
        warning_unexpected("not using shared memory");
        row_bytes = ((width * 32 + 31) / 32) * 4;
        fbuf = (unsigned char *)calloc(row_bytes * height, 1);
        x_image = XCreateImage(x_dpy, visual->visual, bpp, ZPixmap, 0,
                               (char *)fbuf, width, height, 32, row_bytes);
        resultant_bpp = x_image->bits_per_pixel;
        if(x_put_image == nullptr)
            x_put_image = x_normal_put_image;
        else if(x_put_image != x_normal_put_image)
            gui_abort();
    }

        // FIXME: this should be host byte order
    x_image->byte_order = LSBFirst;
    x_image->bitmap_bit_order = LSBFirst;

    *fbuf_return = fbuf;
    *row_bytes_return = row_bytes;
    *x_image_return = x_image;
    *bpp_return = resultant_bpp;
}

#define LINEFD 0x36

#define R11 0x84
#define R13 0x85
#define R15 0x86
#define R7 0x87
#define R9 0x88
#define NOTKEY 0x89

static uint8_t latin_one_table_data[] = {
    MKV_PRINT_SCREEN,
    NOTKEY, /* 1 */
    NOTKEY, /* 2 */
    NOTKEY, /* 3 */
    NOTKEY, /* 4 */
    NOTKEY, /* 5 */
    NOTKEY, /* 6 */
    NOTKEY, /* 7 */
    NOTKEY, /* 8 */
    NOTKEY, /* 9 */
    NOTKEY, /* 10 */
    NOTKEY, /* 11 */
    NOTKEY, /* 12 */
    NOTKEY, /* 13 */
    NOTKEY, /* 14 */
    NOTKEY, /* 15 */
    NOTKEY, /* 16 */
    NOTKEY, /* 17 */
    NOTKEY, /* 18 */
    NOTKEY, /* 19 */
    NOTKEY, /* 20 */
    NOTKEY, /* 21 */
    NOTKEY, /* 22 */
    NOTKEY, /* 23 */
    NOTKEY, /* 24 */
    NOTKEY, /* 25 */
    NOTKEY, /* 26 */
    NOTKEY, /* 27 */
    NOTKEY, /* 28 */
    NOTKEY, /* 29 */
    NOTKEY, /* 30 */
    NOTKEY, /* 31 */
    MKV_SPACE, /* SPACE */
    MKV_1, /* ! */
    MKV_SLASH, /* ? */
    MKV_3, /* # */
    MKV_4, /* $ */
    MKV_5, /* % */
    MKV_7, /* & */
    MKV_TICK, /* ' */
    MKV_9, /* ( */
    MKV_0, /* ) */
    MKV_8, /* * */
    MKV_EQUAL, /* + */
    MKV_COMMA, /* , */
    MKV_MINUS, /* - */
    MKV_PERIOD, /* . */
    MKV_SLASH, /* / */
    MKV_0, /* 0 */
    MKV_1, /* 1 */
    MKV_2, /* 2 */
    MKV_3, /* 3 */
    MKV_4, /* 4 */
    MKV_5, /* 5 */
    MKV_6, /* 6 */
    MKV_7, /* 7 */
    MKV_8, /* 8 */
    MKV_9, /* 9 */
    MKV_SEMI, /* : */
    MKV_SEMI, /* ; */
    MKV_COMMA, /* < */
    MKV_EQUAL, /* = */
    MKV_PERIOD, /* > */
    MKV_SLASH, /* ? */
    MKV_2, /* @ */
    MKV_a, /* A */
    MKV_b, /* B */
    MKV_c, /* C */
    MKV_d, /* D */
    MKV_e, /* E */
    MKV_f, /* F */
    MKV_g, /* G */
    MKV_h, /* H */
    MKV_i, /* I */
    MKV_j, /* J */
    MKV_k, /* K */
    MKV_l, /* L */
    MKV_m, /* M */
    MKV_n, /* N */
    MKV_o, /* O */
    MKV_p, /* P */
    MKV_q, /* Q */
    MKV_r, /* R */
    MKV_s, /* S */
    MKV_t, /* T */
    MKV_u, /* U */
    MKV_v, /* V */
    MKV_w, /* W */
    MKV_x, /* X */
    MKV_y, /* Y */
    MKV_z, /* Z */
    MKV_LEFTBRACKET, /* [ */
    MKV_BACKSLASH, /* \ */
    MKV_RIGHTBRACKET, /* ] */
    MKV_6, /* ^ */
    MKV_MINUS, /* _ */
    MKV_BACKTICK, /* ` */
    MKV_a, /* a */
    MKV_b, /* b */
    MKV_c, /* c */
    MKV_d, /* d */
    MKV_e, /* e */
    MKV_f, /* f */
    MKV_g, /* g */
    MKV_h, /* h */
    MKV_i, /* i */
    MKV_j, /* j */
    MKV_k, /* k */
    MKV_l, /* l */
    MKV_m, /* m */
    MKV_n, /* n */
    MKV_o, /* o */
    MKV_p, /* p */
    MKV_q, /* q */
    MKV_r, /* r */
    MKV_s, /* s */
    MKV_t, /* t */
    MKV_u, /* u */
    MKV_v, /* v */
    MKV_w, /* w */
    MKV_x, /* x */
    MKV_y, /* y */
    MKV_z, /* z */
};

static uint8_t misc_table_data[] = {
    MKV_BACKSPACE, /* 8 back space */
    MKV_TAB, /* 9 tab */
    LINEFD, /* 10 line feed */
    MKV_NUMCLEAR, /* 11 clear */
    NOTKEY, /* 12 */
    MKV_RETURN, /* 13 return */
    NOTKEY, /* 14 */
    NOTKEY, /* 15 */
    NOTKEY, /* 16 */
    NOTKEY, /* 17 */
    NOTKEY, /* 18 */
    MKV_PAUSE, /* 19 pause */
    NOTKEY, /* 20 */
    NOTKEY, /* 21 */
    NOTKEY, /* 22 */
    NOTKEY, /* 23 */
    NOTKEY, /* 24 */
    NOTKEY, /* 25 */
    NOTKEY, /* 26 */
    MKV_ESCAPE, /* 27 escape */
    NOTKEY, /* 28 */
    NOTKEY, /* 29 */
    NOTKEY, /* 30 */
    NOTKEY, /* 31 */
    MKV_RIGHTCNTL, /* 32 multi-key character preface */
    NOTKEY, /* 33 kanji */
    NOTKEY, /* 34 */
    NOTKEY, /* 35 */
    NOTKEY, /* 36 */
    NOTKEY, /* 37 */
    NOTKEY, /* 38 */
    NOTKEY, /* 39 */
    NOTKEY, /* 40 */
    NOTKEY, /* 41 */
    NOTKEY, /* 42 */
    NOTKEY, /* 43 */
    NOTKEY, /* 44 */
    NOTKEY, /* 45 */
    NOTKEY, /* 46 */
    NOTKEY, /* 47 */
    NOTKEY, /* 48 */
    NOTKEY, /* 49 */
    NOTKEY, /* 50 */
    NOTKEY, /* 51 */
    NOTKEY, /* 52 */
    NOTKEY, /* 53 */
    NOTKEY, /* 54 */
    NOTKEY, /* 55 */
    NOTKEY, /* 56 */
    NOTKEY, /* 57 */
    NOTKEY, /* 58 */
    NOTKEY, /* 59 */
    NOTKEY, /* 60 */
    NOTKEY, /* 61 */
    NOTKEY, /* 62 */
    NOTKEY, /* 63 */
    NOTKEY, /* 64 */
    NOTKEY, /* 65 */
    NOTKEY, /* 66 */
    NOTKEY, /* 67 */
    NOTKEY, /* 68 */
    NOTKEY, /* 69 */
    NOTKEY, /* 70 */
    NOTKEY, /* 71 */
    NOTKEY, /* 72 */
    NOTKEY, /* 73 */
    NOTKEY, /* 74 */
    NOTKEY, /* 75 */
    NOTKEY, /* 76 */
    NOTKEY, /* 77 */
    NOTKEY, /* 78 */
    NOTKEY, /* 79 */
    MKV_HOME, /* 80 home */
    MKV_LEFTARROW, /* left arrow */
    MKV_UPARROW, /* up arrow */
    MKV_RIGHTARROW, /* right arrow */
    MKV_DOWNARROW, /* down arrow */
    MKV_PAGEUP, /* prior */
    MKV_PAGEDOWN, /* next */
    MKV_END, /* end */
    NOTKEY, /* 88 begin */
    NOTKEY, /* 89 */
    NOTKEY, /* 90 */
    NOTKEY, /* 91 */
    NOTKEY, /* 92 */
    NOTKEY, /* 93 */
    NOTKEY, /* 94 */
    NOTKEY, /* 95 */
    NOTKEY, /* 96 select */
    NOTKEY, /* print */
    NOTKEY, /* execute */
    MKV_HELP, /* 99 insert/help */
    NOTKEY, /* 100 */
    NOTKEY, /* 101 undo */
    NOTKEY, /* redo */
    NOTKEY, /* menu */
    NOTKEY, /* find */
    NOTKEY, /* cancel */
    MKV_HELP, /* help */
    NOTKEY, /* 107 break */
    NOTKEY, /* 108 */
    NOTKEY, /* 109 */
    NOTKEY, /* 110 */
    NOTKEY, /* 111 */
    NOTKEY, /* 112 */
    NOTKEY, /* 113 */
    NOTKEY, /* 114 */
    NOTKEY, /* 115 */
    NOTKEY, /* 116 */
    NOTKEY, /* 117 */
    NOTKEY, /* 118 */
    NOTKEY, /* 119 */
    NOTKEY, /* 120 */
    NOTKEY, /* 121 */
    NOTKEY, /* 122 */
    NOTKEY, /* 123 */
    NOTKEY, /* 124 */
    MKV_SCROLL_LOCK, /* 125 unassigned (but scroll lock remapped) */
    MKV_LEFTOPTION, /* 126 mode switch (also scroll lock) */
    MKV_NUMCLEAR, /* 127 num lock */
    NOTKEY, /* 128 key space */
    NOTKEY, /* 129 */
    NOTKEY, /* 130 */
    NOTKEY, /* 131 */
    NOTKEY, /* 132 */
    NOTKEY, /* 133 */
    NOTKEY, /* 134 */
    NOTKEY, /* 135 */
    NOTKEY, /* 136 */
    NOTKEY, /* 137 key tab */
    NOTKEY, /* 138 */
    NOTKEY, /* 139 */
    NOTKEY, /* 140 */
    MKV_NUMENTER, /* 141 key enter */
    NOTKEY, /* 142 */
    NOTKEY, /* 143 */
    NOTKEY, /* 144 */
    NOTKEY, /* 145 key f1 */
    NOTKEY, /* key f2 */
    NOTKEY, /* key f3 */
    NOTKEY, /* 148 key f4 */

    MKV_NUM7, /* 149 */
    MKV_NUM4, /* 150 */
    MKV_NUM8, /* 151 */
    MKV_NUM6, /* 152 */
    MKV_NUM2, /* 153 */
    MKV_NUM9, /* 154 */
    MKV_NUM3, /* 155 */
    MKV_NUM1, /* 156 */
    MKV_NUM5, /* 157 */
    MKV_NUM0, /* 158 */
    MKV_NUMPOINT, /* 159 */

    NOTKEY, /* 160 */
    NOTKEY, /* 161 */
    NOTKEY, /* 162 */
    NOTKEY, /* 163 */
    NOTKEY, /* 164 */
    NOTKEY, /* 165 */
    NOTKEY, /* 166 */
    NOTKEY, /* 167 */
    NOTKEY, /* 168 */
    NOTKEY, /* 169 */
    MKV_NUMDIVIDE, /* 170 key multiply */
    MKV_NUMPLUS, /* key plus */
    NOTKEY, /* key comma */
    MKV_NUMMULTIPLY, /* key minus */
    MKV_NUMPOINT, /* key decimal point */
    MKV_NUMEQUAL, /* key divide */
    MKV_NUM0, /* key 0 */
    MKV_NUM1, /* key 1 */
    MKV_NUM2, /* key 2 */
    MKV_NUM3, /* key 3 */
    MKV_NUM4, /* key 4 */
    MKV_NUM5, /* key 5 */
    MKV_NUM6, /* key 6 */
    MKV_NUM7, /* key 7 */
    MKV_NUM8, /* key 8 */
    MKV_NUM9, /* 185 */
    NOTKEY, /* 186 */
    NOTKEY, /* 187 */
    NOTKEY, /* 188 */
    MKV_NUMEQUAL, /* 189 key equals */
    MKV_F1, /* f1 */
    MKV_F2, /* f2 */
    MKV_F3, /* f3 */
    MKV_F4, /* f4 */
    MKV_F5, /* f5 */
    MKV_F6, /* f6 */
    MKV_F7, /* f7 */
    MKV_F8, /* f8 */
    MKV_F9, /* f9 */
    MKV_F10, /* f10 */
    MKV_F11, /* l1 */
    MKV_F12, /* l2 */
    MKV_F13, /* l3 */
    MKV_F14, /* l4 */
    MKV_F15, /* l5 */
    NOTKEY, /* l6 */ /* I don't know what these ones are */
    NOTKEY, /* l7 */
    NOTKEY, /* l8 */
    NOTKEY, /* l9 */
    NOTKEY, /* l10 */
    MKV_HELP, /* r1 */
    MKV_HOME, /* r2 */
    MKV_PAGEUP, /* r3 */
    MKV_DELFORWARD, /* r4 */
    MKV_END, /* r5 */
    MKV_PAGEDOWN, /* r6 */
    R7, /* r7 */
    MKV_UPARROW, /* r8 */
    R9, /* r9 */
    MKV_LEFTARROW, /* r10 */
    R11, /* r11 */
    MKV_RIGHTARROW, /* r12 */
    R13, /* r13 */
    MKV_DOWNARROW, /* r14 */
    R15, /* r15 */
    MKV_LEFTSHIFT, /* left shift */
    MKV_RIGHTSHIFT, /* right shift */
    MKV_LEFTCNTL, /* left control */
    MKV_RIGHTCNTL, /* right control */
    MKV_CAPS, /* caps lock */
    NOTKEY, /* shift lock */
    /* cliff wants left Alt/Meta key to be `clover', the right
     Alt/Meta key to be `alt/option'.  and so it is;

     don't change these unless you also change `COMMONSTATE' in x.c
     to agree with them */
    MKV_CLOVER, /* left meta */
    MKV_RIGHTOPTION, /* right meta */
    MKV_CLOVER, /* left alt */
    MKV_RIGHTOPTION, /* right alt */
    NOTKEY, /* left super */
    NOTKEY, /* right super */
    NOTKEY, /* left hyper */
    NOTKEY, /* 238 right hyper */
    NOTKEY, /* 239 */
    NOTKEY, /* 240 */
    NOTKEY, /* 241 */
    NOTKEY, /* 242 */
    NOTKEY, /* 243 */
    NOTKEY, /* 244 */
    NOTKEY, /* 245 */
    NOTKEY, /* 246 */
    NOTKEY, /* 247 */
    NOTKEY, /* 248 */
    NOTKEY, /* 249 */
    NOTKEY, /* 250 */
    NOTKEY, /* 251 */
    NOTKEY, /* 252 */
    NOTKEY, /* 253 */
    NOTKEY, /* 254 */
    MKV_DELFORWARD, /* 255 delete */
};

typedef struct key_table_data
{
    uint8_t high_byte;
    int min;
    int size;
    uint8_t *data;
} key_table_t;

key_table_t key_tables[] = {
    /* latin one table */
    { 0, 0, std::size(latin_one_table_data), latin_one_table_data },
    /* misc table */
    { 0xFF, 8, std::size(misc_table_data), misc_table_data },
    { 0, 0, 0, nullptr },
};

/* convert x keysym to mac virtual `keywhat'; return true the
   conversion was successful */

static bool
x_keysym_to_mac_virt(unsigned int keysym, unsigned char *virt_out)
{
    key_table_t *table;
    uint8_t keysym_high_byte, keysym_low_byte;
    int16_t mkvkey;
    int i;

    if(use_scan_codes)
    {
        if(keysym == 0xff)
            mkvkey = NOTKEY;
        else
            mkvkey = keysym;
    }
    else
    {
        keysym_high_byte = (keysym >> 8) & 0xFF;
        keysym_low_byte = keysym & 0xFF;

        /* switch off the high 8bits of the keysym to determine which table
	 to index into */
        for(i = 0; key_tables[i].data; i++)
        {
            table = &key_tables[i];
            if(table->high_byte == keysym_high_byte)
                break;
        }
        if(key_tables[i].data == nullptr)
            return false;

        if(keysym_low_byte < table->min
           || keysym_low_byte > (table->min + table->size))
            return false;

        mkvkey = table->data[keysym_low_byte - table->min];
    }

    if(mkvkey == NOTKEY)
        return false;

    mkvkey = ROMlib_right_to_left_key_map(mkvkey);

    *virt_out = mkvkey;
    return true;
}

static bool x_event_pending_p(void)
{
    fd_set fds;
    struct timeval no_wait;
    bool retval;

    FD_ZERO(&fds);
    FD_SET(x_fd, &fds);
    no_wait.tv_sec = no_wait.tv_usec = 0;
    retval = select(x_fd + 1, &fds, 0, 0, &no_wait) > 0;
    return retval;
}

static syn68k_addr_t
post_pending_x_events(syn68k_addr_t interrupt_addr, void *unused)
{
    XEvent evt;

    while(XCheckTypedEvent(x_dpy, SelectionRequest, &evt)
          || XCheckMaskEvent(x_dpy, ~0L, &evt))
    {
        switch(evt.type)
        {
            case SelectionRequest:
            {
                Atom property;
                XSelectionEvent xselevt;

                property = evt.xselectionrequest.property;
                if(property == None)
                    property = x_selection_atom;

                XChangeProperty(x_dpy, evt.xselectionrequest.requestor, property,
                                XA_STRING, (int)8, (int)PropModeReplace,
                                (unsigned char *)selectiontext, selectionlength);

                xselevt.type = SelectionNotify;
                xselevt.requestor = evt.xselectionrequest.requestor; /* check this */
                xselevt.selection = evt.xselectionrequest.selection;
                xselevt.target = evt.xselectionrequest.target;
                xselevt.property = property;
                xselevt.time = evt.xselectionrequest.time;

                XSendEvent(x_dpy, xselevt.requestor, False, 0L,
                           (XEvent *)&xselevt);

                break;
            }
            case KeyPress:
            case KeyRelease:
            {
                unsigned keysym;
                unsigned char virt;

                if(use_scan_codes)
                {
                    uint8_t keycode;

                    keycode = evt.xkey.keycode;
                    if(keycode < std::size(x_keycode_to_mac_virt))
                        keysym = x_keycode_to_mac_virt[keycode];
                    else
                        keysym = NOTKEY;
                }
                else
                {
                    keysym = XLookupKeysym(&evt.xkey, 0);

                    /* This hack is because the default is to map BACKSPACE to
		   the same key that DELETE produces just because people
		   don't like BACKSPACE generating a ^H ... yahoo! */

                    if(keysym == 0xFFFF && evt.xkey.keycode == 0x16)
                        keysym = 0xFF08;

                    /* This hack is because the default is to map Scroll Lock
		   and Right-Alt to the same key.  I'm not sure why */

                    if(keysym == 0xFF7E && evt.xkey.keycode == 0x4e)
                        keysym = 0xFF7D;
                }

                if(x_keysym_to_mac_virt(keysym, &virt))
                {
                    vdriver->callbacks_->keyboardEvent(evt.type == KeyPress, virt);
                }
                break;
            }
            case ButtonPress:
            case ButtonRelease:
            {
                vdriver->callbacks_->mouseButtonEvent(evt.type == ButtonPress, evt.xbutton.x, evt.xbutton.y);
                break;
            }
            case Expose:
                vdriver->updateScreen(evt.xexpose.y, evt.xexpose.x,
                                      evt.xexpose.y + evt.xexpose.height,
                                      evt.xexpose.x + evt.xexpose.width);
                break;
            case FocusIn:
                if(frob_autorepeat_p)
                    XAutoRepeatOff(x_dpy);
                {
                    bool cvt;
                    Window selection_owner;

                    selection_owner = XGetSelectionOwner(x_dpy, XA_PRIMARY);
                    cvt = selection_owner != None && selection_owner != x_window;
                    if(cvt)
                        ZeroScrap();
                    vdriver->callbacks_->resumeEvent(cvt);
                }
                break;
            case FocusOut:
                if(frob_autorepeat_p)
                    XAutoRepeatOn(x_dpy);
                vdriver->callbacks_->suspendEvent();
                break;
            case MotionNotify:
                vdriver->callbacks_->mouseMoved(evt.xmotion.x, evt.xmotion.y);
                break;
        }
    }
    return MAGIC_RTE_ADDRESS;
}

void x_event_handler(int signo)
{
    /* request syncint */
    interrupt_generate(M68K_EVENT_PRIORITY);
}

bool X11VideoDriver::parseCommandLine(int& argc, char *argv[])
{
    x_dpy = XOpenDisplay("");
    if(x_dpy == nullptr)
    {
        fprintf(stderr, "%s: could not open x server `%s'.\n",
                ROMlib_appname.c_str(), XDisplayName(""));
        exit(EXIT_FAILURE);
    }
    x_screen = XDefaultScreen(x_dpy);

    XSetErrorHandler(x_error_handler);

    /* determine if the server supports the `XShm' extension */
    have_shm = XQueryExtension(x_dpy, "MIT-SHM",
                               &shm_major_opcode,
                               &shm_first_event,
                               &shm_first_error);

    char *xdefs = XResourceManagerString(x_dpy);
    if(xdefs)
        xdb = XrmGetStringDatabase(xdefs);

    XrmParseCommand(&xdb, opts, std::size(opts), "Executor", &argc, argv);

    return true;
}

bool X11VideoDriver::init()
{
    int i;

    XVisualInfo *visuals, vistemplate;
    int n_visuals;
    int dummy_int;
    unsigned int geom_width, geom_height;
    char *geom;

    
    get_bool_resource("scancodes", &use_scan_codes);

    if(!get_string_resource("geometry", &geom))
        geom = 0;
    /* default */
    geom_width = VDRIVER_DEFAULT_SCREEN_WIDTH;
    geom_height = VDRIVER_DEFAULT_SCREEN_HEIGHT;
    if(geom)
    {
        /* override the maximum with possible geometry values */
        XParseGeometry(geom, &dummy_int, &dummy_int,
                       &geom_width, &geom_height);
    }
    max_width = std::max<int>(VDRIVER_DEFAULT_SCREEN_WIDTH,
                         (std::max<int>(geom_width, flag_width)));
    max_height = std::max<int>(VDRIVER_DEFAULT_SCREEN_HEIGHT,
                          std::max<int>(geom_height, flag_height));
    max_bpp = 32;

    vistemplate.screen = x_screen;
    vistemplate.depth = 8;

    visual = nullptr;


    vistemplate.screen = x_screen;
    vistemplate.c_class = TrueColor;

    visuals = XGetVisualInfo(x_dpy,
                                (VisualScreenMask | VisualClassMask),
                                &vistemplate, &n_visuals);

    for(i = 0; i < n_visuals; i++)
    {
        if(visuals[i].depth >= 24)
        {
            visual = &visuals[i];
            x_fbuf_bpp = visual->depth;
            break;
        }
    }

    if(visual == nullptr)
    {
        fprintf(stderr, "%s: no acceptable visual found, exiting.\n",
                ROMlib_appname.c_str());
        exit(EXIT_FAILURE);
    }

    alloc_x_image(x_fbuf_bpp, max_width, max_height,
                    &x_fbuf_row_bytes, &x_x_image, &x_fbuf,
                    &x_fbuf_bpp);

    if(max_bpp > 32)
        max_bpp = 32;
    if(max_bpp > x_fbuf_bpp)
        max_bpp = x_fbuf_bpp;

    maxBpp_ = max_bpp;


    cursor_init();

    x_selection_atom = XInternAtom(x_dpy, "ROMlib_selection", False);

    /* hook into syn68k synchronous interrupts */
    {
        syn68k_addr_t event_callback;

        event_callback = callback_install(post_pending_x_events, nullptr);
        *(GUEST<ULONGINT> *)SYN68K_TO_US(M68K_EVENT_VECTOR * 4) = (ULONGINT)event_callback;
    }

    /* set up the async x even handler */
    {
        x_fd = XConnectionNumber(x_dpy);

        struct sigaction sa;

        sa.sa_handler = x_event_handler;
        sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, SIGIO);
        sa.sa_flags = 0;

        sigaction(SIGIO, &sa, nullptr);

        fcntl(x_fd, F_GETOWN, &orig_sigio_owner);
        fcntl(x_fd, F_SETOWN, getpid());
        orig_sigio_flag = fcntl(x_fd, F_GETFL, 0) & ~FASYNC;
        fcntl(x_fd, F_SETFL, orig_sigio_flag | FASYNC);
    }

    return true;
}

void X11VideoDriver::flushDisplay(void)
{
    XFlush(x_dpy);
}

void X11VideoDriver::alloc_x_window(int width, int height, int bpp, bool grayscale_p)
{
    XSetWindowAttributes xswa;
    XSizeHints size_hints = {};
    XGCValues gc_values;

    char *geom;
    int geom_mask;
    int x = 0, y = 0;

    /* size and base size are set below after parsing geometry */
    size_hints.flags = PSize | PMinSize | PMaxSize | PBaseSize | PWinGravity;

    if(!width
       || !height)
    {
        geom = nullptr;
        get_string_resource("geometry", &geom);
        if(geom)
        {
            unsigned w, h;
            geom_mask = XParseGeometry(geom, &x, &y, &w, &h);
            if(geom_mask & WidthValue)
            {
                width_ = w;
                if(width_ < VDRIVER_MIN_SCREEN_WIDTH)
                    width_ = VDRIVER_MIN_SCREEN_WIDTH;
                else if(width_ > max_width)
                    width_ = max_width;

                size_hints.flags |= USSize;
            }
            if(geom_mask & HeightValue)
            {
                height_ = h;
                if(height_ < VDRIVER_MIN_SCREEN_HEIGHT)
                    height_ = VDRIVER_MIN_SCREEN_HEIGHT;
                else if(height_ > max_height)
                    height_ = max_height;
                size_hints.flags |= USSize;
            }
            if(geom_mask & XValue)
            {
                if(geom_mask & XNegative)
                    x = XDisplayWidth(x_dpy, x_screen) + x - width_;
                size_hints.flags |= USPosition;
            }
            if(geom_mask & YValue)
            {
                if(geom_mask & YNegative)
                    y = XDisplayHeight(x_dpy, x_screen) + y - height_;
                size_hints.flags |= USPosition;
            }
            switch(geom_mask & (XNegative | YNegative))
            {
                case 0:
                    size_hints.win_gravity = NorthWestGravity;
                    break;
                case XNegative:
                    size_hints.win_gravity = NorthEastGravity;
                    break;
                case YNegative:
                    size_hints.win_gravity = SouthWestGravity;
                    break;
                default:
                    size_hints.win_gravity = SouthEastGravity;
                    break;
            }
        }
    }
    else
    {
        /* size was specified; we aren't using defaults */
        size_hints.flags |= USSize;
        size_hints.win_gravity = NorthWestGravity;

        if(width < 512)
            width = 512;
        else if(width > max_width)
            width = max_width;

        if(height < 342)
            height = 342;
        else if(height > max_height)
            height = max_height;

        width_ = width;
        height_ = height;
    }

    size_hints.min_width = size_hints.max_width
        = size_hints.base_width = size_hints.width = width_;
    size_hints.min_height = size_hints.max_height
        = size_hints.base_height = size_hints.height = height_;

    size_hints.x = x;
    size_hints.y = y;

    /* #### allow command line options to select {16, 32}bpp modes, but
     don't make them the default since they still have problems */
    if(bpp == 0)
        bpp = std::min(max_bpp, 8);
    else if(bpp > max_bpp)
        bpp = max_bpp;

    bpp_ = bpp;

    isGrayscale_ = grayscale_p;

    /* create the executor window */
    x_window = XCreateWindow(x_dpy, XRootWindow(x_dpy, x_screen),
                             x, y, width_, height_,
                             0, 0, InputOutput, CopyFromParent, 0, &xswa);

    XDefineCursor(x_dpy, x_window, x_hidden_cursor);

    gc_values.function = GXcopy;
    gc_values.foreground = XBlackPixel(x_dpy, x_screen);
    gc_values.background = XWhitePixel(x_dpy, x_screen);
    gc_values.plane_mask = AllPlanes;

    copy_gc = XCreateGC(x_dpy, x_window,
                        (GCFunction | GCForeground
                         | GCBackground | GCPlaneMask),
                        &gc_values);

    {
        /* various hints for `XSetWMProperties ()' */
        XWMHints wm_hints;
        XClassHint class_hint;
        XTextProperty name;

        memset(&wm_hints, 0, sizeof wm_hints);

        class_hint.res_name = "executor";
        class_hint.res_class = "Executor";

        char *program_name = const_cast<char*>(ROMlib_appname.c_str());
        XStringListToTextProperty(&program_name, 1, &name);

        XSetWMProperties(x_dpy, x_window,
                         &name, &name, /* _argv, *_argc, */ nullptr, 0,
                         &size_hints, &wm_hints, &class_hint);
    }

    XSelectInput(x_dpy, x_window, EXECUTOR_WINDOW_EVENT_MASK);

    XMapRaised(x_dpy, x_window);
    XClearWindow(x_dpy, x_window);
    XFlush(x_dpy);
}

::Cursor
create_x_cursor(char *data, char *mask,
                int hotspot_x, int hotspot_y)
{
    ::Cursor retval;
    char x_mask[32], x_data[32];
    int i;

    static XColor x_black = { 0, 0, 0, 0 },
                  x_white = { (unsigned long)~0,
                              (unsigned short)~0,
                              (unsigned short)~0,
                              (unsigned short)~0 };

    for(i = 0; i < 32; i++)
    {
        x_mask[i] = data[i] | mask[i];
        x_data[i] = data[i];
    }

    x_image_cursor_data->data = x_data;
    x_image_cursor_mask->data = x_mask;

    XPutImage(x_dpy, pixmap_cursor_data, cursor_data_gc, x_image_cursor_data,
              0, 0, 0, 0, 16, 16);
    XPutImage(x_dpy, pixmap_cursor_mask, cursor_mask_gc, x_image_cursor_mask,
              0, 0, 0, 0, 16, 16);

    if(hotspot_x < 0)
        hotspot_x = 0;
    else if(hotspot_x > 16)
        hotspot_x = 16;

    if(hotspot_y < 0)
        hotspot_y = 0;
    else if(hotspot_y > 16)
        hotspot_y = 16;

    retval = XCreatePixmapCursor(x_dpy, pixmap_cursor_data, pixmap_cursor_mask,
                                 &x_black, &x_white,
                                 hotspot_x, hotspot_y);
    return retval;
}

void X11VideoDriver::setCursor(char *cursor_data,
                              uint16_t cursor_mask[16],
                              int hotspot_x, int hotspot_y)
{
    ::Cursor orig_x_cursor = x_cursor;

    x_cursor = create_x_cursor(cursor_data, (char *)cursor_mask,
                               hotspot_x, hotspot_y);

    /* if visible, set `x_cursor' to be the current cursor */
    if(cursor_visible_p)
        XDefineCursor(x_dpy, x_window, x_cursor);

    if(orig_x_cursor != (::Cursor)-1)
        XFreeCursor(x_dpy, orig_x_cursor);
}

bool X11VideoDriver::setCursorVisible(bool show_p)
{
    int orig_cursor_visible_p = cursor_visible_p;

    if(cursor_visible_p)
    {
        if(!show_p)
        {
            cursor_visible_p = false;
            XDefineCursor(x_dpy, x_window, x_hidden_cursor);
        }
    }
    else
    {
        if(show_p)
        {
            cursor_visible_p = true;
            XDefineCursor(x_dpy, x_window, x_cursor);
        }
    }

    return orig_cursor_visible_p;
}

void cursor_init(void)
{
    XGCValues gc_values;
    static char zero[2 * 16] = {
        0,
    };

    /* the following are used to create x cursors, they must
     be done before calling `create_x_cursor ()' */
    x_image_cursor_data = XCreateImage(x_dpy, XDefaultVisual(x_dpy, x_screen),
                                       1, XYBitmap, 0, nullptr, 16, 16, 8, 2);
    x_image_cursor_mask = XCreateImage(x_dpy, XDefaultVisual(x_dpy, x_screen),
                                       1, XYBitmap, 0, nullptr, 16, 16, 8, 2);

    x_image_cursor_data->byte_order = MSBFirst;
    x_image_cursor_mask->byte_order = MSBFirst;

    x_image_cursor_data->bitmap_bit_order = MSBFirst;
    x_image_cursor_mask->bitmap_bit_order = MSBFirst;

    pixmap_cursor_data = XCreatePixmap(x_dpy, XRootWindow(x_dpy, x_screen),
                                       16, 16, 1);
    pixmap_cursor_mask = XCreatePixmap(x_dpy, XRootWindow(x_dpy, x_screen),
                                       16, 16, 1);

    gc_values.function = GXcopy;
    gc_values.foreground = ~0;
    gc_values.background = 0;

    gc_values.plane_mask = AllPlanes;

    cursor_data_gc = XCreateGC(x_dpy, pixmap_cursor_data,
                               (GCFunction | GCForeground
                                | GCBackground | GCPlaneMask),
                               &gc_values);
    cursor_mask_gc = XCreateGC(x_dpy, pixmap_cursor_mask,
                               (GCFunction | GCForeground
                                | GCBackground | GCPlaneMask),
                               &gc_values);

    x_hidden_cursor = create_x_cursor(zero, zero, 0, 0);
}


void X11VideoDriver::updateScreenRects(int num_rects, const vdriver_rect_t *r)
{
    updateBuffer((uint32_t*)x_fbuf, width_, height_, num_rects, r);

    for(int i = 0; i < num_rects; i++)
    {
        (*x_put_image)(x_dpy, x_window, copy_gc, x_x_image,
                           r[i].left, r[i].top, r[i].left, r[i].top, r[i].right - r[i].left, r[i].bottom - r[i].top);
    }
}

void X11VideoDriver::shutdown(void)
{
    if(x_dpy == nullptr)
        return;

    /* no more sigio */
    signal(SIGIO, SIG_IGN);

    fcntl(x_fd, F_SETOWN, orig_sigio_owner);
    fcntl(x_fd, F_SETFL, orig_sigio_flag);

    XCloseDisplay(x_dpy);

    x_dpy = nullptr;
}

bool X11VideoDriver::isAcceptableMode(int width, int height, int bpp,
                                      bool grayscale_p,
                                      bool exact_match_p)
{
    if(width == 0)
        width = this->width();
    if(height == 0)
        height = this->height();
    if(bpp == 0)
        bpp = this->bpp();

    if(width > max_width
       || width < 512
       || height > max_height
       || height < 342
       || bpp > max_bpp
       || (bpp != 8
           && bpp != 4
           && bpp != 2
           && bpp != 1))
        return false;
    return true;
}

bool X11VideoDriver::setMode(int width, int height, int bpp, bool grayscale_p)
{
    if(!x_window)
    {
        alloc_x_window(width, height, bpp, grayscale_p);
        rowBytes_ = ((width_ * bpp_ + 31) & ~31) / 8;
        framebuffer_ = new uint8_t[rowBytes_ * height_];
        return true;
    }

    if(width == 0)
        width = width_;
    else if(width > max_width)
        width = max_width;
    else if(width < VDRIVER_MIN_SCREEN_WIDTH)
        width = VDRIVER_MIN_SCREEN_WIDTH;

    if(height == 0)
        height = height_;
    else if(height > max_height)
        height = max_height;
    else if(height < VDRIVER_MIN_SCREEN_HEIGHT)
        height = VDRIVER_MIN_SCREEN_HEIGHT;

    if(bpp == 0)
    {
        bpp = bpp_;
        if(bpp == 0)
            bpp = std::min(8, maxBpp_);
    }

    if(!isAcceptableMode(width, height, bpp, grayscale_p,
                         /* ignored */ false))
        return false;

    if(width != width_
       || height != height_)
    {
        /* resize; the event code will deal with things when the resize
	 event comes through */
        XResizeWindow(x_dpy, x_window, width, height);
    }

    bpp_ = bpp;
    isGrayscale_ = grayscale_p;

    if(framebuffer_)
        delete[] framebuffer_;
    rowBytes_ = ((width_ * bpp_ + 31) & ~31) / 8;
    framebuffer_ = new uint8_t[rowBytes_ * height_];
    
    memset(framebuffer_, 0xFF, rowBytes_ * height);

    return true;
}

void X11VideoDriver::pumpEvents()
{
    if(x_event_pending_p())
        post_pending_x_events(/* dummy */ -1, /* dummy */ nullptr);

    LONGINT x, y;
    Window dummy_window;
    Window child_window;
    int dummy_int;
    unsigned int mods;

    XQueryPointer(x_dpy, x_window, &dummy_window,
                  &child_window, &dummy_int, &dummy_int,
                  &x, &y, &mods);

    vdriver->callbacks_->mouseMoved(x,y);
}

/* stuff from x.c */

void X11VideoDriver::beepAtUser(void)
{
    /* 50 for now */
    XBell(x_dpy, 0);
}

void X11VideoDriver::putScrap(OSType type, LONGINT length, char *p, int scrap_count)
{
    if(type == "TEXT"_4)
    {
        if(selectiontext)
            free(selectiontext);
        selectiontext = (char *)malloc(length);
        if(selectiontext)
        {
            selectionlength = length;
            memcpy(selectiontext, p, length);
            {
                char *ip, *ep;

                for(ip = selectiontext, ep = ip + length; ip < ep; ++ip)
                    if(*ip == '\r')
                        *ip = '\n';
            }
            XSetSelectionOwner(x_dpy, XA_PRIMARY, x_window, CurrentTime);
        }
    }
}

void X11VideoDriver::weOwnScrap(void)
{
    XSetSelectionOwner(x_dpy, XA_PRIMARY, x_window, CurrentTime);
}

int X11VideoDriver::getScrap(OSType type, Handle h)
{
    int retval;

    retval = -1;
    if(type == "TEXT"_4)
    {
        Window selection_owner;

        selection_owner = XGetSelectionOwner(x_dpy, XA_PRIMARY);
        if(selection_owner != None && selection_owner != x_window)
        {
            XEvent xevent;
            Atom rettype;
            LONGINT actfmt;
            unsigned long nitems, ul;
            unsigned long nbytesafter;
            unsigned char *propreturn;
            int i;
            Ptr p;

            XConvertSelection(x_dpy, XA_PRIMARY, XA_STRING, None, x_window,
                              CurrentTime);
            for(i = 10;
                (--i >= 0
                 && !XCheckTypedEvent(x_dpy, SelectionNotify, &xevent));
                Delay(10L, nullptr))
                ;
            if(i >= 0 && xevent.xselection.property != None)
            {
                XGetWindowProperty(x_dpy, xevent.xselection.requestor,
                                   xevent.xselection.property, 0L, 16384L,
                                   (LONGINT)True, XA_STRING, &rettype, &actfmt,
                                   &nitems, &nbytesafter, &propreturn);
                if(rettype == XA_STRING && actfmt == 8)
                {
                    SetHandleSize((Handle)h, nitems);
                    if(LM(MemErr) == noErr)
                    {
                        memcpy(*h, propreturn, nitems);
                        for(ul = nitems, p = *h; ul > 0; ul--)
                            if(*p++ == '\n')
                                p[-1] = '\r';
                        retval = nitems;
                    }
                }
                XFree((char *)propreturn);
            }
        }
    }
    return retval;
}

void X11VideoDriver::setTitle(const std::string& newtitle)
{
    XSizeHints xsh;

    memset(&xsh, 0, sizeof xsh);
    char *newtitle_c = const_cast<char*>(newtitle.c_str());
    XSetStandardProperties(x_dpy, x_window, newtitle_c, newtitle_c, None,
                           nullptr, 0, &xsh);
}
