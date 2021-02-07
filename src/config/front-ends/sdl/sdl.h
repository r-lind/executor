#pragma once

#include <vdriver/vdriver.h>

#include <vector>
#include <functional>
#include <condition_variable>

typedef struct SDL_Surface SDLSurface;

class SDLVideoDriver : public Executor::VideoDriver
{
public:
    SDLVideoDriver(Executor::IEventListener *listener, int& argc, char* argv[]);
    ~SDLVideoDriver();

    void registerOptions() override;
    bool setOptions(std::unordered_map<std::string, std::string> options) override;
    bool isAcceptableMode(int width, int height, int bpp, bool grayscale_p) override;
    bool setMode(int width, int height, int bpp, bool grayscale_p) override;
    void setColors(int num_colors, const Executor::vdriver_color_t *colors) override;
    void setCursor(char *cursor_data, uint16_t cursor_mask[16], int hotspot_x, int hotspot_y) override;
    void setCursorVisible(bool show_p) override;
    void setTitle(const std::string& title) override;
    Executor::LONGINT getScrap(Executor::LONGINT type, Executor::Handle h) override;
    void putScrap(Executor::LONGINT type, Executor::LONGINT length, char *p, int scrap_count) override;

    void runEventLoop() override;
    void endEventLoop() override;

private:
    void requestUpdate() override;
    void onMainThread(std::function<void()> f); 

    std::vector<std::function<void()>> todos_;
    std::condition_variable done_;
    bool quit_ = false;

    SDLSurface *screen;
};
