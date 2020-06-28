#include <vdriver/vdriver.h>
#include <error/error.h>

using namespace Executor;

VideoDriver *Executor::vdriver = nullptr;

VideoDriver::~VideoDriver()
{
}

bool VideoDriver::parseCommandLine(int& argc, char *argv[])
{
    return true;
}

bool VideoDriver::init()
{
    return true;
}

bool VideoDriver::setOptions(std::unordered_map<std::string, std::string> options)
{
    return true;
}

void VideoDriver::shutdown()
{
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

bool VideoDriver::isAcceptableMode(int width, int height, int bpp, bool grayscale_p, bool exact_match_p)
{
    if(bpp == 1 || bpp == 4 || bpp == 8 || bpp == 16 || bpp == 32)
        return true;
    else
        return false;
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

bool VideoDriver::setCursorVisible(bool show_p)
{
    return true;
}

void VideoDriver::beepAtUser()
{
}

void VideoDriver::pumpEvents()
{
}

void VideoDriver::setRootlessRegion(RgnHandle rgn)
{
}