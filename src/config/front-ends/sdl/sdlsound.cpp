/* Copyright 1995-2005 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/*
 * This is a hacked up copy of linux-sound.c that is being used to test
 * a different way to drive SDL's sound.
 */

#include <base/common.h>
#include <vdriver/vdriver.h>
#include <sound/sounddriver.h>
#include <base/m68kint.h>
#include "sdlsound.h"

#include <SDL/SDL_audio.h>

using namespace Executor;

enum
{
    NON_RUNNING_SOUND_RATE = 22255
};

static char *sdl_audio_driver_name = nullptr;

void
ROMlib_set_sdl_audio_driver_name(const char *str)
{
    if(sdl_audio_driver_name)
        free(sdl_audio_driver_name);
    if(str)
        sdl_audio_driver_name = strdup(str);
    else
        sdl_audio_driver_name = nullptr;
}

bool SDLSound::sound_works()
{
    return have_sound_p;
}

void SDLSound::sound_go()
{
    sound_on = 1;

    SDL_PauseAudio(0);
}

void SDLSound::sound_stop()
{
    sound_on = 0;
}

/* Do any bookkeeping needed to start feeding a hungry device */

void SDLSound::HungerStart()
{
    std::unique_lock<std::mutex> lock(mutex_);

    while(state_ != BufferState::empty)
        cond_.wait(lock);
}

/* Figure out how to feed the hungry output device. */

HungerInfo
SDLSound::GetHungerInfo()
{
    HungerInfo info;

    info.buf = buffer_;
    info.bufsize = buffersize_;

    info.t2 = t1;
    info.t3 = info.t2 + buffersize_;
    info.t4 = info.t3;

    return info;
}

/* Assuming that the information returned by snd_get_hunger_info was
   honored, send the samples off to the device. */

void SDLSound::HungerFinish()
{
    t1 += buffersize_;

    std::unique_lock<std::mutex> lock(mutex_);

    state_ = BufferState::full;
    cond_.notify_one();
}

void SDLSound::sound_shutdown()
{
    if(have_sound_p)
    {
        /* possibly kill the thread here */
        have_sound_p = false;
    }
}

bool SDLSound::sound_silent()
{
    return false;
}


void SDLSound::audioCallback(uint8_t *buffer, int len)
{
    std::unique_lock<std::mutex> lock(mutex_);

    assert(state_ == BufferState::none);
    
    state_ = BufferState::empty;
    buffer_ = buffer;
    buffersize_ = len;

    interrupt_generate(M68K_SOUND_PRIORITY);
    cond_.notify_one();

    while(state_ != BufferState::full)
        cond_.wait(lock);
    state_ = BufferState::none;
}

bool SDLSound::sound_init()
{
    SDL_AudioSpec spec;
    syn68k_addr_t my_callback;

    sound_on = 0; /* 1 if we are generating interrupts */
    t1 = 0;
    ROMlib_SND_RATE = 22255; 

    have_sound_p = false;

    if(sound_disabled_p)
        return false;

    if(SDL_AudioInit(sdl_audio_driver_name) != 0)
    {
        const char *err;

        err = SDL_GetError();
        fprintf(stderr, "SDL_Init(SDL_INIT_AUDIO) failed: '%s'\n",
                err ? err : "(nullptr)");
        return false;
    }

    memset(&spec, 0, sizeof spec);

    spec.freq = ROMlib_SND_RATE;
    spec.format = AUDIO_U8;
    spec.channels = 1;
    spec.samples = BUFSIZE;
    spec.callback = [](void* userData, Uint8* buffer, int len) {
        reinterpret_cast<SDLSound*>(userData)->audioCallback(buffer, len);
    };
    spec.userdata = this;

    if(SDL_OpenAudio(&spec, nullptr) < 0)
    {
        fprintf(stderr, "SDL_OpenAudio failed '%s'\n", SDL_GetError());
        return false;
    }

    my_callback = callback_install(sound_callback, nullptr);
    *(GUEST<syn68k_addr_t> *)SYN68K_TO_US(M68K_SOUND_VECTOR * 4) = my_callback;

    have_sound_p = true;

    /* Success! */
    return true;
}
