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
#include <quickdraw/region.h>

#include "available_geometry.h"

//#include "keycode_map.h"

#include <iostream>
#include <memory>
#include <unordered_map>

#ifdef __APPLE__
void macosx_hide_menu_bar(int mouseX, int mouseY, int width, int height);
void macosx_autorelease_pool(const std::function<void ()>& fun);
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
    IEventListener *callbacks_;
public:
    ExecutorWindow(IEventListener *callbacks)
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

    void mouseMoveEvent(QMouseEvent *ev)
    {
#ifdef __APPLE__
        macosx_hide_menu_bar(ev->x(), ev->y(), width(), height());
#endif
        callbacks_->mouseMoved(ev->x(), ev->y());
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


QtVideoDriver::QtVideoDriver(Executor::IEventListener *eventListener, int& argc, char* argv[])
    : VideoDriverCommon(eventListener)
{
    std::mutex initEndMutex;
    std::condition_variable initEndCond;
    bool initEnded = false;

    thread_ = std::thread([&] {
        qapp = new QGuiApplication(argc, argv);

        {
            std::unique_lock lk(initEndMutex);
            initEnded = true;
            initEndCond.notify_all();
        }
        qapp->exec();
    });
    
    std::unique_lock lk(initEndMutex);
    initEndCond.wait(lk, [&] { return initEnded; });
}

QtVideoDriver::~QtVideoDriver()
{
    std::cout << "quitting.\n";
    QMetaObject::invokeMethod(qapp, "quit");

    thread_.join();
}


std::optional<QRegion> rootlessRegion;

void QtVideoDriver::setRootlessRegion(RgnHandle rgn)
{
    VideoDriverCommon::setRootlessRegion(rgn);
    QMetaObject::invokeMethod(qapp, [=] {

        if(!rootlessRegionDirty_)
            return;
        rootlessRegion_ = pendingRootlessRegion_;

        RegionProcessor rgnP(rootlessRegion_.begin());

        QRegion qtRgn;

        int height = framebuffer_.height;

        while(rgnP.bottom() < height)
        {
            rgnP.advance();
            
            for(int i = 0; i + 1 < rgnP.row.size(); i += 2)
                qtRgn += QRect(rgnP.row[i], rgnP.top(), rgnP.row[i+1] - rgnP.row[i], rgnP.bottom() - rgnP.top());
        }
        
    #ifdef __APPLE__
        macosx_autorelease_pool([&] {
    #endif
            window->setMask(qtRgn);
    #ifdef __APPLE__
        });
    #endif

        rootlessRegion = qtRgn;
    });
}


bool QtVideoDriver::setMode(int width, int height, int bpp, bool grayscale_p)
{
    std::mutex initEndMutex;
    std::condition_variable initEndCond;
    bool initEnded = false;

    QMetaObject::invokeMethod(qapp, [=, &initEndMutex, &initEndCond, &initEnded] {
    #ifdef __APPLE__
        macosx_hide_menu_bar(500, 0, 1000, 1000);
        QVector<QRect> screenGeometries = getScreenGeometries();
    #else
        QVector<QRect> screenGeometries = getAvailableScreenGeometries();
    #endif

        printf("set_mode: %d %d %d\n", width, height, bpp);
        
        QRect geom = screenGeometries[0];
        for(const QRect& r : screenGeometries)
            if(r.width() * r.height() > geom.width() * geom.height())
                geom = r;

        framebuffer_ = Framebuffer(geom.width(), geom.height(), bpp ? bpp : 8);
        framebuffer_.rootless = true;

        qimage = new QImage(framebuffer_.width, framebuffer_.height, QImage::Format_RGB32);
        
        if(!window)
            window = new ExecutorWindow(callbacks_);
        window->setGeometry(geom);
        window->showMaximized();

        {
            std::unique_lock lk(initEndMutex);
            initEnded = true;
            initEndCond.notify_all();
        }

    });

    std::unique_lock lk(initEndMutex);
    initEndCond.wait(lk, [&] { return initEnded; });

    return true;
}

void QtVideoDriver::updateScreenRects(int num_rects, const vdriver_rect_t *rectP)
{
    std::vector<vdriver_rect_t> r(rectP, rectP+num_rects);
    QMetaObject::invokeMethod(qapp, [=] {
        QRegion rgn;
        for(int i = 0; i < r.size(); i++)
        {
            rgn += QRect(r[i].left, r[i].top, r[i].right-r[i].left, r[i].bottom-r[i].top);
            std::cout << "update " << r[i].left << " " << r[i].top << " " << r[i].right << " " << r[i].bottom << std::endl;
        }

        updateBuffer(framebuffer_, (uint32_t*)qimage->bits(), qimage->width(), qimage->height(),
            num_rects, r.data());

        window->update(rgn);
    });
}

void QtVideoDriver::setCursor(char *cursor_data,
                              uint16_t cursor_mask[16],
                              int hotspot_x, int hotspot_y)
{
    QMetaObject::invokeMethod(qapp, [=] {
        static QCursor theCursor(Qt::ArrowCursor);

        if(cursor_data)
        {
            uchar data2[32];
            uchar mask2[32];
            memcpy(data2, cursor_data, 32);
            memcpy(mask2, cursor_mask, 32);
            
            for(int i = 0; i<32; i++)
                mask2[i] |= data2[i];
            QBitmap crsr = QBitmap::fromData(QSize(16, 16), (const uchar*)data2, QImage::Format_Mono);
            QBitmap mask = QBitmap::fromData(QSize(16, 16), (const uchar*)mask2, QImage::Format_Mono);
            
            theCursor = QCursor(crsr, mask, hotspot_x, hotspot_y);
        }
        window->setCursor(theCursor);   // TODO: should we check for visibility?
    });
}

void QtVideoDriver::setCursorVisible(bool show_p)
{
    QMetaObject::invokeMethod(qapp, [=] {
        if(show_p)
            setCursor(nullptr, nullptr, 0, 0);
        else
            window->setCursor(Qt::BlankCursor);
    });
}
