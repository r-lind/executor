#pragma once

#include <sound/sounddriver.h>
#include <mutex>
#include <condition_variable>

#define LOGBUFSIZE 11 /* Must be between 7 and 17 decimal */

/*
   * There's what appears to be a bug in some of the SDLs out there that
   * results in SDL choosing to use one half the number of samples that we ask
   * for.  As such, we're going to make room for twice the amount we want and
   * then ask for twice the amount.  If we get it, oh well, it just means
   * more latency.
   */

#define BUFSIZE (1 << (LOGBUFSIZE + 1)) /* +1 as bug workaround */

class SDLSound : public Executor::SoundDriver
{
public:
    virtual bool sound_init();
    virtual void sound_shutdown();
    virtual bool sound_works();
    virtual bool sound_silent();
    virtual void sound_go();
    virtual void sound_stop();
    virtual Executor::HungerInfo HungerStart();
    virtual void HungerFinish();

private:
    std::mutex mutex_;
    std::condition_variable cond_;

    enum class BufferState
    {
        none,
        empty,
        full,
    };
    BufferState state_ = BufferState::none;

    int sound_on; /* 1 if we are generating interrupts */
    bool have_sound_p; /* true if sound is supported */

    Executor::snd_time t1;

    uint8_t* buffer_;
    size_t buffersize_;
    int rate_;

    void audioCallback(uint8_t *buffer, int len);
};

//extern bool sound_sdl_init (sound_driver_t *s);
extern void ROMlib_set_sdl_audio_driver_name(const char *str);
