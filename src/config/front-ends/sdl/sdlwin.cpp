/* Copyright 1994 - 1999 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */
#include "sdl.h"
#include <base/common.h>
#include <quickdraw/srcblt.h>
#include <vdriver/refresh.h>

#include <SDL/SDL.h>

#include "sdlevents.h"
#include "syswm_map.h"
#include "sdlsound.h"

#if defined(linux) && !defined(powerpc) && !defined(__ppc__)
#define USE_SDL_EVENT_THREAD
#include "sdlX.h"
#endif

using namespace Executor;

/* Currently a private colormap is the default */
static int video_flags = (SDL_SWSURFACE | SDL_HWPALETTE);

/* SDL vdriver implementation */

SDLVideoDriver::SDLVideoDriver(Executor::IEventListener *listener, int& argc, char* argv[])
    : VideoDriver(listener)
{
    int flags;

    flags = SDL_INIT_VIDEO;
#if defined(USE_SDL_EVENT_THREAD)
    ROMlib_XInitThreads();
    flags |= SDL_INIT_EVENTTHREAD;
#endif

    if(SDL_Init(flags) < 0)
        throw std::runtime_error("Failed to initialize SDL.");
    sdl_events_init();

    sound_driver = new SDLSound();
}

SDLVideoDriver::~SDLVideoDriver()
{
    SDL_Quit();
}

bool SDLVideoDriver::isAcceptableMode(int width, int height, int bpp,
                                      bool grayscale_p)
{
    bool retval;

    onMainThread([&] {
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
    });

    return retval;
}

bool SDLVideoDriver::setMode(int width, int height, int bpp, bool grayscale_p)
{
    bool success = false;
    onMainThread([&] {
        /* Massage the width and height parameters */
        if(width == 0)
        {
            width = framebuffer_.width;
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
            height = framebuffer_.height;
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
            bpp = 8;

        screen = nullptr;
        if(bpp > 8)
            return;

        /* Set the video mode */
        screen = SDL_SetVideoMode(width, height, bpp, video_flags);
        if(!screen)
            return;

        uint8_t *data;
        if(SDL_MUSTLOCK(screen))
        {
            /* WARNING!  This results in surface memory that is unsafe to access! */
            if(SDL_LockSurface(screen) < 0)
                return;
            data = (uint8_t *)screen->pixels;
            SDL_UnlockSurface(screen);
            fprintf(stderr, "Warning: Executor performing unsafe video access\n");
        }
        else
            data = (uint8_t *)screen->pixels;

        framebuffer_ = {};
        framebuffer_.data = std::shared_ptr<uint8_t>(data, [](uint8_t* p){ /* FIXME: delete */ });
        framebuffer_.width = screen->w;
        framebuffer_.height = screen->h;
        framebuffer_.bpp = screen->format->BitsPerPixel;
        framebuffer_.rowBytes = screen->pitch;

        sdl_syswm_init();
        success = true;
    });

    return success;
}

void SDLVideoDriver::setColors(int num_colors, const vdriver_color_t *colors)
{
    onMainThread([&] {
        int i;
        SDL_Color *sdl_cmap;

        sdl_cmap = (SDL_Color *)alloca(num_colors * sizeof(SDL_Color));
        for(i = 0; i < num_colors; ++i)
        {
            sdl_cmap[i].r = (colors[i].red >> 8);
            sdl_cmap[i].g = (colors[i].green >> 8);
            sdl_cmap[i].b = (colors[i].blue >> 8);
        }
        SDL_SetColors(screen, sdl_cmap, 0, num_colors);
        dirtyRects_.add(0, 0, height(), width());
    });
}
