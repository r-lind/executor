#pragma once

#include "vdriver.h"
#include <array>
#include <vector>
#include <thread>
#include <mutex>
#include <type_traits>
#include <future>

namespace Executor
{

class VideoDriverCommon : public VideoDriver
{
protected:
    std::thread thread_;
    std::mutex mutex_;

    std::array<uint32_t, 256> colors_;

    std::vector<int16_t> rootlessRegion_;

    void updateBuffer(uint32_t* buffer, int bufferWidth, int bufferHeight,
                    int num_rects, const vdriver_rect_t *rects);

    virtual void runOnThread(std::function<void ()> f) = 0;

    template<typename F>
    std::invoke_result_t<F> runOnGuiThreadSync(F f)
    {
        using T = std::invoke_result_t<F>;

        std::promise<T> promise;
        std::future<T> future = promise.get_future();

        runOnThread([&]() {
            try
            {
                if constexpr(std::is_same_v<T, void>)
                {
                    f();
                    promise.set_value();
                }
                else
                {
                    promise.set_value(f());
                }
            }
            catch(...)
            {
                promise.set_exception(std::current_exception());
            }
        });

        return future.get();
    }

public:
    VideoDriverCommon(IEventListener *cb);
    ~VideoDriverCommon();

    void setColors(int num_colors, const Executor::vdriver_color_t *color_array) override;
    void setRootlessRegion(RgnHandle rgn) override;
};

}