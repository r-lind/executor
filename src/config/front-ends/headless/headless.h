#include <vdriver/vdriver.h>

class HeadlessVideoDriver : public Executor::VideoDriver
{
    virtual void setColors(int first_color, int num_colors,
                               const struct Executor::ColorSpec *color_array) override;
    virtual bool setMode(int width, int height, int bpp,
                                bool grayscale_p) override;
};