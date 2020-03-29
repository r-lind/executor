#pragma once

#include <vdriver/vdriver.h>

class X11VideoDriver : public Executor::VideoDriver
{
public:
    using VideoDriver::VideoDriver;
    
    bool parseCommandLine(int& argc, char *argv[]) override;
    bool init() override;
    void shutdown() override;
    bool isAcceptableMode(int width, int height, int bpp, bool grayscale_p, bool exact_match_p) override;
    bool setMode(int width, int height, int bpp, bool grayscale_p) override;
    void setColors(int num_colors, const Executor::vdriver_color_t *colors) override;
    void updateScreenRects(int num_rects, const Executor::vdriver_rect_t *r, bool cursor_p) override;
    void pumpEvents() override;
    void setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y) override;
    bool setCursorVisible(bool show_p) override;
    void registerOptions() override;
    void flushDisplay() override;
    void beepAtUser() override;
    void putScrap(Executor::OSType type, Executor::LONGINT length, char *p, int scrap_count) override;
    void weOwnScrap(void) override;
    int getScrap(Executor::OSType type, Executor::Handle h) override;
    void setTitle(const std::string& newtitle) override;
    std::string getTitle(void) override;

private:
    void alloc_x_window(int width, int height, int bpp, bool grayscale_p);
};