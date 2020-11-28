
#if !defined(_VDRIVER_H_)
#define _VDRIVER_H_

#include <unordered_map>
#include <string>

#include <ExMacTypes.h>

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

}

namespace Executor
{

/*
 * VideoDriverCallbacks
 * 
 * This will probably become an abstract class that should be the only calls that
 * video drivers make into the rest of Executor.
 */
class VideoDriverCallbacks
{
public:
    void mouseButtonEvent(bool down, int h, int v)
    {
        mouseMoved(h,v);
        mouseButtonEvent(down);
    }

    virtual void mouseButtonEvent(bool down);
    virtual void mouseMoved(int h, int v);
    virtual void keyboardEvent(bool down, unsigned char mkvkey);
    virtual void suspendEvent();
    virtual void resumeEvent(bool updateClipboard /* TODO: does this really make sense? */);

    virtual void framebufferSetupChanged();
private:
    GUEST<uint32_t> keytransState = 0;
};

class VideoDriver
{
public:
    VideoDriver(VideoDriverCallbacks *cb) : callbacks_(cb) {}
    void setCallbacks(VideoDriverCallbacks *cb) { callbacks_ = cb; }

    virtual ~VideoDriver();

    virtual bool parseCommandLine(int& argc, char *argv[]);
    virtual bool setOptions(std::unordered_map<std::string, std::string> options);
    virtual void registerOptions();

    virtual bool init();
    
    virtual void shutdown();
    virtual void updateScreen(int top, int left, int bottom, int right);
    void updateScreen() { updateScreen(0,0,height(),width()); }
    virtual void updateScreenRects(int num_rects, const vdriver_rect_t *r);
    virtual bool isAcceptableMode(int width, int height, int bpp, bool grayscale_p);
    virtual void setColors(int num_colors, const vdriver_color_t *colors) = 0;
    virtual bool setMode(int width, int height, int bpp,
                                bool grayscale_p) = 0;

    virtual void putScrap(OSType type, LONGINT length, char *p, int scrap_cnt);
    virtual LONGINT getScrap(OSType type, Handle h);
    virtual void weOwnScrap();

    virtual void setTitle(const std::string& name);

    virtual void setCursor(char *cursor_data,
                                uint16_t cursor_mask[16],
                                int hotspot_x, int hotspot_y);
    virtual bool setCursorVisible(bool show_p);


    virtual void pumpEvents();
    virtual void setRootlessRegion(RgnHandle rgn);

        // TODO: should move to sound driver?
    virtual void beepAtUser();

    int cursorDepth() { return cursorDepth_; }
    uint8_t *framebuffer() { return framebuffer_; }
    int width() { return width_; }
    int height() { return height_; }
    int rowBytes() { return rowBytes_; }
    int bpp() { return bpp_; }
    int maxBpp() { return maxBpp_; }
    rgb_spec_t *rgbSpec() { return rgbSpec_; }
    bool isGrayscale() { return isGrayscale_; }
    bool isRootless() { return isRootless_; }

public:
    VideoDriverCallbacks *callbacks_ = nullptr;

    uint8_t* framebuffer_ = nullptr;
    int width_ = VDRIVER_DEFAULT_SCREEN_WIDTH;
    int height_ = VDRIVER_DEFAULT_SCREEN_HEIGHT;
    int bpp_ = 8;
    int rowBytes_ = VDRIVER_DEFAULT_SCREEN_WIDTH;
    bool isGrayscale_ = false;

    int maxBpp_ = 8;
    int cursorDepth_ = 1;
    bool isRootless_ = false;

    rgb_spec_t *rgbSpec_ = nullptr;
};

extern VideoDriver *vdriver;

// #define VDRIVER_DIRTY_RECT_BYTE_ALIGNMENT number-of-bytes

}
#endif /* !_VDRIVER_H_ */
