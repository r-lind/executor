#pragma once

#include "vdriver.h"
#include <array>
#include <vector>
#include <mutex>
#include <type_traits>
#include <future>

namespace Executor
{

class VideoDriverCommon : public VideoDriver
{
protected:
    std::mutex mutex_;

    std::array<uint32_t, 256> colors_;

    std::vector<int16_t> rootlessRegion_;
    std::vector<int16_t> pendingRootlessRegion_;
    bool rootlessRegionDirty_ = false;

    void updateBuffer(const Framebuffer& fb, uint32_t* buffer, int bufferWidth, int bufferHeight,
                    int num_rects, const vdriver_rect_t *rects);

public:
    using VideoDriver::VideoDriver;

    void setColors(int num_colors, const Executor::vdriver_color_t *color_array) override;
    void setRootlessRegion(RgnHandle rgn) override;
};

}