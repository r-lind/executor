#pragma once

#include <vdriver/vdriver.h>

class SDLVideoDriver : public Executor::VideoDriver
{
public:
    bool init() override;
    void shutdown() override;
    bool isAcceptableMode(int width, int height, int bpp, bool grayscale_p, bool exact_match_p) override;
    bool setMode(int width, int height, int bpp, bool grayscale_p) override;
    void setColors(int first_color, int num_colors, const Executor::ColorSpec *colors) override;
    void updateScreenRects(int num_rects, const Executor::vdriver_rect_t *r, bool cursor_p) override;
    void pumpEvents() override;
    void setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y) override;
    bool setCursorVisible(bool show_p) override;
    void setTitle(const std::string& title) override;
    std::string getTitle() override;
    void setUseScancodes(bool val) override;
    Executor::LONGINT getScrap(Executor::LONGINT type, Executor::Handle h) override;
    void putScrap(Executor::LONGINT type, Executor::LONGINT length, char *p, int scrap_count) override;
};
