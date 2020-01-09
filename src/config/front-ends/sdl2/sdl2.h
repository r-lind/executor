#pragma once

#include <vdriver/vdriver.h>

class SDL2VideoDriver : public Executor::VideoDriver
{
public:
    using VideoDriver::VideoDriver;
    
    bool init() override;
    bool setMode(int width, int height, int bpp, bool grayscale_p) override;
    void setColors(int first_color, int num_colors, const Executor::ColorSpec *colors) override;
    void updateScreenRects(int num_rects, const Executor::vdriver_rect_t *r, bool cursor_p) override;
    void pumpEvents() override;
    void setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y) override;
    bool setCursorVisible(bool show_p) override;
};