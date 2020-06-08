#include "vdriver.h"

#include <rsys/adb.h>
#include <rsys/scrap.h>
#include <osevent/osevent.h>
#include <rsys/keyboard.h>
#include <ToolboxEvent.h>

using namespace Executor;

void VideoDriverCallbacks::mouseButtonEvent(bool down, int h, int v)
{
    mouseMoved(h,v);
    mouseButtonEvent(down);
}

void VideoDriverCallbacks::mouseButtonEvent(bool down)
{
    if(down)
        LM(MBState) = 0;
    else
        LM(MBState) = 0xFF;

    PostEvent(down ? mouseDown : mouseUp, 0);
}


void VideoDriverCallbacks::mouseMoved(int h, int v)
{
    LM(MouseLocation2) = LM(MTemp) = LM(MouseLocation) = Point{(int16_t)v, (int16_t)h};

    adb_apeiron_hack();
}

void VideoDriverCallbacks::keyboardEvent(bool down, unsigned char mkvkey)
{
    mkvkey = ROMlib_right_to_left_key_map(mkvkey);

    ROMlib_SetKey(mkvkey, down);

    auto kchr = ROMlib_kchr_ptr();

    uint32_t translated = KeyTranslate(
        kchr, 
        (ROMlib_GetModifiers() & 0xFF00) | mkvkey | (down ? 0 : 0x80),
        &keytransState);

    auto evcode = down ? keyDown : keyUp;

    if(auto first_key = (translated >> 16) & 0xFF)
        PostEvent(evcode, (mkvkey << 8) | first_key);
    
    if(auto second_key = translated & 0xFF)
    {
        uint32_t msg = (mkvkey << 8) | second_key;
        PostEvent(evcode, msg);
        ROMlib_SetAutokey(down ? msg : -1);
    }
}

void VideoDriverCallbacks::suspendEvent()
{
    sendsuspendevent();
}

void VideoDriverCallbacks::resumeEvent(bool updateClipboard /* TODO: does this really make sense? */)
{
    sendresumeevent(updateClipboard);
}
