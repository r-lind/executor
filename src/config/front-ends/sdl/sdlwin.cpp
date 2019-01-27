/* Copyright 1994 - 1999 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */
#include "sdl.h"
#include <base/common.h>
#include <quickdraw/srcblt.h>
#include <vdriver/refresh.h>

#if defined(CYGWIN32)
#include "win_screen.h"
#endif

#include <SDL/SDL.h>

#include "sdlevents.h"
#include "syswm_map.h"

#if defined(linux) && !defined(powerpc) && !defined(__ppc__)
#define USE_SDL_EVENT_THREAD
#include "sdlX.h"
#endif

using namespace Executor;

/* This is our video display */
static SDL_Surface *screen;

/* Currently a private colormap is the default */
static int video_flags = (SDL_SWSURFACE | SDL_HWPALETTE);

/* SDL vdriver implementation */

bool ROMlib_fullscreen_p = false;
bool ROMlib_hwsurface_p = false;

bool SDLVideoDriver::init()
{
    int flags;

    flags = SDL_INIT_VIDEO;
#if defined(USE_SDL_EVENT_THREAD)
    ROMlib_XInitThreads();
    flags |= SDL_INIT_EVENTTHREAD;
#endif

    if(SDL_Init(flags) < 0)
        return (false);
    sdl_events_init();

#if 0
  {
    SDL_PixelFormat format;

    /* Find out our "best" pixel depth */
    SDL_GetDisplayFormat(&format);
    maxBpp_ = format.BitsPerPixel;
  }
#else
    maxBpp_ = 8;
#endif

    /* Try for fullscreen on platforms that support it */
    if(getenv("SDL_FULLSCREEN") != nullptr || ROMlib_fullscreen_p)
        video_flags |= SDL_FULLSCREEN;

    /* Allow unsafe fullscreen video memory access */
    if(getenv("SDL_HWSURFACE") != nullptr || ROMlib_hwsurface_p)
        video_flags |= SDL_HWSURFACE;

    return (true);
}

bool SDLVideoDriver::isAcceptableMode(int width, int height, int bpp,
                                      bool grayscale_p, bool exact_match_p)
{
    bool retval;

    if(!width)
        width = this->width();

    if(!height)
        height = this->height();

    if(!bpp)
        bpp = this->bpp();

    if(!SDL_VideoModeOK(width, height, bpp, video_flags))
        retval = false;
    else if(grayscale_p != isGrayscale())
        retval = false;
    else
        retval = bpp <= 8;

    return retval;
}

bool SDLVideoDriver::setMode(int width, int height, int bpp, bool grayscale_p)
{
    /* Massage the width and height parameters */
    if(width == 0)
    {
        width = width_;
        if(width == 0)
        {
            width = VDRIVER_DEFAULT_SCREEN_WIDTH;
#if 0
            if(ROMlib_fullscreen_p)
                width = std::max(width, os_current_screen_width());
#endif
        }
    }

    if(height == 0)
    {
        height = height_;
        if(height == 0)
        {
            height = VDRIVER_DEFAULT_SCREEN_HEIGHT;
#if 0
            if(ROMlib_fullscreen_p)
                height = std::max(height, os_current_screen_height());
#endif
        }
    }

    if(bpp == 0)
        bpp = bpp_;

    if(!isAcceptableMode(width, height, bpp, grayscale_p, false))
        return (false);

    /* Set the video mode */
    screen = SDL_SetVideoMode(width, height, bpp, video_flags);
    if(screen == nullptr)
        return (false);

    /* Fill the vdriver globals */
    width_ = screen->w;
    height_ = screen->h;
    bpp_ = screen->format->BitsPerPixel;
    rowBytes_ = screen->pitch;
    if(SDL_MUSTLOCK(screen))
    {
        /* WARNING!  This results in surface memory that is unsafe to access! */
        if(SDL_LockSurface(screen) < 0)
            return (false);
        framebuffer_ = (uint8_t *)screen->pixels;
        SDL_UnlockSurface(screen);
        fprintf(stderr, "Warning: Executor performing unsafe video access\n");
    }
    else
        framebuffer_ = (uint8_t *)screen->pixels;

    sdl_syswm_init();

#if defined(CYGWIN32)
    ROMlib_recenter_window();
#endif

    return (true);
}

void SDLVideoDriver::setColors(int first_color, int num_colors, const ColorSpec *colors)
{
    int i;
    SDL_Color *sdl_cmap;

    sdl_cmap = (SDL_Color *)alloca(num_colors * sizeof(SDL_Color));
    for(i = 0; i < num_colors; ++i)
    {
        sdl_cmap[i].r = (colors[i].rgb.red >> 8);
        sdl_cmap[i].g = (colors[i].rgb.green >> 8);
        sdl_cmap[i].b = (colors[i].rgb.blue >> 8);
    }
    SDL_SetColors(screen, sdl_cmap, first_color, num_colors);
}

void SDLVideoDriver::updateScreenRects(int num_rects, const vdriver_rect_t *r,
                                       bool cursor_p)
{
    SDL_Rect *rects;
    int i;

    rects = (SDL_Rect *)alloca(num_rects * sizeof(SDL_Rect));
    for(i = 0; i < num_rects; ++i)
    {
        rects[i].x = r[i].left;
        rects[i].w = r[i].right - r[i].left;
        rects[i].y = r[i].top;
        rects[i].h = r[i].bottom - r[i].top;
    }
    SDL_UpdateRects(screen, num_rects, rects);
}

void SDLVideoDriver::shutdown(void)
{
    SDL_Quit();
}
