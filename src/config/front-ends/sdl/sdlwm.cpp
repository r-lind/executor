/* Copyright 1994, 1995, 1996 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */
#include "sdl.h"
#include <stdio.h>
#include "SDL/SDL.h"

#include "base/common.h"

/* Window manager interface functions */

void SDLVideoDriver::setTitle(const std::string& title)
{
    SDL_WM_SetCaption(title.c_str(), "executor");
}

std::string
SDLVideoDriver::getTitle(void)
{
    char *retval;

    SDL_WM_GetCaption(&retval, (char **)0);
    return retval;
}

/* This is really inefficient.  We should hash the cursors */
void SDLVideoDriver::setCursor(char *cursor_data,
                               unsigned short cursor_mask[16],
                               int hotspot_x, int hotspot_y)
{
    SDL_Cursor *old_cursor, *new_cursor;

    old_cursor = SDL_GetCursor();
    new_cursor = SDL_CreateCursor((unsigned char *)cursor_data,
                                  (unsigned char *)cursor_mask,
                                  16, 16, hotspot_x, hotspot_y);
    if(new_cursor != nullptr)
    {
        SDL_SetCursor(new_cursor);
        SDL_FreeCursor(old_cursor);
    }
}

bool SDLVideoDriver::setCursorVisible(bool show_p)
{
    return (SDL_ShowCursor(show_p));
}
