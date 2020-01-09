#include "gtest/gtest.h"

#include "compat.h"

#ifdef EXECUTOR
#include <ToolboxEvent.h>
#include <OSEvent.h>
#else
#include <Events.h>
#endif

TEST(Events, FlushThenGet)
{
    FlushEvents(everyEvent, 0);

    EventRecord e;
    bool gotEvent = GetOSEvent(everyEvent, &e);
    long ticks = TickCount();

    EXPECT_FALSE(gotEvent);
    EXPECT_EQ(nullEvent, e.what);
    EXPECT_LE(e.when, ticks);
    EXPECT_GT(e.when + 5, ticks);
}


TEST(Events, PostKey)
{
    FlushEvents(everyEvent, 0);

    PostEvent(keyDown, 0 | 'a');

    EventRecord e;
    bool gotEvent = GetOSEvent(everyEvent, &e);
    long ticks = TickCount();

    EXPECT_TRUE(gotEvent);
    EXPECT_EQ(keyDown, e.what);
    EXPECT_LE(e.when, ticks);
    EXPECT_GT(e.when + 5, ticks);
    EXPECT_EQ(0 | 'a', e.message);
    EXPECT_EQ(btnState, e.modifiers & 0xFF80);

    EXPECT_EQ(0, ((uint8_t*)0x174)[0]);
    PostEvent(keyUp, 0 | 'a');


    gotEvent = GetOSEvent(everyEvent, &e);
    ticks = TickCount();

    EXPECT_FALSE(gotEvent);
    EXPECT_EQ(nullEvent, e.what);
    EXPECT_LE(e.when, ticks);
    EXPECT_GT(e.when + 5, ticks);
}



TEST(Events, PostKeyMod)
{
    FlushEvents(everyEvent, 0);

        // set command key in KeyMap
    ((uint8_t*)0x174)[6] |= 0x80;

    PostEvent(keyDown, 0 | 'a');

        // clear command key in KeyMap
    ((uint8_t*)0x174)[6] &= ~0x80;

    EventRecord e;
    bool gotEvent = GetOSEvent(everyEvent, &e);
    long ticks = TickCount();

    EXPECT_TRUE(gotEvent);
    EXPECT_EQ(keyDown, e.what);
    EXPECT_LE(e.when, ticks);
    EXPECT_GT(e.when + 5, ticks);
    EXPECT_EQ(0 | 'a', e.message);
    EXPECT_EQ(btnState | cmdKey, e.modifiers & 0xFF80);

    EXPECT_EQ(0, ((uint8_t*)0x174)[0] & 0x80);

    gotEvent = GetOSEvent(everyEvent, &e);
    ticks = TickCount();

    EXPECT_FALSE(gotEvent);
    EXPECT_EQ(nullEvent, e.what);
    EXPECT_LE(e.when, ticks);
    EXPECT_GT(e.when + 5, ticks);
}

TEST(Events, Button)
{
    EXPECT_FALSE(Button());
    auto saveButton = *(uint8_t*)0x172;
    EXPECT_NE(0, saveButton);

    *(uint8_t*)0x172 = 0;
    EXPECT_TRUE(Button());

    *(uint8_t*)0x172 = saveButton;
    EXPECT_FALSE(Button());
}


TEST(Events, ButtonMod)
{
    FlushEvents(everyEvent, 0);

    EXPECT_FALSE(Button());
    auto saveButton = *(uint8_t*)0x172;
    EXPECT_NE(0, saveButton);

    *(uint8_t*)0x172 = 0;
    EXPECT_TRUE(Button());

    PostEvent(keyDown, 0 | 'a');

    EventRecord e;
    bool gotEvent = GetOSEvent(everyEvent, &e);
    long ticks = TickCount();

    EXPECT_TRUE(gotEvent);
    EXPECT_EQ(keyDown, e.what);
    EXPECT_LE(e.when, ticks);
    EXPECT_GT(e.when + 5, ticks);
    EXPECT_EQ(0 | 'a', e.message);
    EXPECT_EQ(0, e.modifiers & 0xFF80);

    *(uint8_t*)0x172 = saveButton;
    EXPECT_FALSE(Button());
}