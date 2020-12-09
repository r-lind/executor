#pragma once

#include <vdriver/vdrivercommon.h>

#include <wayland-client.hpp>
#include <wayland-client-protocol-extra.hpp>

#include <mutex>
#include <vector>

class WaylandVideoDriver : public Executor::VideoDriverCommon
{
    class SharedMem
    {
        int fd_ = -1;
        void *mem_ = nullptr;
        size_t size_ = 0;
    public:
        SharedMem() = default;
        SharedMem(size_t size);
        ~SharedMem();

        SharedMem(const SharedMem&) = delete;
        SharedMem& operator=(const SharedMem&) = delete;

        friend void swap(SharedMem& a, SharedMem& b) noexcept
        {
            using std::swap;
            swap(a.fd_, b.fd_);
            swap(a.mem_, b.mem_);
            swap(a.size_, b.size_);
        }

        SharedMem(SharedMem&& other) noexcept
        {
            swap(*this, other);
        }

        SharedMem& operator=(SharedMem&& other) noexcept
        {
            SharedMem moved(std::move(other));
            swap(*this, moved);
            return *this;
        }

        int fd() const { return fd_; }
        void *data() const { return mem_; }
        size_t size() const { return size_; }
    };

    class Buffer
    {
        SharedMem mem_;
        wayland::buffer_t wlbuffer_;
        int width_ = 0, height_ = 0;
    public:
        Buffer() = default;
        Buffer(wayland::shm_t& shm, int w, int h);

        int width() const { return width_; }
        int height() const { return height_; }
        uint32_t *data() { return (uint32_t*)mem_.data(); }

        wayland::buffer_t& wlbuffer() { return wlbuffer_; }
    };

    wayland::display_t display_;
    wayland::registry_t registry_;
    wayland::compositor_t compositor_;
    wayland::shell_t shell_;
    wayland::xdg_wm_base_t xdg_wm_base_;
    wayland::seat_t seat_;
    wayland::shm_t shm_;

    wayland::surface_t surface_;
    wayland::xdg_surface_t xdg_surface_;
    wayland::xdg_toplevel_t xdg_toplevel_;

    wayland::pointer_t pointer_;
    wayland::keyboard_t keyboard_;

    bool frameRequested_ = false;
    wayland::callback_t frameCallback_;

    Buffer buffer_;
    bool initDone_ = false;


    wayland::surface_t cursorSurface_;
    std::pair<int,int> hotSpot_;
    Buffer cursorBuffer_;
    uint32_t cursorEnterSerial_ = 0;



    int requestedBpp_ = 8;

    struct WindowState
    {
        uint32_t serial;
        int width, height;
        bool maximized;
    };

    WindowState configuredState_ {};
    WindowState committedState_ {};

    bool activated_ = false;


    double mouseX_, mouseY_;

    int wakeFd_;
    bool exitMainThread_ = false;
    std::mutex executeOnUiThreadMutex_;
    std::vector<std::function<void ()>> executeOnUiThreadQueue_;


    std::vector<Executor::vdriver_rect_t> rectsToUpdate_;

    void commitBuffer();

    bool delayingUpdates_ = false;
    std::chrono::high_resolution_clock::time_point delayingUpdatesUntil_;
public:
    using VideoDriverCommon::VideoDriverCommon;

    bool init() override;
    bool setMode(int width, int height, int bpp,
                                bool grayscale_p) override;
    void updateScreenRects(int num_rects, const Executor::vdriver_rect_t *r) override;

    void pumpEvents() override;

    void setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y) override;
    void setCursorVisible(bool show_p) override;

    void setRootlessRegion(Executor::RgnHandle rgn) override;

    void noteUpdatesDone() override;
    bool updateMode() override;

    void runEventLoop() override;
    void runOnThread(std::function<void ()> f) override;
    void endEventLoop() override;
};
