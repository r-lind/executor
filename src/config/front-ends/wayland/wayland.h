#pragma once

#include <vdriver/vdriver.h>

#include <wayland-client.hpp>
#include <wayland-client-protocol-extra.hpp>

class WaylandVideoDriver : public Executor::VideoDriver
{
    using VideoDriver::VideoDriver;

    wayland::display_t display;
    wayland::compositor_t compositor;
    wayland::shell_t shell;
    wayland::xdg_wm_base_t xdg_wm_base;
    wayland::seat_t seat;
    wayland::shm_t shm;

    wayland::surface_t surface;
    wayland::xdg_surface_t xdg_surface;
    wayland::xdg_toplevel_t xdg_toplevel;
    

public:
    virtual void setColors(int num_colors, const Executor::vdriver_color_t *color_array) override;
    virtual bool setMode(int width, int height, int bpp,
                                bool grayscale_p) override;

    virtual void pumpEvents() override;
};
