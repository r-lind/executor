#pragma once

#include <vdriver/vdriver.h>

class SDLVideoDriver : public Executor::VideoDriver
{
public:
    using VideoDriver::VideoDriver;
    
    void registerOptions() override;
    bool init() override;
    bool setOptions(std::unordered_map<std::string, std::string> options) override;
    void shutdown() override;
    bool isAcceptableMode(int width, int height, int bpp, bool grayscale_p) override;
    bool setMode(int width, int height, int bpp, bool grayscale_p) override;
    void setColors(int num_colors, const Executor::vdriver_color_t *colors) override;
    void updateScreenRects(int num_rects, const Executor::vdriver_rect_t *r) override;
    void pumpEvents() override;
    void setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y) override;
    bool setCursorVisible(bool show_p) override;
    void setTitle(const std::string& title) override;
    Executor::LONGINT getScrap(Executor::LONGINT type, Executor::Handle h) override;
    void putScrap(Executor::LONGINT type, Executor::LONGINT length, char *p, int scrap_count) override;
};
