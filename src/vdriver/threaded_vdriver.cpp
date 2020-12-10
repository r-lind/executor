#include "threaded_vdriver.h"

#include <future>

using namespace Executor;

ThreadedVideoDriver::ThreadedVideoDriver(VideoDriverCallbacks *cb, std::unique_ptr<VideoDriver> drv)
    : VideoDriver(cb), driver_(std::move(drv))
{
}
ThreadedVideoDriver::~ThreadedVideoDriver()
{
    driver_->endEventLoop();
    thread_.join();
}

void ThreadedVideoDriver::runOnEmulatorThread(std::function<void ()> f)
{
    std::lock_guard lk(mutex_);
    todo_.push_back(f);
}

template<typename F>
std::invoke_result_t<F> ThreadedVideoDriver::runOnGuiThreadSync(F f)
{
    using T = std::invoke_result_t<F>;

    std::promise<T> promise;
    std::future<T> future = promise.get_future();

    driver_->runOnThread([&]() {
        try
        {
            if constexpr(std::is_same_v<T, void>)
            {
                f();
                promise.set_value();
            }
            else
            {
                promise.set_value(f());
            }
        }
        catch(...)
        {
            promise.set_exception(std::current_exception());
        }
    });

    return future.get();
}


void ThreadedVideoDriver::pumpEvents()
{
    std::vector<std::function<void()>> todo;
    {
        std::lock_guard lk(mutex_);
        todo = std::move(todo_);
    }
    for(const auto& f : todo)
        f();
}

bool ThreadedVideoDriver::init()
{
    driver_->init();
    thread_ = std::thread([this]{
        driver_->runEventLoop();
    });

    return true;
}



void ThreadedVideoDriver::mouseButtonEvent(bool down)
{
    runOnEmulatorThread([=]() {
        callbacks_->mouseButtonEvent(down);
    });
}
void ThreadedVideoDriver::mouseMoved(int h, int v)
{
    runOnEmulatorThread([=]() {
        callbacks_->mouseMoved(h,v);
    });
}
void ThreadedVideoDriver::keyboardEvent(bool down, unsigned char mkvkey)
{
    runOnEmulatorThread([=]() {
        callbacks_->keyboardEvent(down, mkvkey);
    });
}
void ThreadedVideoDriver::suspendEvent()
{
    runOnEmulatorThread([this]() {
        callbacks_->suspendEvent();
    });
}
void ThreadedVideoDriver::resumeEvent(bool updateClipboard)
{
    runOnEmulatorThread([=]() {
        callbacks_->resumeEvent(updateClipboard);
    });
}
void ThreadedVideoDriver::modeAboutToChange()
{
    modeChangeRequested_ = true;
}

void ThreadedVideoDriver::requestUpdatesDone()
{
    updatesDoneRequested_ = true;
}


bool ThreadedVideoDriver::parseCommandLine(int& argc, char *argv[])
{
    return driver_->parseCommandLine(argc, argv);
}
bool ThreadedVideoDriver::setOptions(std::unordered_map<std::string, std::string> options)
{
    return driver_->setOptions(options);
}
void ThreadedVideoDriver::registerOptions()
{
    driver_->registerOptions();
}

bool ThreadedVideoDriver::isAcceptableMode(int width, int height, int bpp, bool grayscale_p)
{
    return driver_->isAcceptableMode(width, height, bpp, grayscale_p);
}
bool ThreadedVideoDriver::setMode(int width, int height, int bpp, bool grayscale_p)
{
    bool success = runOnGuiThreadSync([=]() {
        return driver_->setMode(width, height, bpp, grayscale_p);
    });

    if (!success)
        return false;

    framebuffer_ = driver_->framebuffer_;   // ###

    return true;
}

void ThreadedVideoDriver::setColors(int num_colors, const vdriver_color_t *colors)
{
    runOnGuiThreadSync([=]() {
        driver_->setColors(num_colors, colors);
    });
}

void ThreadedVideoDriver::updateScreenRects(int num_rects, const Executor::vdriver_rect_t *r)
{
    driver_->runOnThread([this, rects = std::vector<Executor::vdriver_rect_t>(r, r + num_rects)]() {
        driver_->updateScreenRects(rects.size(), rects.data());
    });
}

void ThreadedVideoDriver::setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y)
{
    runOnGuiThreadSync([=]() {
        driver_->setCursor(cursor_data, cursor_mask, hotspot_x, hotspot_y);
    });
}
void ThreadedVideoDriver::setCursorVisible(bool show_p)
{
    driver_->runOnThread([=]() {
        driver_->setCursorVisible(show_p);
    });
}
void ThreadedVideoDriver::setRootlessRegion(Executor::RgnHandle rgn)
{
    runOnGuiThreadSync([=]() {
        driver_->setRootlessRegion(rgn);
    });
}
void ThreadedVideoDriver::beepAtUser()
{
    driver_->runOnThread([this]() {
        driver_->beepAtUser();
    });
}

void ThreadedVideoDriver::noteUpdatesDone()
{
    bool expected = true;
    if(!updatesDoneRequested_.compare_exchange_strong(expected, false))
        return;

    driver_->runOnThread([this]() {
        driver_->noteUpdatesDone();
    });
}

bool ThreadedVideoDriver::updateMode()
{
    bool expected = true;
    if(!modeChangeRequested_.compare_exchange_strong(expected, false))
        return false;

    return runOnGuiThreadSync([this]() {
        bool modeChanged = driver_->updateMode();

        if(modeChanged)
            framebuffer_ = driver_->framebuffer_;
        
        return modeChanged;
    });
}

bool ThreadedVideoDriver::handleMenuBarDrag()
{
    return runOnGuiThreadSync([this]() {
        return driver_->handleMenuBarDrag();
    });
}
