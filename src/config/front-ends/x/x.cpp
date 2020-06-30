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
#include "x_keysym.h"

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
                    }
                }
            }
        }
    }

    if(fbuf == nullptr)
    {
        have_shm = false;

        warning_unexpected("not using shared memory");
        row_bytes = ((width * 32 + 31) / 32) * 4;
        fbuf = (unsigned char *)calloc(row_bytes * height, 1);
        x_image = XCreateImage(x_dpy, visual->visual, bpp, ZPixmap, 0,
                               (char *)fbuf, width, height, 32, row_bytes);
        resultant_bpp = x_image->bits_per_pixel;
    }

        // FIXME: this should be host byte order
    x_image->byte_order = LSBFirst;
    x_image->bitmap_bit_order = LSBFirst;

    *fbuf_return = fbuf;
    *row_bytes_return = row_bytes;
    *x_image_return = x_image;
    *bpp_return = resultant_bpp;
}


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
        int x = r[i].left;
        int y = r[i].top;
        int w = r[i].right - r[i].left;
        int h = r[i].bottom - r[i].top;
        if(have_shm)
            XShmPutImage(x_dpy, x_window, copy_gc, x_x_image,
                    x, y, x, y, w, h, False);
        else
            XPutImage(x_dpy, x_window, copy_gc, x_x_image,
                    x, y, x, y, w, h);
    }
    XFlush(x_dpy);
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
