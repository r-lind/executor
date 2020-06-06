#include "qt.h"

#include <QGuiApplication>
#include <QPainter>
#include <QRasterWindow>
#include <QMouseEvent>
#include <QBitmap>
#include <QScreen>
#ifdef STATIC_WINDOWS_QT
#include <QtPlugin>
#endif

#include <optional>


#include <vdriver/vdriver.h>
#include <quickdraw/cquick.h> /* for ThePortGuard */

#include "available_geometry.h"

//#include "keycode_map.h"

#include <iostream>
#include <memory>
#include <unordered_map>

#ifdef __APPLE__
void macosx_hide_menu_bar(int mouseX, int mouseY, int width, int height);
#endif
#include <../x/x_keycodes.h>

#ifdef STATIC_WINDOWS_QT
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

constexpr bool log_key_events = false;

using namespace Executor;

namespace Executor
{
    extern std::unordered_map<Qt::Key,int> qtToMacKeycodeMap;
}

namespace
{
class ExecutorWindow;
QGuiApplication *qapp;
QImage *qimage;
ExecutorWindow *window;

class ExecutorWindow : public QRasterWindow
{
    VideoDriverCallbacks *callbacks_;
public:
    ExecutorWindow(VideoDriverCallbacks *callbacks)
        : callbacks_(callbacks)
    {
        setFlag(Qt::FramelessWindowHint, true);
        setFlag(Qt::NoDropShadowWindowHint, true);
    }

    void paintEvent(QPaintEvent *e)
    {
        QPainter painter(this);
        if(qimage)
        {
            for(const QRect& r : e->region())
                painter.drawImage(r, *qimage, r);
        }
    }

    void mousePressRelease(QMouseEvent *ev)
    {
        callbacks_->mouseButtonEvent(!!(ev->buttons() & Qt::LeftButton), ev->x(), ev->y());
    }
    void mousePressEvent(QMouseEvent *ev)
    {
        mousePressRelease(ev);
    }
    void mouseReleaseEvent(QMouseEvent *ev)
    {
        mousePressRelease(ev);
    }

    void keyEvent(QKeyEvent *ev, bool down_p)
    {
        unsigned char mkvkey;

        auto p = qtToMacKeycodeMap.find(Qt::Key(ev->key()));
        if(p == qtToMacKeycodeMap.end())
            mkvkey = 0x89;// NOTAKEY
        else
            mkvkey = p->second;
        if(mkvkey == 0x89 && ev->nativeScanCode() > 1 && ev->nativeScanCode() < std::size(x_keycode_to_mac_virt))
        {
            mkvkey = x_keycode_to_mac_virt[ev->nativeScanCode()];
            if constexpr(log_key_events)
                std::cout << "mkvkey: " << ev->nativeScanCode() << " -> " << std::hex << (int)mkvkey << std::dec << std::endl;
        }
#ifdef __APPLE__
        if(ev->nativeVirtualKey())
            mkvkey = ev->nativeVirtualKey();
#endif
        callbacks_->keyboardEvent(down_p, mkvkey);
    }
    
    void keyPressEvent(QKeyEvent *ev)
    {
        if constexpr(log_key_events)
            std::cout << "press: " << std::hex << ev->key() << " " << ev->nativeScanCode() << " " << ev->nativeVirtualKey() << std::dec << std::endl;
        if(!ev->isAutoRepeat())
            keyEvent(ev, true);
    }
    void keyReleaseEvent(QKeyEvent *ev)
    {
        if constexpr(log_key_events)
            std::cout << "release\n";
        if(!ev->isAutoRepeat())
            keyEvent(ev, false);
    }

    bool event(QEvent *ev)
    {
        switch(ev->type())
        {
            case QEvent::FocusIn:
                callbacks_->resumeEvent(true);
                break;
            case QEvent::FocusOut:
                callbacks_->suspendEvent();
                break;

            default:
                ;
        }
        return QRasterWindow::event(ev);
    }
};

}

std::optional<QBitmap> rootlessRegion;

void QtVideoDriver::setRootlessRegion(RgnHandle rgn)
{
    ThePortGuard guard;
    GrafPort grayRegionPort;

    C_OpenPort(&grayRegionPort);
    short grayRegionRowBytes = ((width() + 31) & ~31) / 8;
    grayRegionPort.portBits.baseAddr = (Ptr) framebuffer() + rowBytes() * height();
    grayRegionPort.portBits.rowBytes =  grayRegionRowBytes ;
    grayRegionPort.portBits.bounds = { 0, 0, height(), width() };
    grayRegionPort.portRect = grayRegionPort.portBits.bounds;

    memset(framebuffer() + rowBytes() * height(), 0, grayRegionRowBytes * height());

    C_SetPort(&grayRegionPort);
    C_PaintRgn(rgn);

    C_ClosePort(&grayRegionPort);

    rootlessRegion = QBitmap::fromData(
        QSize((width() + 31)&~31, height()),
        (const uchar*)grayRegionPort.portBits.baseAddr,
        QImage::Format_Mono);
    
    window->setMask(*rootlessRegion);
}

bool QtVideoDriver::parseCommandLine(int& argc, char *argv[])
{
    qapp = new QGuiApplication(argc, argv);
    return true;
}

bool QtVideoDriver::isAcceptableMode(int width, int height, int bpp,
                                      bool grayscale_p,
                                      bool exact_match_p)
{
    return bpp == 1 || bpp == 2 || bpp == 4 || bpp == 8;
}


bool QtVideoDriver::setMode(int width, int height, int bpp, bool grayscale_p)
{
#ifdef __APPLE__
    macosx_hide_menu_bar(500, 0, 1000, 1000);
    QVector<QRect> screenGeometries = getScreenGeometries();
#else
    QVector<QRect> screenGeometries = getAvailableScreenGeometries();
#endif

    printf("set_mode: %d %d %d\n", width, height, bpp);
    if(framebuffer_)
        delete[] framebuffer_;
    
    QRect geom = screenGeometries[0];
    for(const QRect& r : screenGeometries)
        if(r.width() * r.height() > geom.width() * geom.height())
            geom = r;

    width_ = geom.width();
    height_ = geom.height();

    isRootless_ = true;
    if(width)
        width_ = width;
    if(height)
        height_ = height;
    if(bpp)
        bpp_ = bpp;
    rowBytes_ = width_ * bpp_ / 8;
    rowBytes_ = (rowBytes_+3) & ~3;

    maxBpp_ = 8; //32;

    framebuffer_ = new uint8_t[rowBytes_ * height_ + width_ * height_];

    switch(bpp_)
    {
        case 1:
            qimage = new QImage(framebuffer_, width_, height_, rowBytes_, QImage::Format_Mono);
            break;
        case 2:
        case 4:
            qimage = new QImage(width_, height_, QImage::Format_Indexed8);
            break;
        case 8:
            qimage = new QImage(framebuffer_, width_, height_, rowBytes_, QImage::Format_Indexed8);
            break;
        case 16:
            qimage = new QImage(width_, height_, QImage::Format_RGB555);
            break;
        case 32:
            qimage = new QImage(width_, height_, QImage::Format_RGB32);
            break;
    }
    
    if(bpp_ <= 8)
        qimage->setColorTable({qRgb(0,0,0),qRgb(255,255,255)});

    if(!window)
        window = new ExecutorWindow(callbacks_);
    window->setGeometry(geom);
#ifdef __APPLE__
    window->show();
#else
    window->showMaximized();
#endif
    return true;
}
void QtVideoDriver::setColors(int num_colors, const vdriver_color_t *colors)
{
    if(bpp_ > 8)
        return;

    QVector<QRgb> qcolors(num_colors);
    for(int i = 0; i < num_colors; i++)
    {
        qcolors[i] = qRgb(
            colors[i].red >> 8,
            colors[i].green >> 8,
            colors[i].blue >> 8
        );
    }
    qimage->setColorTable(qcolors);
}

void QtVideoDriver::convertRect(QRect r)
{
    if(bpp_ == 2)
    {
        r.setLeft(r.left() & ~3);

        for(int y = r.top(); y <= r.bottom(); y++)
        {
            uint8_t *src = framebuffer_ + y * rowBytes_ + r.left() / 4;
            uint8_t *dst = qimage->scanLine(y) + r.left();

            for(int i = 0; i < (r.width() + 3) / 4; i++)
            {
                uint8_t packed = *src++;
                *dst++ = (packed >> 6) & 3;
                *dst++ = (packed >> 4) & 3;
                *dst++ = (packed >> 2) & 3;
                *dst++ = packed & 3;
            }
        }
    }
    else if(bpp_ == 4)
    {
        r.setLeft(r.left() & ~1);

        for(int y = r.top(); y <= r.bottom(); y++)
        {
            uint8_t *src = framebuffer_ + y * rowBytes_ + r.left() / 2;
            uint8_t *dst = qimage->scanLine(y) + r.left();

            for(int i = 0; i < (r.width() + 1) / 2; i++)
            {
                uint8_t packed = *src++;
                *dst++ = packed >> 4;
                *dst++ = packed & 0xF;
            }
        }
    }
    else if(bpp_ == 16)
    {
        for(int y = r.top(); y <= r.bottom(); y++)
        {
            auto src = (GUEST<uint16_t>*) (framebuffer_ + y * rowBytes_) + r.left();
            auto dst = (uint16_t*) (qimage->scanLine(y)) + r.left();

            for(int i = 0; i < r.width(); i++)
                *dst++ = *src++;
        }
    }
    else if(bpp_ == 32)
    {
        for(int y = r.top(); y <= r.bottom(); y++)
        {
            auto src = (GUEST<uint32_t>*) (framebuffer_ + y * rowBytes_) + r.left();
            auto dst = (uint32_t*) (qimage->scanLine(y)) + r.left();

            for(int i = 0; i < r.width(); i++)
                *dst++ = *src++;
        }
    }
}

void QtVideoDriver::updateScreenRects(int num_rects, const vdriver_rect_t *r,
                                      bool cursor_p)
{
    QRegion rgn;
    for(int i = 0; i < num_rects; i++)
    {
        rgn += QRect(r[i].left, r[i].top, r[i].right-r[i].left, r[i].bottom-r[i].top);
    }

    rgn &= QRect(0,0,width_,height_);
    if(bpp_ != 1 && bpp_ != 8)
        for(QRect rect : rgn)
            convertRect(rect);

    window->update(rgn);
    pumpEvents();
}

void QtVideoDriver::pumpEvents()
{
    qapp->processEvents();
    
    auto cursorPos = QCursor::pos();
#ifdef __APPLE__
    macosx_hide_menu_bar(cursorPos.x(), cursorPos.y(), window->width(), window->height());
#endif
    cursorPos = window->mapFromGlobal(cursorPos);
    callbacks_->mouseMoved(cursorPos.x(), cursorPos.y());

    static bool beenHere = false;
    if(!beenHere && rootlessRegion)
    {
        window->setMask(*rootlessRegion);
        beenHere = true;
    }
}


void QtVideoDriver::setCursor(char *cursor_data,
                              uint16_t cursor_mask[16],
                              int hotspot_x, int hotspot_y)
{
    static QCursor theCursor(Qt::ArrowCursor);

    if(cursor_data)
    {
        uchar data2[32];
        uchar *mask2 = (uchar*)cursor_mask;
        std::copy(cursor_data, cursor_data+32, data2);
        for(int i = 0; i<32; i++)
            mask2[i] |= data2[i];
        QBitmap crsr = QBitmap::fromData(QSize(16, 16), (const uchar*)data2, QImage::Format_Mono);
        QBitmap mask = QBitmap::fromData(QSize(16, 16), (const uchar*)mask2, QImage::Format_Mono);
        
        theCursor = QCursor(crsr, mask, hotspot_x, hotspot_y);
    }
    window->setCursor(theCursor);   // TODO: should we check for visibility?
}

bool QtVideoDriver::setCursorVisible(bool show_p)
{
    if(show_p)
        setCursor(nullptr, nullptr, 0, 0);
    else
        window->setCursor(Qt::BlankCursor);
    return true;
}
