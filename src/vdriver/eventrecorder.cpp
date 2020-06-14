#include "eventrecorder.h"
#include <time/time.h>
#include <iostream>

using namespace Executor;

EventPlayback* EventPlayback::instance = nullptr;

void EventRecorder::mouseButtonEvent(bool down)
{
    out << msecs_elapsed() << " 1 " << (down ? 1 : 0) << "\n" << std::flush;
    VideoDriverCallbacks::mouseButtonEvent(down);
}
void EventRecorder::mouseMoved(int h, int v)
{
    if(h == lastH || v == lastV)
        return;

    lastH = h;
    lastV = v;

    out << msecs_elapsed() << " 2 " << h << " " << v << "\n";
    VideoDriverCallbacks::mouseMoved(h, v);
}
void EventRecorder::keyboardEvent(bool down, unsigned char mkvkey)
{
    out << msecs_elapsed() << " 3 " << (down ? 1 : 0) << " " << (int)mkvkey << "\n" << std::flush;
    VideoDriverCallbacks::keyboardEvent(down, mkvkey);
}


EventPlayback::EventPlayback(fs::path fn)
    : in(fn), playbackActive(true)
{
    in >> nextEvent;
    if(!in)
        playbackActive = false;

    instance = this;
}

void EventPlayback::mouseButtonEvent(bool down)
{
    playbackActive = false;    
    VideoDriverCallbacks::mouseButtonEvent(down);
}
void EventPlayback::mouseMoved(int h, int v)
{
    if(!playbackActive)
        VideoDriverCallbacks::mouseMoved(h, v);
}
void EventPlayback::keyboardEvent(bool down, unsigned char mkvkey)
{
    playbackActive = false;    
    VideoDriverCallbacks::keyboardEvent(down, mkvkey);
}

void EventPlayback::pumpEvents()
{
    while(playbackActive && msecs_elapsed() >= nextEvent)
    {
        int type, h, v, down, key;
        in >> type;
        std::cout << "playback " << nextEvent << " " << type << std::endl;
        switch(type)
        {
            case 1:
                in >> down;
                VideoDriverCallbacks::mouseButtonEvent(down);
                break;
            case 2:
                in >> h >> v;
                VideoDriverCallbacks::mouseMoved(h, v);
                break;
            case 3:
                in >> down >> key;
                VideoDriverCallbacks::keyboardEvent(down, key);
                break;
        }
        in >> nextEvent;
        if(!in)
            playbackActive = false;
    }
}

EventPlayback* EventPlayback::getInstance()
{
    return instance;
}