
#if !defined(_VDRIVER_H_)
#define _VDRIVER_H_


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
#define VDRIVER_DEFAULT_SCREEN_WIDTH 640
#define VDRIVER_DEFAULT_SCREEN_HEIGHT 480

/* This is a macro to facilitate static declarations for hosts
 * where the number of modes is fixed. */
#define VDRIVER_MODE_LIST_TYPE(num_entries)                    \
    struct                                                     \
    {                                                          \
        int continuous_range_p; /* 2 entry inclusive range? */ \
        int num_sizes; /* StrLength of size[] array.  */          \
        struct                                                 \
        {                                                      \
            short width, height;                               \
        } size[num_entries];                                   \
    }

typedef VDRIVER_MODE_LIST_TYPE(0) vdriver_modes_t;

#define VDRIVER_MODE_LIST_SIZE(nelt) \
    (sizeof(vdriver_modes_t)         \
     + ((nelt) * sizeof(((vdriver_modes_t *)0)->size[0])))

typedef struct
{
    int top, left, bottom, right;
} vdriver_rect_t;

typedef enum {
    VDRIVER_ACCEL_NO_UPDATE,
    VDRIVER_ACCEL_FULL_UPDATE,
    VDRIVER_ACCEL_HOST_SCREEN_UPDATE_ONLY
} vdriver_accel_result_t;
}

/* We don't provide any accelerated display functions under SDL (yet) */
#define VDRIVER_BYPASS_INTERNAL_FBUF_P() false

namespace Executor
{

class VideoDriver
{
public:
    virtual ~VideoDriver();

    virtual bool parseCommandLine(int& argc, char *argv[]);
    virtual bool init();
    virtual void shutdown();
    virtual void updateScreen(int top, int left, int bottom, int right,
                                 bool cursor_p);
    virtual void updateScreenRects(int num_rects, const vdriver_rect_t *r,
                                       bool cursor_p);
    virtual bool isAcceptableMode(int width, int height, int bpp,
                                      bool grayscale_p,
                                      bool exact_match_p);
    virtual void setColors(int first_color, int num_colors,
                               const struct ColorSpec *color_array) = 0;
    virtual void getColors(int first_color, int num_colors,
                                struct ColorSpec *color_array);
    virtual bool setMode(int width, int height, int bpp,
                                bool grayscale_p) = 0;
    virtual void flushDisplay();
    virtual void registerOptions();

    virtual vdriver_accel_result_t accelFillRect(int top, int left,
                                                      int bottom, int right,
                                                      uint32_t color);

        // NOTE: this is never called, and there is no non-trivial implementation
    virtual vdriver_accel_result_t accelScrollRect(int top, int left,
                                                            int bottom, int right,
                                                            int dx, int dy);
    
        // NOTE: there is no non-trivial implementation
    virtual void accelWait();


    virtual void putScrap(OSType type, LONGINT length, char *p, int scrap_cnt);
    virtual LONGINT getScrap(OSType type, Handle h);
    virtual void weOwnScrap();

    virtual void setTitle(const std::string& name);
    virtual std::string getTitle();

        // X & SDL1/windows only.
        // TODO: should probably use registerOptions and add a custom option.
    virtual void setUseScancodes(bool val);

    

    virtual void setCursor(char *cursor_data,
                                uint16_t cursor_mask[16],
                                int hotspot_x, int hotspot_y);
    virtual bool setCursorVisible(bool show_p);

    // called only if VDRIVER_SUPPORTS_REAL_SCREEN_BLITS only:
    virtual bool hideCursorIfIntersects(int top, int left,
                                        int bottom, int right);


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
    bool isFixedCLUT() { return isFixedCLUT_; }
    bool isRootless() { return isRootless_; }

public:
    uint8_t* framebuffer_ = nullptr;
    int width_ = VDRIVER_DEFAULT_SCREEN_WIDTH;
    int height_ = VDRIVER_DEFAULT_SCREEN_HEIGHT;
    int bpp_ = 8;
    int rowBytes_ = VDRIVER_DEFAULT_SCREEN_WIDTH;
    bool isGrayscale_ = false;

    int maxBpp_ = 8;
    int cursorDepth_ = 1;
    bool isRootless_ = false;
    bool isFixedCLUT_ = false;

    rgb_spec_t *rgbSpec_ = nullptr;
};

extern VideoDriver *vdriver;

// TODO: none of the existing front ends define this, but there is a lot of conditionally-defined code
#if defined(VDRIVER_SUPPORTS_REAL_SCREEN_BLITS)

#if !defined(vdriver_flip_real_screen_pixels_p)
extern bool vdriver_flip_real_screen_pixels_p;
#endif

#if !defined(vdriver_real_screen_row_bytes)
extern int vdriver_real_screen_row_bytes;
#endif

#if !defined(vdriver_real_screen_baseaddr)
extern uint8_t *vdriver_real_screen_baseaddr;
#endif

#if !defined(vdriver_set_up_internal_screen)
extern void vdriver_set_up_internal_screen();
#endif

#endif /* VDRIVER_SUPPORTS_REAL_SCREEN_BLITS */

// #define VDRIVER_DIRTY_RECT_BYTE_ALIGNMENT number-of-bytes

}
#endif /* !_VDRIVER_H_ */
