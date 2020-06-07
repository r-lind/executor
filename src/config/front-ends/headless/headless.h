#pragma once

#include <vdriver/vdriver.h>

class HeadlessVideoDriver : public Executor::VideoDriver
{
    using VideoDriver::VideoDriver;

    virtual void setColors(int num_colors, const Executor::vdriver_color_t *color_array) override;
    virtual bool setMode(int width, int height, int bpp,
                                bool grayscale_p) override;
};