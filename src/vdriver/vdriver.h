
#if !defined(_VDRIVER_H_)
#define _VDRIVER_H_

#include <unordered_map>
#include <string>

#include <ExMacTypes.h>

#include <memory>

namespace Executor
{
struct ColorSpec;
struct Region;
typedef Region *RgnPtr;
typedef GUEST<RgnPtr> *RgnHandle;
struct rgb_spec_t;
}

namespace Executor
{

/* Minimum screen size we'll allow. */
#define VDRIVER_MIN_SCREEN_WIDTH 512
#define VDRIVER_MIN_SCREEN_HEIGHT 342

/* Suggested default screen size (may be safely ignored). */
#define VDRIVER_DEFAULT_SCREEN_WIDTH 1024
#define VDRIVER_DEFAULT_SCREEN_HEIGHT 768

struct vdriver_rect_t
{
    int top, left, bottom, right;
};

struct vdriver_color_t
{
    uint16_t red, green, blue;
};

struct Framebuffer
{
    std::shared_ptr<uint8_t[]> data;
    int width = 0, height = 0;
    int bpp = 0;
    int rowBytes = 0;
    int cursorBpp = 1;
    bool grayscale = false;
    bool rootless = false;
    rgb_spec_t *rgbSpec = nullptr;

    Framebuffer() = default;
    Framebuffer(int w, int h, int d);
};

class IVideoDriverCallbacks
{
public:
    void mouseButtonEvent(bool down, int h, int v)
    {
        mouseMoved(h,v);
        mouseButtonEvent(down);
    }

    virtual void mouseButtonEvent(bool down) = 0;
    virtual void mouseMoved(int h, int v) = 0;
    virtual void keyboardEvent(bool down, unsigned char mkvkey) = 0;
    virtual void suspendEvent() = 0;
    virtual void resumeEvent(bool updateClipboard /* TODO: does this really make sense? */) = 0;

    virtual void modeAboutToChange() {}
    virtual void requestUpdatesDone() {}
};

class VideoDriverCallbacks : public IVideoDriverCallbacks
{
public:
    virtual void mouseButtonEvent(bool down) override;
    virtual void mouseMoved(int h, int v) override;
    virtual void keyboardEvent(bool down, unsigned char mkvkey) override;
    virtual void suspendEvent() override;
    virtual void resumeEvent(bool updateClipboard) override;
private:
    GUEST<uint32_t> keytransState = 0;
};

class VideoDriver
{
public:
    VideoDriver(IVideoDriverCallbacks *cb) : callbacks_(cb) {}
    void setCallbacks(IVideoDriverCallbacks *cb) { callbacks_ = cb; }

    virtual ~VideoDriver();

    virtual bool parseCommandLine(int& argc, char *argv[]);
    virtual bool setOptions(std::unordered_map<std::string, std::string> options);
    virtual void registerOptions();

    virtual bool init();
    
    void updateScreen(int top, int left, int bottom, int right);
    void updateScreen() { updateScreen(0,0,height(),width()); }
    virtual void updateScreenRects(int num_rects, const vdriver_rect_t *r);
    virtual bool isAcceptableMode(int width, int height, int bpp, bool grayscale_p);
    virtual void setColors(int num_colors, const vdriver_color_t *colors) = 0;
    virtual bool setMode(int width, int height, int bpp, bool grayscale_p) = 0;

    virtual void putScrap(OSType type, LONGINT length, char *p, int scrap_cnt);
    virtual LONGINT getScrap(OSType type, Handle h);
    virtual void weOwnScrap();

    virtual void setTitle(const std::string& name);

    virtual void setCursor(char *cursor_data,
                                uint16_t cursor_mask[16],
                                int hotspot_x, int hotspot_y);
    virtual void setCursorVisible(bool show_p);


    virtual void pumpEvents();
    virtual void setRootlessRegion(RgnHandle rgn);

        // TODO: should move to sound driver?
    virtual void beepAtUser();

    virtual void runEventLoop() {}  // fixme: shouldn't really be optional?
    virtual void runOnThread(std::function<void ()> f) {}
    virtual void endEventLoop() {}

    virtual void noteUpdatesDone() {}
    virtual bool updateMode() { return false; }

    virtual bool handleMenuBarDrag() { return false; }

    uint8_t *framebuffer() { return framebuffer_.data.get(); }
    int cursorDepth() { return framebuffer_.cursorBpp; }
    int width() { return framebuffer_.width; }
    int height() { return framebuffer_.height; }
    int rowBytes() { return framebuffer_.rowBytes; }
    int bpp() { return framebuffer_.bpp; }
    rgb_spec_t *rgbSpec() { return framebuffer_.rgbSpec; }
    bool isGrayscale() { return framebuffer_.grayscale; }
    bool isRootless() { return framebuffer_.rootless; }

public:
    IVideoDriverCallbacks *callbacks_ = nullptr;

    Framebuffer framebuffer_;
};

extern std::unique_ptr<VideoDriver> vdriver;

// #define VDRIVER_DIRTY_RECT_BYTE_ALIGNMENT number-of-bytes

}
#endif /* !_VDRIVER_H_ */
