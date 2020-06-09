#pragma once

#include <vdriver/vdriver.h>

#include <wayland-client.hpp>
#include <wayland-client-protocol-extra.hpp>

class WaylandVideoDriver : public Executor::VideoDriver
{
    using VideoDriver::VideoDriver;

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


    
    std::array<uint32_t, 256> colors_;

    class SharedMem
    {
        int fd_;
        void *mem_ = nullptr;
        size_t size_;
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
        int width_, height_;
    public:
        Buffer() = default;
        Buffer(wayland::shm_t& shm, int w, int h);

        int width() const { return width_; }
        int height() const { return height_; }
        uint8_t *data() { return (uint8_t*)mem_.data(); }

        wayland::buffer_t& wlbuffer() { return wlbuffer_; }
    };


    Buffer buffer_;
    bool initDone_ = false;


    wayland::surface_t cursorSurface_;
    std::pair<int,int> hotSpot_;
    Buffer cursorBuffer_;
    uint32_t cursorEnterSerial_ = 0;

public:
    void setColors(int num_colors, const Executor::vdriver_color_t *color_array) override;
    bool setMode(int width, int height, int bpp,
                                bool grayscale_p) override;
    void updateScreenRects(int num_rects, const Executor::vdriver_rect_t *r,
                                       bool cursor_p) override;

    void pumpEvents() override;

    void setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y) override;
    bool setCursorVisible(bool show_p) override;
};
