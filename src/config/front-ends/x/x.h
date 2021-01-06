#pragma once

#include <vdriver/vdrivercommon.h>

class X11VideoDriver : public Executor::VideoDriverCommon
{
public:    
    X11VideoDriver(Executor::IEventListener *listener, int& argc, char* argv[]);
    ~X11VideoDriver();

    bool setMode(int width, int height, int bpp, bool grayscale_p) override;
    void setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y) override;
    void setCursorVisible(bool show_p) override;
    void registerOptions() override;
    void beepAtUser() override;
    void putScrap(Executor::OSType type, Executor::LONGINT length, char *p, int scrap_count) override;
    void weOwnScrap(void) override;
    int getScrap(Executor::OSType type, Executor::Handle h) override;
    void setTitle(const std::string& newtitle) override;

    void runEventLoop() override;
    void endEventLoop() override;

private:
    void alloc_x_window(int width, int height, int bpp, bool grayscale_p);
    void requestUpdate() override;
    void render(std::unique_lock<std::mutex>& lk, std::optional<Executor::DirtyRects::Rect> rect);
    void handleEvents();
};