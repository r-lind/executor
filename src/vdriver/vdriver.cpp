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

void VideoDriver::updateScreen(int top, int left, int bottom, int right)
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