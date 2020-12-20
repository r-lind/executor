#pragma once

#include <vdriver/vdriver.h>
#include <util/static_vector.h>

#include <array>

namespace Executor
{

class DirtyRects
{
    constexpr static int MAX_DIRTY_RECTS = 5;

    static_vector<vdriver_rect_t, MAX_DIRTY_RECTS> rects_;

public:
    void add(int top, int left, int bottom, int right);

    bool empty() const { return rects_.empty(); }

    static_vector<vdriver_rect_t, MAX_DIRTY_RECTS> getAndClear();
};

extern void dirty_rect_accrue(int top, int left, int bottom, int right);
extern void dirty_rect_update_screen(void);
}
