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
void ThreadedVideoDriver::framebufferSetupChanged()
{
    runOnEmulatorThread([this]() {
        callbacks_->framebufferSetupChanged();
    });
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

    width_ = driver_->width();
    height_ = driver_->height();
    bpp_ = driver_->bpp();
    rowBytes_ = driver_->rowBytes();
    isGrayscale_ = driver_->isGrayscale();
    isRootless_ = driver_->isRootless();
    cursorDepth_ = driver_->cursorDepth();
    rgbSpec_ = driver_->rgbSpec();
    framebuffer_ = driver_->framebuffer();

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
bool ThreadedVideoDriver::setCursorVisible(bool show_p)
{
    return runOnGuiThreadSync([=]() {
        return driver_->setCursorVisible(show_p);
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

