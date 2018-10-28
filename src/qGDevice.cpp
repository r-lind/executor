/* Copyright 1994, 1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#include "rsys/common.h"
#include "QuickDraw.h"
#include "CQuickDraw.h"
#include "MemoryMgr.h"
#include "WindowMgr.h"
#include "MenuMgr.h"
#include "SysErr.h"

#include "rsys/cquick.h"
#include "rsys/wind.h"
#include "rsys/vdriver.h"
#include "rsys/flags.h"
#include "rsys/autorefresh.h"
#include "rsys/redrawscreen.h"
#include "rsys/executor.h"
#include "rsys/functions.impl.h"

using namespace Executor;

/*
 * determined experimentally -- this fix is needed for Energy Scheming because
 * they put a bogus test for mode >= 8 in their code to detect color.  NOTE:
 * the value that will get stored is most definitely not 8 or anything near
 * 8, since 8bpp will be translated to 0x83.
 */

static LONGINT
mode_from_bpp(int bpp)
{
    LONGINT retval;
    int log2;

    if(bpp >= 1 && bpp <= 32)
        log2 = ROMlib_log2[bpp];
    else
        log2 = -1;

    if(log2 != -1)
        retval = 0x80 | ROMlib_log2[bpp];
    else
    {
        warning_unexpected("bpp = %d", bpp);
        retval = 0x86; /* ??? */
    }
    return retval;
}

void Executor::gd_allocate_main_device(void)
{
    GDHandle graphics_device;

    if(vdriver->framebuffer() == nullptr)
        gui_fatal("vdriver not initialized, unable to allocate `LM(MainDevice)'");

    TheZoneGuard guard(LM(SysZone));

    PixMapHandle gd_pixmap;
    Rect *gd_rect;

    SET_HILITE_BIT();
    LM(TheGDevice) = LM(MainDevice) = LM(DeviceList) = CLC_NULL;

    graphics_device = NewGDevice(/* no driver */ 0,
                                 mode_from_bpp(vdriver->bpp()));

    /* we are the main device, since there are currently no others */
    LM(TheGDevice) = LM(MainDevice) = graphics_device;

    /* set gd flags reflective of the main device */
    GD_FLAGS_X(graphics_device) |= (1 << mainScreen) | (1 << screenDevice) | (1 << screenActive)
                                           /* PacMan Deluxe avoids
						 GDevices with noDriver
						 set.  Looking around
						 Apple's site shows that
						 people set this bit when
						 they're creating offscreen
						 gDevices.  It's not clear
						 whether or not we should
						 be setting this bit.
					    | (1 << noDriver) */;

    gd_set_bpp(graphics_device, !vdriver->isGrayscale(), vdriver->isFixedCLUT(),
               vdriver->bpp());

    gd_pixmap = GD_PMAP(graphics_device);
    PIXMAP_SET_ROWBYTES_X(gd_pixmap, vdriver->rowBytes());
    PIXMAP_BASEADDR_X(gd_pixmap) = (Ptr)vdriver->framebuffer();

    gd_rect = &GD_RECT(graphics_device);
    gd_rect->top = gd_rect->left = 0;
    gd_rect->bottom = vdriver->height();
    gd_rect->right = vdriver->width();
    PIXMAP_BOUNDS(gd_pixmap) = *gd_rect;

    /* add ourselves to the device list */
    GD_NEXT_GD_X(graphics_device) = LM(DeviceList);
    LM(DeviceList) = graphics_device;

    /* Assure that we're using the correct colors. */
    if(vdriver->bpp() <= 8)
        vdriver->setColors(0, 1 << vdriver->bpp(),
                           CTAB_TABLE(PIXMAP_TABLE(GD_PMAP(graphics_device))));
}

GDHandle Executor::C_NewGDevice(INTEGER ref_num, LONGINT mode)
{
    GDHandle this2;
    GUEST<Handle> h;
    GUEST<PixMapHandle> pmh;

    TheZoneGuard guard(LM(SysZone));

    this2 = (GDHandle)NewHandle((Size)sizeof(GDevice));

    if(this2 == nullptr)
        return this2;

    /* initialize fields; for some of these, i'm not sure what
	  the value should be */
    GD_ID_X(this2) = 0; /* ??? */

    /* CLUT graphics device by default */
    GD_TYPE_X(this2) = clutType;

    /* how do i allocate a new inverse color table? */
    h = NewHandle(0);
    GD_ITABLE_X(this2) = guest_cast<ITabHandle>(h);
    GD_RES_PREF_X(this2) = DEFAULT_ITABLE_RESOLUTION;

    GD_SEARCH_PROC_X(this2) = nullptr;
    GD_COMP_PROC_X(this2) = nullptr;

    GD_FLAGS_X(this2) = 0;
    /* mode_from_bpp (1)  indicates b/w hardware */
    if(mode != mode_from_bpp(1))
        GD_FLAGS_X(this2) |= 1 << gdDevType;

    pmh = NewPixMap();
    GD_PMAP_X(this2) = pmh;
    CTAB_FLAGS_X(PIXMAP_TABLE(GD_PMAP(this2))) |= CTAB_GDEVICE_BIT_X;

    GD_REF_CON_X(this2) = 0; /* ??? */
    GD_REF_NUM_X(this2) = ref_num; /* ??? */
    GD_MODE_X(this2) = mode; /* ??? */

    GD_NEXT_GD_X(this2) = nullptr;

    GD_RECT(this2).top = 0;
    GD_RECT(this2).left = 0;
    GD_RECT(this2).bottom = 0;
    GD_RECT(this2).right = 0;

    /* handle to cursor's expanded data/mask? */
    GD_CCBYTES_X(this2) = 0;
    GD_CCDEPTH_X(this2) = 0;
    GD_CCXDATA_X(this2) = nullptr;
    GD_CCXMASK_X(this2) = nullptr;

    GD_RESERVED_X(this2) = 0;

    /* if mode is -1, this2 is a user created gdevice,
	  and InitGDevice () should not be called (IMV-122) */
    if(mode != -1)
        InitGDevice(ref_num, mode, this2);

    return this2;
}

void Executor::gd_set_bpp(GDHandle gd, bool color_p, bool fixed_p, int bpp)
{
    PixMapHandle gd_pixmap;
    bool main_device_p = (gd == LM(MainDevice));

    /* set the color bit, all other flag bits should be the same */
    if(color_p)
        GD_FLAGS_X(gd) |= 1 << gdDevType;
    else
        GD_FLAGS_X(gd) &= ~(1 << gdDevType);

    GD_TYPE_X(gd) = (bpp > 8
                         ? directType
                         : (fixed_p ? fixedType : clutType));

    gd_pixmap = GD_PMAP(gd);
    pixmap_set_pixel_fields(STARH(gd_pixmap), bpp);

    if(bpp <= 8)
    {
        CTabHandle gd_color_table;

        gd_color_table = PIXMAP_TABLE(gd_pixmap);

        if(fixed_p)
        {
            if(!main_device_p)
                gui_fatal("unable to set bpp of gd not the screen");
            else
            {
                CTabHandle gd_color_table;

                gd_color_table = PIXMAP_TABLE(gd_pixmap);
                SetHandleSize((Handle)gd_color_table,
                              CTAB_STORAGE_FOR_SIZE((1 << bpp) - 1));
                CTAB_SIZE_X(gd_color_table) = (1 << bpp) - 1;
                vdriver->getColors(0, 1 << bpp,
                                   CTAB_TABLE(gd_color_table));

                CTAB_SEED_X(gd_color_table) = GetCTSeed();
            }
        }
        else
        {
            CTabHandle temp_color_table;

            temp_color_table = GetCTable(color_p
                                             ? bpp
                                             : (bpp + 32));
            if(temp_color_table == nullptr)
                gui_fatal("unable to get color table `%d'",
                          color_p ? bpp : (bpp + 32));
            ROMlib_copy_ctab(temp_color_table, gd_color_table);
            DisposeCTable(temp_color_table);
        }

        CTAB_FLAGS_X(gd_color_table) = CTAB_GDEVICE_BIT_X;
        MakeITable(gd_color_table, GD_ITABLE(gd), GD_RES_PREF(gd));

        if(main_device_p && !fixed_p && bpp <= 8)
            vdriver->setColors(0, 1 << bpp, CTAB_TABLE(gd_color_table));
    }
}

/* it seems that `gd_ref_num' describes which device to initialize,
   and `mode' tells it what mode to start it in

   i'm not sure how this2 relates to NeXT/vga video hardware, for now,
   we do nothing */
void Executor::C_InitGDevice(INTEGER gd_ref_num, LONGINT mode, GDHandle gdh)
{
}

void Executor::C_SetDeviceAttribute(GDHandle gdh, INTEGER attribute,
                                    BOOLEAN value)
{
    if(value)
        GD_FLAGS_X(gdh) |= 1 << attribute;
    else
        GD_FLAGS_X(gdh) &= ~(1 << attribute);
}

void Executor::C_SetGDevice(GDHandle gdh)
{
    if(LM(TheGDevice) != gdh)
    {
        LM(TheGDevice) = gdh;
        ROMlib_invalidate_conversion_tables();
    }
}

void Executor::C_DisposeGDevice(GDHandle gdh)
{
    DisposeHandle((Handle)GD_ITABLE(gdh));
    DisposePixMap(GD_PMAP(gdh));

    /* FIXME: do other cleanup */
    DisposeHandle((Handle)gdh);
}

GDHandle Executor::C_GetGDevice()
{
    return LM(TheGDevice);
}

GDHandle Executor::C_GetDeviceList()
{
    return LM(DeviceList);
}

GDHandle Executor::C_GetMainDevice()
{
    GDHandle retval;

    retval = LM(MainDevice);

    /* Unfortunately, Realmz winds up dereferencing non-existent
     memory unless noDriver is set, but PacMan Deluxe will have
     trouble if that bit is set. */

    if(ROMlib_creator == TICK("RLMZ"))
        GD_FLAGS_X(retval) |= 1 << noDriver;
    else
        GD_FLAGS_X(retval) &= ~(1 << noDriver);

    return retval;
}

GDHandle Executor::C_GetMaxDevice(Rect *globalRect)
{
    /* FIXME:
     currently we have only a single device, so that has
     the max pixel depth for any given region (tho we would
     probably see if it intersects the main screen and return
     nullptr otherwise */
    return LM(MainDevice);
}

GDHandle Executor::C_GetNextDevice(GDHandle cur_device)
{
    return GD_NEXT_GD(cur_device);
}

void Executor::C_DeviceLoop(RgnHandle rgn,
                            DeviceLoopDrawingProcPtr drawing_proc,
                            LONGINT user_data, DeviceLoopFlags flags)
{
    GDHandle gd;
    GUEST<RgnHandle> save_vis_rgn_x;
    RgnHandle sect_rgn, gd_rect_rgn;

    save_vis_rgn_x = PORT_VIS_REGION_X(qdGlobals().thePort);

    sect_rgn = NewRgn();
    gd_rect_rgn = NewRgn();

    /* Loop over all GDevices, looking for active screens. */
    for(gd = LM(DeviceList); gd; gd = GD_NEXT_GD(gd))
        if((GD_FLAGS_X(gd) & ((1 << screenDevice)
                                 | (1 << screenActive)))
           == ((1 << screenDevice) | (1 << screenActive)))
        {
            /* NOTE: I'm blowing off some flags that come into play when
	 * you have multiple screens.  I don't think anything terribly
	 * bad would happen even if we had multiple screens, but we
	 * don't.  We can worry about it later.
	 */

            if(!(flags & allDevices))
            {
                Rect gd_rect, *port_bounds;

                /* Map the GDevice rect into qdGlobals().thePort local coordinates. */
                gd_rect = GD_RECT(gd);
                port_bounds = &(PORT_BOUNDS(qdGlobals().thePort));
                OffsetRect(&gd_rect,
                           port_bounds->left,
                           port_bounds->top);

                /* Intersect GDevice rect with the specified region. */
                RectRgn(gd_rect_rgn, &gd_rect);
                SectRgn(gd_rect_rgn, rgn, sect_rgn);

                SectRgn(sect_rgn, PORT_VIS_REGION(qdGlobals().thePort), sect_rgn);

                /* Save it away in qdGlobals().thePort. */
                PORT_VIS_REGION_X(qdGlobals().thePort) = sect_rgn;
            }

            if((flags & allDevices) || !EmptyRgn(sect_rgn))
            {
                drawing_proc(PIXMAP_PIXEL_SIZE(GD_PMAP(gd)),
                             GD_FLAGS(gd), gd, user_data);
            }
        }

    PORT_VIS_REGION_X(qdGlobals().thePort) = save_vis_rgn_x;

    DisposeRgn(gd_rect_rgn);
    DisposeRgn(sect_rgn);
}

BOOLEAN Executor::C_TestDeviceAttribute(GDHandle gdh, INTEGER attribute)
{
    BOOLEAN retval;

    retval = (GD_FLAGS(gdh) & (1 << attribute)) != 0;
    return retval;
}

// FIXME: #warning ScreenRes is duplicate with toolutil.cpp
void Executor::C_ScreenRes(GUEST<INTEGER> *h_res, GUEST<INTEGER> *v_res)
{
    *h_res = PIXMAP_HRES(GD_PMAP(LM(MainDevice))) >> 16;
    *v_res = PIXMAP_VRES(GD_PMAP(LM(MainDevice))) >> 16;
}

INTEGER Executor::C_HasDepth(GDHandle gdh, INTEGER bpp, INTEGER which_flags,
                             INTEGER flags)
{
    flags &= ~1;
    which_flags &= ~1;

    if(gdh != LM(MainDevice)
       || bpp == 0)
        return false;

    return (vdriver->isAcceptableMode(0, 0, bpp, ((which_flags & (1 << gdDevType))
                                                      ? (flags & (1 << gdDevType)) == (1 << gdDevType)
                                                      : vdriver->isGrayscale()),
                                      false));
}

OSErr Executor::C_SetDepth(GDHandle gdh, INTEGER bpp, INTEGER which_flags,
                           INTEGER flags)
{
    PixMapHandle gd_pixmap;
    WindowPeek tw;

    if(gdh != LM(MainDevice))
    {
        warning_unexpected("Setting the depth of a device not the screen; "
                           "this violates bogus assumptions in SetDepth.");
    }

    gd_pixmap = GD_PMAP(gdh);

    if(bpp == PIXMAP_PIXEL_SIZE(gd_pixmap))
        return noErr;

    HideCursor();

    note_executor_changed_screen(0, vdriver->height());

    if(bpp == 0 || !vdriver->setMode(0, 0, bpp, ((which_flags & (1 << gdDevType)) ? !(flags & (1 << gdDevType)) : vdriver->isGrayscale())))
    {
        /* IMV says this2 returns non-zero in error case; not positive
	 cDepthErr is correct; need to verify */
        ShowCursor();
        return cDepthErr;
    }

    if(vdriver->framebuffer() == nullptr)
        gui_fatal("vdriver not initialized, unable to change bpp");

#if SIZEOF_CHAR_P > 4
    // code duplication with ROMlib_InitGDevices() below
    ROMlib_offsets[1] = (uintptr_t)vdriver->framebuffer();
    ROMlib_offsets[1] -= (1UL << 30);
    ROMlib_sizes[1] = vdriver->width() * vdriver->height() * 5;
#endif

    gd_set_bpp(gdh, !vdriver->isGrayscale(), vdriver->isFixedCLUT(), bpp);

    PIXMAP_SET_ROWBYTES_X(gd_pixmap, vdriver->rowBytes());
    qdGlobals().screenBits.rowBytes = vdriver->rowBytes();

    cursor_reset_current_cursor();

    ShowCursor();

    /* FIXME: assuming (1) all windows are on the current
     graphics device, and (2) the rowbytes and baseaddr
     of the gdevice cannot change */
    /* set the pixel size, rowbytes, etc
     of windows and the window manager color graphics port */

    if(LM(WWExist) == EXIST_YES)
    {
        for(tw = LM(WindowList); tw; tw = WINDOW_NEXT_WINDOW(tw))
        {
            GrafPtr gp;

            gp = WINDOW_PORT(tw);

            if(CGrafPort_p(gp))
            {
                PixMapHandle window_pixmap = CPORT_PIXMAP(gp);
                pixmap_set_pixel_fields(*window_pixmap, bpp);
                PIXMAP_SET_ROWBYTES_X(window_pixmap,
                                      PIXMAP_ROWBYTES_X(gd_pixmap));

                ROMlib_copy_ctab(PIXMAP_TABLE(gd_pixmap),
                                 PIXMAP_TABLE(window_pixmap));

                ThePortGuard guard(gp);
                RGBForeColor(&CPORT_RGB_FG_COLOR(gp));
                RGBBackColor(&CPORT_RGB_BK_COLOR(gp));
            }
            else
            {
                BITMAP_SET_ROWBYTES_X(&PORT_BITS(gp),
                                      PIXMAP_ROWBYTES_X(gd_pixmap));
            }
        }

        /* do the same for the LM(WMgrCPort) */
        {
            PixMapHandle wmgr_cport_pixmap = CPORT_PIXMAP(LM(WMgrCPort));

            pixmap_set_pixel_fields(*wmgr_cport_pixmap, bpp);
                
            PIXMAP_SET_ROWBYTES_X(wmgr_cport_pixmap,
                                  PIXMAP_ROWBYTES_X(gd_pixmap));

            ROMlib_copy_ctab(PIXMAP_TABLE(gd_pixmap),
                             PIXMAP_TABLE(wmgr_cport_pixmap));
        }

        ThePortGuard guard(wmgr_port);
        RGBForeColor(&ROMlib_black_rgb_color);
        RGBBackColor(&ROMlib_white_rgb_color);
    }

    /* Redraw the screen if that's what changed. */
    if(gdh == LM(MainDevice))
        redraw_screen();

    return noErr;
}

void Executor::ROMlib_InitGDevices()
{
    if(!vdriver->init())
    {
        fprintf(stderr, "Unable to initialize video driver.\n");
        exit(-12);
    }

    /* Set up the current graphics mode appropriately. */
    if(!vdriver->setMode(flag_width, flag_height, flag_bpp, flag_grayscale))
    {
        fprintf(stderr, "Could not set graphics mode.\n");
        exit(-12);
    }

#if SIZEOF_CHAR_P > 4
    if(vdriver->framebuffer() == 0)
        abort();
    ROMlib_offsets[1] = (uintptr_t)vdriver->framebuffer();
    ROMlib_offsets[1] -= (1UL << 30);
    ROMlib_sizes[1] = vdriver->width() * vdriver->height() * 5; // ### //vdriver->rowBytes() * vdriver->height();
#endif

    if(vdriver->isGrayscale())
    {
        /* Choose a nice light gray hilite color. */
        LM(HiliteRGB).red = (unsigned short)0xAAAA;
        LM(HiliteRGB).green = (unsigned short)0xAAAA;
        LM(HiliteRGB).blue = (unsigned short)0xAAAA;
    }
    else
    {
        /* how about a nice yellow hilite color? no, it's ugly. */
        LM(HiliteRGB).red = (unsigned short)0xAAAA;
        LM(HiliteRGB).green = (unsigned short)0xAAAA;
        LM(HiliteRGB).blue = (unsigned short)0xFFFF;
    }



    /* initialize the mac rgb_spec's */
    make_rgb_spec(&mac_16bpp_rgb_spec,
                    16, true, 0,
                    5, 10, 5, 5, 5, 0,
                    CL_RAW(GetCTSeed()));

    make_rgb_spec(&mac_32bpp_rgb_spec,
                    32, true, 0,
                    8, 16, 8, 8, 8, 0,
                    CL_RAW(GetCTSeed()));

    gd_allocate_main_device();
}