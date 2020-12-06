#pragma once

#include "vdriver.h"

#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <vector>
#include <type_traits>

namespace Executor
{

class ThreadedVideoDriver : public VideoDriver, public IVideoDriverCallbacks
{
    std::unique_ptr<VideoDriver> driver_;
    std::thread thread_;

    std::mutex mutex_;
    std::vector<std::function<void ()>> todo_;

    std::atomic<bool> modeChangeRequested_, updatesDoneRequested_;

    void mouseButtonEvent(bool down) override;
    void mouseMoved(int h, int v) override;
    void keyboardEvent(bool down, unsigned char mkvkey) override;
    void suspendEvent() override;
    void resumeEvent(bool updateClipboard) override;

    void runOnEmulatorThread(std::function<void ()> f);

    void modeAboutToChange() override;
    void requestUpdatesDone() override;

    template<typename F>
    std::invoke_result_t<F> runOnGuiThreadSync(F f);

public:
    ThreadedVideoDriver(VideoDriverCallbacks *cb, std::unique_ptr<VideoDriver> drv);
    virtual ~ThreadedVideoDriver();


    bool parseCommandLine(int& argc, char *argv[]) override;
    bool setOptions(std::unordered_map<std::string, std::string> options) override;
    void registerOptions() override;

    bool init() override;
    bool isAcceptableMode(int width, int height, int bpp, bool grayscale_p) override;
    bool setMode(int width, int height, int bpp, bool grayscale_p) override;
    void setColors(int num_colors, const vdriver_color_t *colors) override;

    void updateScreenRects(int num_rects, const Executor::vdriver_rect_t *r) override;

    void pumpEvents() override;

    void setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y) override;
    void setCursorVisible(bool show_p) override;

    void setRootlessRegion(Executor::RgnHandle rgn) override;

    void beepAtUser() override;

    void noteUpdatesDone() override;
    bool updateMode() override;
};

template<typename Driver>
class TThreadedVideoDriver : public ThreadedVideoDriver
{
public:
    TThreadedVideoDriver(VideoDriverCallbacks *cb)
        : ThreadedVideoDriver(cb, std::make_unique<Driver>(this))
    {}
};

}