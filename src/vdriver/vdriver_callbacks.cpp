#include "vdriver.h"

#include <rsys/adb.h>
#include <rsys/scrap.h>
#include <osevent/osevent.h>
#include <rsys/keyboard.h>
#include <ToolboxEvent.h>

using namespace Executor;

void VideoDriverCallbacks::mouseButtonEvent(bool down, int h, int v)
{
    if(down)
        modifiers &= ~btnState;
    else
        modifiers |= btnState;

    ROMlib_PPostEvent(down ? mouseDown : mouseUp,
                        0, nullptr, TickCount(), Point{v,h},
                        modifiers);
    adb_apeiron_hack(false);
}

void VideoDriverCallbacks::mouseMoved(int h, int v)
{
    LM(MouseLocation).h = h;
    LM(MouseLocation).v = v;

    adb_apeiron_hack(false);
}

void VideoDriverCallbacks::keyboardEvent(bool down, unsigned char mkvkey)
{
    mkvkey = ROMlib_right_to_left_key_map(mkvkey);

    short modchange = 0;
    switch(mkvkey)
    {
        case MKV_CAPS:
            modchange = alphaLock;
            break;
        case MKV_CLOVER:
            modchange = cmdKey;
            break;
        case MKV_LEFTSHIFT:
            modchange = shiftKey;
            break;
        case MKV_LEFTOPTION:
            modchange = optionKey;
            break;
        case MKV_LEFTCNTL:
            modchange = ControlKey;
            break;
    }
    if(down)
        modifiers |= modchange;
    else
        modifiers &= ~modchange;

    auto when = TickCount();
    Point where = LM(MouseLocation);
    auto keywhat = ROMlib_xlate(mkvkey, modifiers, down);
    post_keytrans_key_events(down ? keyDown : keyUp,
                            keywhat, when, where,
                            modifiers, mkvkey);
}
void VideoDriverCallbacks::suspendEvent()
{
    sendsuspendevent();
}
void VideoDriverCallbacks::resumeEvent(bool updateClipboard /* TODO: does this really make sense? */)
{
    sendresumeevent(updateClipboard);
}
