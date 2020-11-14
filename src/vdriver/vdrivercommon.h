#pragma once

#include "vdriver.h"
#include <array>
#include <vector>

namespace Executor
{

class VideoDriverCommon : public VideoDriver
{
protected:
    std::array<uint32_t, 256> colors_;

    std::vector<int16_t> rootlessRegion_;

    void updateBuffer(uint32_t* buffer, int bufferWidth, int bufferHeight,
                    int num_rects, const vdriver_rect_t *rects);

public:
    using VideoDriver::VideoDriver;

    void setColors(int num_colors, const Executor::vdriver_color_t *color_array) override;
    void setRootlessRegion(RgnHandle rgn) override;
};

}