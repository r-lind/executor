#include "wayland.h"
#include <../x/x_keycodes.h>

#include <iostream>
#include <algorithm>

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/input-event-codes.h>

#include <QuickDraw.h>
#include <quickdraw/region.h>

using namespace wayland;
using namespace Executor;

WaylandVideoDriver::SharedMem::SharedMem(size_t size)
    : size_(size)
{
    fd_ = memfd_create("executor", MFD_CLOEXEC);
    ftruncate(fd_, size_);
    mem_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
}

WaylandVideoDriver::SharedMem::~SharedMem()
{
    if(mem_)
    {
        munmap(mem_, size_);
        close(fd_);
    }
}

WaylandVideoDriver::Buffer::Buffer(shm_t& shm, int w, int h)
    : mem_(w * h * 4), 
    wlbuffer_(shm.create_pool(mem_.fd(), mem_.size())
            .create_buffer(0, w, h, w*4, wayland::shm_format::argb8888)),
    width_(w), height_(h)
{
}


void WaylandVideoDriver::setColors(int num_colors, const Executor::vdriver_color_t *color_array)
{
    for(int i = 0; i < num_colors; i++)
    {
        colors_[i] = (0xFF << 24)
                    | ((color_array[i].red >> 8) << 16)
                    | ((color_array[i].green >> 8) << 8)
                    | (color_array[i].blue >> 8);
    }
}

bool WaylandVideoDriver::init()
{
    rootlessRegion_ = { 0, 0, RGN_STOP, RGN_STOP };

    registry_ = display_.get_registry();
    registry_.on_global() = [this] (uint32_t name, const std::string& interface, uint32_t version) {
            if(interface == compositor_t::interface_name)
                registry_.bind(name, compositor_, version);
            else if(interface == shell_t::interface_name)
                registry_.bind(name, shell_, version);
            else if(interface == xdg_wm_base_t::interface_name)
                registry_.bind(name, xdg_wm_base_, version);
            else if(interface == seat_t::interface_name)
                registry_.bind(name, seat_, version);
            else if(interface == shm_t::interface_name)
                registry_.bind(name, shm_, version);
            // TODO: handle changes
        };
    display_.roundtrip();

    xdg_wm_base_.on_ping() = [this] (uint32_t serial) { xdg_wm_base_.pong(serial); };

    surface_ = compositor_.create_surface();
    xdg_surface_ = xdg_wm_base_.get_xdg_surface(surface_);
    xdg_toplevel_ = xdg_surface_.get_toplevel();
    xdg_toplevel_.set_title("Executor");
    xdg_toplevel_.set_app_id("io.github.autc04.executor");

    xdg_toplevel_.on_configure() = [this] (int32_t x, int32_t y, array_t states) { 
        if(x && y && !initDone_)
        {
            width_ = x;
            height_ = y;
        }

        std::vector<xdg_toplevel_state> states1 = states;

        std::cout << "toplevel configure " << x << " " << y << "\n";
    };

    xdg_surface_.on_configure() = [this] (uint32_t serial) {
        xdg_surface_.ack_configure(serial);

        if(width_ == buffer_.width() && height_ == buffer_.height())
            return;

        //updateScreenRects(0,nullptr,false);

        buffer_ = Buffer(shm_, width_, height_);

        std::fill(buffer_.data(), buffer_.data() + width_ * height_ * 4, 0);
        

        surface_.attach(buffer_.wlbuffer(), 0, 0);
        surface_.commit();
    };
    xdg_toplevel_.on_close() = [this] () { };

    pointer_ = seat_.get_pointer();
    pointer_.on_button() = [this] (uint32_t serial, uint32_t time, uint32_t button, pointer_button_state state) {
        std::cout << "button: " << button << " " << (int)state << std::endl;
        if(button == BTN_LEFT)
            callbacks_->mouseButtonEvent(state == pointer_button_state::pressed);
    };
    pointer_.on_motion() = [this] (uint32_t serial, double x, double y) {
        std::cout << "motion: " << x << " " << y << std::endl;
        callbacks_->mouseMoved(x, y);
    };

    keyboard_ = seat_.get_keyboard();
    keyboard_.on_key() = [this] (uint32_t serial, uint32_t time, uint32_t key, keyboard_key_state state) {
        bool down = state == keyboard_key_state::pressed;

        std::cout << "key: " << std::hex << key << std::dec << " " << (down ? "down" : "up") << std::endl;

        auto mkvkey = x_keycode_to_mac_virt[key + 8];

        callbacks_->keyboardEvent(down, mkvkey);
    };

    keyboard_.on_enter() = [this] (uint32_t serial, wayland::surface_t, wayland::array_t) {
        callbacks_->resumeEvent(true);
    };
    
    keyboard_.on_leave() = [this] (uint32_t serial, wayland::surface_t) {
        callbacks_->suspendEvent();
    };


    pointer_.on_enter() = [this](uint32_t serial, surface_t surface, double x, double y) {
        pointer_.set_cursor(serial, cursorSurface_, hotSpot_.first, hotSpot_.second);
        cursorEnterSerial_ = serial;
    };


    cursorBuffer_ = Buffer(shm_, 16,16);
    cursorSurface_ = compositor_.create_surface();
    hotSpot_ = {0,0};
    


    width_ = 1024;
    height_ = 768;


    xdg_toplevel_.set_maximized();

    return true;
}

bool WaylandVideoDriver::setMode(int width, int height, int bpp,
                                  bool grayscale_p)
{
    bpp_ = bpp ? bpp : 8;


	surface_.commit();



    display_.roundtrip();


    cursorSurface_.attach(cursorBuffer_.wlbuffer(), 0, 0);
    cursorSurface_.commit();
    //pointer_.set_cursor(0, cursorSurface_, hotSpot_.first, hotSpot_.second);
    //display_.roundtrip();
    rowBytes_ = ((width_ * bpp_ + 31) & ~31) / 8;
    framebuffer_ = new uint8_t[rowBytes_ * height_];
    initDone_ = true;

    isRootless_ = true;
    return true;
}

void WaylandVideoDriver::pumpEvents()
{
    //display_.dispatch();
    display_.roundtrip();
}

void WaylandVideoDriver::setRootlessRegion(RgnHandle rgn)
{
    if((*rgn)->rgnSize == 10)
    {
        rootlessRegion_.clear();
        rootlessRegion_.insert(rootlessRegion_.end(),
            { (*rgn)->rgnBBox.top.get(), (*rgn)->rgnBBox.left.get(), (*rgn)->rgnBBox.right.get(), RGN_STOP,
            (*rgn)->rgnBBox.bottom.get(), (*rgn)->rgnBBox.left.get(), (*rgn)->rgnBBox.right.get(), RGN_STOP,
            RGN_STOP });
    }
    else
    {
        GUEST<uint16_t> *p = (GUEST<uint16_t>*) ((*(Handle)rgn) + 10);
        GUEST<uint16_t> *q = (GUEST<uint16_t>*) ((*(Handle)rgn) + (*rgn)->rgnSize);
        rootlessRegion_.clear();
        rootlessRegion_.insert(rootlessRegion_.end(), p, q);
    }


    RegionProcessor rgnP(rootlessRegion_.begin());

    region_t waylandRgn = compositor_.create_region();

    while(rgnP.bottom() < height_)
    {
        rgnP.advance();
        
        for(int i = 0; i + 1 < rgnP.row.size(); i += 2)
            waylandRgn.add(rgnP.row[i], rgnP.top(), rgnP.row[i+1] - rgnP.row[i], rgnP.bottom() - rgnP.top());
    }

    surface_.set_input_region(waylandRgn);
}

template<int depth>
struct IndexedPixelGetter
{
    uint8_t *src;
    int shift;
    std::array<uint32_t, 256>& colors;

    static constexpr uint8_t mask = (1 << depth) - 1;

    IndexedPixelGetter(std::array<uint32_t, 256>& colors, uint8_t *line, int x)
        : colors(colors)
    {
        src = line + x * depth / 8;
        shift = 8 - (x * depth % 8) - depth;
    }

    uint32_t operator() ()
    {
        auto c = colors[(*src >> shift) & mask];
        shift -= depth;
        if(shift < 0)
        {
            ++src;
            shift = 8 - depth;
        }
        return c;
    }
};

void WaylandVideoDriver::updateScreenRects(
    int num_rects, const vdriver_rect_t *r,
    bool cursor_p)
{
    std::cout << "update.\n";
    for(int i = 0; i < num_rects; i++)
        std::cout << r[i].left << ", " << r[i].top << " - " << r[i].right << ", " << r[i].bottom << std::endl;
    uint32_t *screen = reinterpret_cast<uint32_t*>(buffer_.data());

    for(int i = 0; i < num_rects; i++)
    {
        RegionProcessor rgnP(rootlessRegion_.begin());

        for(int y = r[i].top; y < r[i].bottom; y++)
        {
            while(y >= rgnP.bottom())
                rgnP.advance();

            auto blitLine = [this, &rgnP, screen, y, r, i](auto getPixel) {
                auto rowIt = rgnP.row.begin();
                int x = r[i].left;

                while(x < r[i].right)
                {
                    int nextX = std::min(r[i].right, (int)*rowIt++);

                    for(; x < nextX; x++)
                    {
                        uint32_t pixel = getPixel();
                        screen[y * width_ + x] = pixel == 0xFFFFFFFF ? 0 : pixel;
                    }
                    
                    if(x >= r[i].right)
                        break;

                    nextX = std::min(r[i].right, (int)*rowIt++);

                    for(; x < nextX; x++)
                        screen[y * width_ + x] = getPixel();
                }
            };

            uint8_t *src = framebuffer_ + y * rowBytes_;
            switch(bpp_)
            {
                case 8:
                    src += r[i].left;
                    blitLine([&] { return colors_[*src++]; });
                    break;

                case 1: 
                    blitLine(IndexedPixelGetter<1>(colors_, src, r[i].left));
                    break;
                case 2: 
                    blitLine(IndexedPixelGetter<2>(colors_, src, r[i].left));
                    break;
                case 4: 
                    blitLine(IndexedPixelGetter<4>(colors_, src, r[i].left));
                    break;
                case 16:
                    {
                        auto *src16 = reinterpret_cast<GUEST<uint16_t>*>(src);
                        src16 += r[i].left;
                        blitLine([&] { 
                            uint16_t pix = *src16++;
                            auto fiveToEight = [](uint32_t x) {
                                return (x << 3) | (x >> 2);
                            };
                            return 0xFF000000
                                | (fiveToEight((pix >> 10) & 31) << 16)
                                | (fiveToEight((pix >> 5) & 31) << 8)
                                | fiveToEight(pix & 31);
                        });
                    }
                    break;

                case 32:
                    {
                        auto *src32 = reinterpret_cast<GUEST<uint32_t>*>(src);
                        src32 += r[i].left;
                        blitLine([&] { return (*src32++) | 0xFF000000; });
                    }
                    break;

            }
        }
    }

    for(int i = 0; i < num_rects; i++)
        surface_.damage_buffer(r[i].left,r[i].top,r[i].right-r[i].left,r[i].bottom-r[i].top);
    surface_.attach(buffer_.wlbuffer(), 0, 0);
    surface_.commit();
    display_.flush();
}

void WaylandVideoDriver::setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y)
{
    uint32_t *dst = reinterpret_cast<uint32_t*>(cursorBuffer_.data());
    uint8_t *data = reinterpret_cast<uint8_t*>(cursor_data);
    uint8_t *mask = reinterpret_cast<uint8_t*>(cursor_mask);
    for(int y = 0; y < 16; y++)
        for(int x = 0; x < 16; x++)
        {
            if(mask[2*y + x/8] & (0x80 >> x%8))
                *dst++ = (data[2*y + x/8] & (0x80 >> x%8)) ? 0xFF000000 : 0xFFFFFFFF;
            else
                *dst++ = (data[2*y + x/8] & (0x80 >> x%8)) ? 0xFF000000 : 0;
        }
    cursorSurface_.damage_buffer(0,0,16,16);
    cursorSurface_.attach(cursorBuffer_.wlbuffer(), 0, 0);

    hotSpot_ = { hotspot_x, hotspot_y };
    cursorSurface_.commit();
    pointer_.set_cursor(cursorEnterSerial_, cursorSurface_, hotSpot_.first, hotSpot_.second);
}

bool WaylandVideoDriver::setCursorVisible(bool show_p)
{
    pointer_.set_cursor(cursorEnterSerial_, show_p ? cursorSurface_ : surface_t(), hotSpot_.first, hotSpot_.second);
    return true;
}
