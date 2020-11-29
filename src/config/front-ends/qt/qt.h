#pragma once

#include <vdriver/vdrivercommon.h>

class QRect;

class QtVideoDriver : public Executor::VideoDriverCommon
{
public:
    using VideoDriverCommon::VideoDriverCommon;
    
    void setRootlessRegion(Executor::RgnHandle rgn) override;
    bool parseCommandLine(int& argc, char *argv[]) override;
    bool setMode(int width, int height, int bpp, bool grayscale_p) override;
    void updateScreenRects(int num_rects, const Executor::vdriver_rect_t *r) override;
    void pumpEvents() override;
    void setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y) override;
    void setCursorVisible(bool show_p) override;

private:
    void convertRect(QRect r);
};