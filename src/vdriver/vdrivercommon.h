#pragma once

#include "vdriver.h"
#include <vdriver/dirtyrect.h>

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

    std::vector<int16_t> rootlessRegion_ = { 32767 };
    std::vector<int16_t> pendingRootlessRegion_;
    bool rootlessRegionDirty_ = false;

    Executor::DirtyRects dirtyRects_;

    void updateBuffer(const Framebuffer& fb, uint32_t* buffer, int bufferWidth, int bufferHeight,
                    const Executor::DirtyRects::Rects& rects);

    virtual void requestUpdate() = 0;

    void commitRootlessRegion();

public:
    using VideoDriver::VideoDriver;

    void setColors(int num_colors, const Executor::vdriver_color_t *color_array) override;
    void setRootlessRegion(RgnHandle rgn) override;
    void updateScreen(int top, int left, int bottom, int right) override;
};

}