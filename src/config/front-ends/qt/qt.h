#pragma once

#include <vdriver/vdrivercommon.h>

class QRect;

class QtVideoDriver : public Executor::VideoDriverCommon
{
public:
    QtVideoDriver(Executor::IEventListener *eventListener, int& argc, char* argv[]);
    ~QtVideoDriver();

    void setRootlessRegion(Executor::RgnHandle rgn) override;
    bool setMode(int width, int height, int bpp, bool grayscale_p) override;
    void updateScreenRects(int num_rects, const Executor::vdriver_rect_t *r) override;
    void setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y) override;
    void setCursorVisible(bool show_p) override;
    void runEventLoop() override;
    void endEventLoop() override;

private:
    void convertRect(QRect r);
};