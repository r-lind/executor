#include <vdriver/vdriver.h>
#include <error/error.h>

using namespace Executor;

std::unique_ptr<VideoDriver> Executor::vdriver;

Framebuffer::Framebuffer(int w, int h, int d)
    : width(w), height(h), bpp(d)
{
    rowBytes = ((width * bpp + 31) & ~31) / 8;
    data = std::shared_ptr<uint8_t>(new uint8_t[rowBytes * height], [](uint8_t* p) { delete[] p; });
}

VideoDriver::~VideoDriver()
{
}

bool VideoDriver::setOptions(std::unordered_map<std::string, std::string> options)
{
    return true;
}

void VideoDriver::updateScreen(int top, int left, int bottom, int right)
{
    if(top < 0)
        top = 0;
    if(left < 0)
        left = 0;

    if(bottom > height())
        bottom = height();
    if(right > width())
        right = width();

    vdriver_rect_t r = {top, left, bottom, right};
    updateScreenRects(1, &r);
}

void VideoDriver::updateScreenRects(int num_rects, const vdriver_rect_t *r)
{
}

bool VideoDriver::isAcceptableMode(int width, int height, int bpp, bool grayscale_p)
{
    if(width && width < VDRIVER_MIN_SCREEN_WIDTH)
        return false;
    if(height && height < VDRIVER_MIN_SCREEN_HEIGHT)
        return false;
    if(bpp & (bpp - 1))
        return false;
    if(bpp > 32)
        return false;
    return true;
}

void VideoDriver::registerOptions()
{
}

void VideoDriver::putScrap(OSType type, LONGINT length, char *p, int scrap_cnt)
{
}

LONGINT VideoDriver::getScrap(OSType type, Handle h)
{
    return 0;
}

void VideoDriver::weOwnScrap()
{
}

void VideoDriver::setTitle(const std::string& name)
{
}

void VideoDriver::setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y)
{
}

void VideoDriver::setCursorVisible(bool show_p)
{
}

void VideoDriver::beepAtUser()
{
}

void VideoDriver::setRootlessRegion(RgnHandle rgn)
{
}