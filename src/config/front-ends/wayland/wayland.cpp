#include "wayland.h"
#include <iostream>

using namespace wayland;

void WaylandVideoDriver::setColors(int num_colors, const Executor::vdriver_color_t *color_array)
{
}
bool WaylandVideoDriver::setMode(int width, int height, int bpp,
                                  bool grayscale_p)
{
    if(xdg_wm_base)
        return true;

    auto registry = display.get_registry();
    registry.on_global() = [&] (uint32_t name, const std::string& interface, uint32_t version) {
            if(interface == compositor_t::interface_name)
                registry.bind(name, compositor, version);
            else if(interface == shell_t::interface_name)
                registry.bind(name, shell, version);
            else if(interface == xdg_wm_base_t::interface_name)
                registry.bind(name, xdg_wm_base, version);
            else if(interface == seat_t::interface_name)
                registry.bind(name, seat, version);
            else if(interface == shm_t::interface_name)
                registry.bind(name, shm, version);
            // TODO: handle changes
        };
    display.roundtrip();

    // TODO: 
    xdg_wm_base.on_ping() = [this] (uint32_t serial) { xdg_wm_base.pong(serial); };

    surface = compositor.create_surface();
    xdg_surface = xdg_wm_base.get_xdg_surface(surface);
    xdg_toplevel = xdg_surface.get_toplevel();
    xdg_toplevel.set_title("Executor");
    xdg_toplevel.set_app_id("executor");

    xdg_toplevel.on_configure() = [this] (int32_t x, int32_t y, array_t states) { 
        //configuredX = std::max(0,x);
        //configuredY = std::max(0,y);
        
        std::vector<xdg_toplevel_state> states1 = states;

        std::cout << "toplevel configure " << x << " " << y << "\n";
    };



    width_ = 512;
    height_ = 342;
    bpp_ = 1;
    rowBytes_ = ((width_ * bpp_ + 31) & ~31) / 8;
    framebuffer_ = new uint8_t[rowBytes_ * height_];
    return true;
}

void WaylandVideoDriver::pumpEvents()
{
    display.dispatch();
}