#include "gtest/gtest.h"

#ifdef EXECUTOR
#define AUTOMATIC_CONVERSIONS
#include <ListMgr.h>
#include <WindowMgr.h>
#include <MemoryMgr.h>

using namespace Executor;

#define PTR(x) (&inout(x))

#else
#include <Lists.h>
#include <Windows.h>

#define PTR(x) (&x)
#define PATREF(pat) (&(pat))

#endif

struct ListMgr : public testing::Test
{
    WindowPtr window;
    ListHandle list = nullptr;

    ListMgr()
    {
        Rect r = {50,50,250,450};
        window = NewWindow(nullptr, &r, (StringPtr)"", false, 0, (WindowPtr)-1, false, 0);
        SetPort(window);
    }

    ~ListMgr()
    {
        if(list)
            LDispose(list);
        DisposeWindow(window);
    }
};

TEST_F(ListMgr, LNew)
{
    Rect view = {0,0,200,400};
    Rect bounds = {0,0,7,3};
    list = LNew(&view,&bounds, {16,42}, 0, window, false, false, false, false);

    EXPECT_EQ(0, (*list)->rView.top);
    EXPECT_EQ(0, (*list)->rView.left);
    EXPECT_EQ(200, (*list)->rView.bottom);
    EXPECT_EQ(400, (*list)->rView.right);

    EXPECT_EQ(window, (*list)->port);
    EXPECT_EQ(16, (*list)->cellSize.v);
    EXPECT_EQ(42, (*list)->cellSize.h);
    EXPECT_EQ(nullptr, (*list)->vScroll);
    EXPECT_EQ(nullptr, (*list)->hScroll);
    EXPECT_EQ(0, (*list)->selFlags);
    EXPECT_EQ(true, (*list)->lActive);
    EXPECT_EQ(0, (*list)->lReserved);

//    EXPECT_EQ(0, (*list)->clikTime);
 //   EXPECT_EQ(0, (*list)->clikLoc);

    EXPECT_EQ(nullptr, (*list)->userHandle);
    EXPECT_EQ(0, (*list)->dataBounds.top);
    EXPECT_EQ(0, (*list)->dataBounds.left);
    EXPECT_EQ(7, (*list)->dataBounds.bottom);
    EXPECT_EQ(3, (*list)->dataBounds.right);

    Handle cells = (Handle)(*list)->cells;

    EXPECT_NE(nullptr, cells);
    EXPECT_EQ(0, GetHandleSize(cells));
}

TEST_F(ListMgr, flagsDefault)
{
    Rect view = {0,0,200,400};
    Rect bounds = {0,0,7,3};
    list = LNew(&view,&bounds, {16,42}, 0, window, false, false, false, false);
    EXPECT_EQ(0x1C, (*list)->listFlags);
}

TEST_F(ListMgr, flagsDraw)
{
    Rect view = {0,0,200,400};
    Rect bounds = {0,0,7,3};
    list = LNew(&view,&bounds, {16,42}, 0, window, true, false, false, false);
    EXPECT_EQ(0x14, (*list)->listFlags);
}
TEST_F(ListMgr, flagsGrow)
{
    Rect view = {0,0,200,400};
    Rect bounds = {0,0,7,3};
    list = LNew(&view,&bounds, {16,42}, 0, window, false, true, false, false);
    EXPECT_EQ(0x3C, (*list)->listFlags);
}
TEST_F(ListMgr, flagsScrollH)
{
    Rect view = {0,0,200,400};
    Rect bounds = {0,0,7,3};
    list = LNew(&view,&bounds, {16,42}, 0, window, false, false, true, false);
    EXPECT_EQ(0x1D, (*list)->listFlags);
}
TEST_F(ListMgr, flagsScrollV)
{
    Rect view = {0,0,200,400};
    Rect bounds = {0,0,7,3};
    list = LNew(&view,&bounds, {16,42}, 0, window, false, false, false, true);
    EXPECT_EQ(0x1E, (*list)->listFlags);
}


TEST_F(ListMgr, visible)
{
    Rect view = {0,0,200,400};
    Rect bounds = {0,0,7,3};
    list = LNew(&view,&bounds, {16,42}, 0, window, false, false, false, false);
    EXPECT_EQ(0, (*list)->visible.top);
    EXPECT_EQ(0, (*list)->visible.left);
    EXPECT_EQ(13, (*list)->visible.bottom);
    EXPECT_EQ(10, (*list)->visible.right);
}

TEST_F(ListMgr, maxIndex)
{
    Rect view = {0,0,200,400};
    Rect bounds = {0,0,7,3};
    list = LNew(&view,&bounds, {16,42}, 0, window, false, false, false, false);

    EXPECT_EQ(42, (*list)->maxIndex);

    EXPECT_EQ(0x56 + (7*3+1)*2, GetHandleSize((Handle)list));

    for(int i = 0; i < (7*3+1); i++)
        EXPECT_EQ(0, (*list)->cellArray[i]) << " i = " << i;
}

TEST_F(ListMgr, maxIndex2)
{
    Rect view = {0,0,200,400};
    Rect bounds = {0,0,70,40};
    list = LNew(&view,&bounds, {16,42}, 0, window, false, false, false, false);

    EXPECT_EQ(5600, (*list)->maxIndex);
    EXPECT_EQ(0x56 + (70*40+1)*2, GetHandleSize((Handle)list));
}