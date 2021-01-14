#include "vdrivercommon.h"
#include <quickdraw/region.h>

#include <future>
#include <iostream>

using namespace Executor;

void VideoDriverCommon::setColors(int num_colors, const Executor::vdriver_color_t *color_array)
{
    std::lock_guard lk(mutex_);
    for(int i = 0; i < num_colors; i++)
    {
        colors_[i] = (0xFF << 24)
                    | ((color_array[i].red >> 8) << 16)
                    | ((color_array[i].green >> 8) << 8)
                    | (color_array[i].blue >> 8);
    }
}


void VideoDriverCommon::setRootlessRegion(RgnHandle rgn)
{
    std::lock_guard lk(mutex_);

    RgnVector inRgn(rgn);
    pendingRootlessRegion_.clear();
        
    if(inRgn.size())
        pendingRootlessRegion_.insert(pendingRootlessRegion_.end(), inRgn.begin(), inRgn.end());
    else
        pendingRootlessRegion_.insert(pendingRootlessRegion_.end(),
            { (*rgn)->rgnBBox.top.get(), (*rgn)->rgnBBox.left.get(), (*rgn)->rgnBBox.right.get(), RGN_STOP,
            (*rgn)->rgnBBox.bottom.get(), (*rgn)->rgnBBox.left.get(), (*rgn)->rgnBBox.right.get(), RGN_STOP,
            RGN_STOP });
    rootlessRegionDirty_ = true;
    requestUpdate();
}

void VideoDriverCommon::updateScreen(int top, int left, int bottom, int right)
{
    std::lock_guard lk(mutex_);
    dirtyRects_.add(top, left, bottom, right);
    requestUpdate();
}

void VideoDriverCommon::commitRootlessRegion()
{
    // calculate symmetric difference (XorRgn) and add to dirtyRects.
    std::vector<int16_t> rgnDiff(rootlessRegion_.size() + pendingRootlessRegion_.size());

    auto p = rootlessRegion_.begin();
    auto q = pendingRootlessRegion_.begin();
    auto dst = rgnDiff.begin();

    while(*p < RGN_STOP || *q < RGN_STOP)
    {
        if(*p < *q)
            while((*dst++ = *p++) < RGN_STOP)
                ;
        else if(*p > *q)
            while((*dst++ = *q++) < RGN_STOP)
                ;
        else
        {
            auto startOfLine = dst;
            *dst++ = *p++;
            q++;

            while(*p < RGN_STOP || *q < RGN_STOP)
            {
                if(*p < *q)
                    *dst++ = *p++;
                else if(*p > *q)
                    *dst++ = *q++;
                else
                    p++, q++;
            }

            if(dst == startOfLine + 1)
                dst = startOfLine;
            else
                *dst++ = RGN_STOP;
                
            p++, q++;
        }
    }
    *dst++ = RGN_STOP;

    forEachRect(rgnDiff.begin(), [this](int l, int t, int r, int b) {
        //std::cout << "region changed: " << l << " " << t << " " << r << " " << b << std::endl;
        dirtyRects_.add(t, l, b, r);
    });

    rootlessRegion_ = pendingRootlessRegion_;
    rootlessRegionDirty_ = false;
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

void VideoDriverCommon::updateBuffer(const Framebuffer& fb, uint32_t* buffer, int bufferWidth, int bufferHeight,
                                const Executor::DirtyRects::Rects& rects)
{
    auto before = std::chrono::high_resolution_clock::now();

    if(!rootlessRegion_.size())
        rootlessRegion_.insert(rootlessRegion_.end(),
            { 0, 0, (int16_t)bufferWidth, RGN_STOP,
            (int16_t)bufferHeight, 0, (int16_t)bufferWidth, RGN_STOP,
            RGN_STOP });
    
    int width = std::min(fb.width, bufferWidth);
    int height = std::min(fb.height, bufferHeight);

    for(const auto& r : rects)
    {
        assert(r.left >= 0 && r.right >= 0 && r.left <= width && r.bottom <= height);
        
        RegionProcessor rgnP(rootlessRegion_.begin());

        for(int y = r.top; y < r.bottom; y++)
        {
            while(y >= rgnP.bottom())
                rgnP.advance();

            auto blitLine = [this, &rgnP, buffer, bufferWidth, bufferHeight, y, &r](auto getPixel) {
                auto rowIt = rgnP.row.begin();
                int x = r.left;

                while(x < r.right)
                {
                    int nextX = std::min(r.right, (int)*rowIt++);

                    for(; x < nextX; x++)
                    {
                        uint32_t pixel = getPixel();
                        buffer[y * bufferWidth + x] = pixel == 0xFFFFFFFF ? 0 : pixel;
                    }
                    
                    if(x >= r.right)
                        break;

                    nextX = std::min(r.right, (int)*rowIt++);

                    for(; x < nextX; x++)
                        buffer[y * bufferWidth + x] = getPixel();
                }
            };

            uint8_t *src = fb.data.get() + y * fb.rowBytes;
            switch(fb.bpp)
            {
                case 8:
                    src += r.left;
                    blitLine([&] { return colors_[*src++]; });
                    break;

                case 1: 
                    blitLine(IndexedPixelGetter<1>(colors_, src, r.left));
                    break;
                case 2: 
                    blitLine(IndexedPixelGetter<2>(colors_, src, r.left));
                    break;
                case 4: 
                    blitLine(IndexedPixelGetter<4>(colors_, src, r.left));
                    break;
                case 16:
                    {
                        auto *src16 = reinterpret_cast<GUEST<uint16_t>*>(src);
                        src16 += r.left;
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
                        src32 += r.left;
                        blitLine([&] { return (*src32++) | 0xFF000000; });
                    }
                    break;

            }
        }
    }

    auto after = std::chrono::high_resolution_clock::now();
    static int sum = 0, count = 0;

    if(count > 100)
        count = sum = 0;

    sum += std::chrono::duration_cast<std::chrono::milliseconds>(after-before).count();
    count++;

    std::cout << double(sum)/count << std::endl;
}