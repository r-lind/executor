#pragma once

#include <vdriver/vdrivercommon.h>

class X11VideoDriver : public Executor::VideoDriverCommon
{
public:
    using VideoDriverCommon::VideoDriverCommon;
    
    bool parseCommandLine(int& argc, char *argv[]) override;
    bool init() override;
    void shutdown() override;
    bool isAcceptableMode(int width, int height, int bpp, bool grayscale_p, bool exact_match_p) override;
    bool setMode(int width, int height, int bpp, bool grayscale_p) override;
    void updateScreenRects(int num_rects, const Executor::vdriver_rect_t *r) override;
    void pumpEvents() override;
    void setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y) override;
    bool setCursorVisible(bool show_p) override;
    void registerOptions() override;
    void beepAtUser() override;
    void putScrap(Executor::OSType type, Executor::LONGINT length, char *p, int scrap_count) override;
    void weOwnScrap(void) override;
    int getScrap(Executor::OSType type, Executor::Handle h) override;
    void setTitle(const std::string& newtitle) override;

private:
    void alloc_x_window(int width, int height, int bpp, bool grayscale_p);
};