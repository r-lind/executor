#if !defined(_TEXTEDIT_H_)
#define _TEXTEDIT_H_

/*
 * Copyright 1986, 1989, 1990, 1995 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

#include <QuickDraw.h>

#define MODULE_NAME TextEdit
#include <base/api-module.h>

namespace Executor
{
/* new justification defines, accepted by `TESetAlignment ()' and
   `TETextBox ()' */
enum
{
    teFlushDefault = 0,
    teCenter = 1,
    teFlushRight = (-1),
    teFlushLeft = (-2),
};

/* older justification defines */
enum
{
    teJustLeft = 0,
    teJustCenter = 1,
    teJustRight = (-1),
    teForceLeft = (-2),
};

enum
{
    doFont = 1,
    doFace = 2,
    doSize = 4,
    doColor = 8,
    doAll = 15,
    addSize = 16,
    doToggle = 32,
};

enum
{
    teFind = 0,
    teHilite = 1,
    teDraw = -1,
    teCaret = -2,
};

enum
{
    caret_vis = (-1),
    caret_invis = (255),
    hilite_vis = (0),
};

enum
{
    teFAutoScroll = (0),
    teFTextBuffering = (1),
    teFOutlineHilite = (2),
    teFInlineInput = (3),
    teFUseTextServices = (4),
};

enum
{
    teBitClear = (0),
    teBitSet = (1),
    teBitTest = (-1),
};

struct TERec
{
    GUEST_STRUCT;
    GUEST<Rect> destRect;
    GUEST<Rect> viewRect;
    GUEST<Rect> selRect;
    GUEST<INTEGER> lineHeight;
    GUEST<INTEGER> fontAscent;
    GUEST<Point> selPoint;
    GUEST<INTEGER> selStart;
    GUEST<INTEGER> selEnd;
    GUEST<INTEGER> active;
    GUEST<ProcPtr> wordBreak;
    GUEST<ProcPtr> clikLoop;
    GUEST<LONGINT> clickTime;
    GUEST<INTEGER> clickLoc;
    GUEST<LONGINT> caretTime;
    GUEST<INTEGER> caretState;
    GUEST<INTEGER> just;
    GUEST<INTEGER> teLength;
    GUEST<Handle> hText;
    GUEST<INTEGER> recalBack;
    GUEST<INTEGER> recalLines;
    GUEST<INTEGER> clikStuff;
    GUEST<INTEGER> crOnly;
    GUEST<INTEGER> txFont;
    GUEST<Style> txFace;
    GUEST<Byte> filler;
    GUEST<INTEGER> txMode;
    GUEST<INTEGER> txSize;
    GUEST<GrafPtr> inPort;
    GUEST<ProcPtr> highHook;
    GUEST<ProcPtr> caretHook;
    GUEST<INTEGER> nLines;
    GUEST<INTEGER[1]> lineStarts;
};

typedef TERec *TEPtr;

typedef GUEST<TEPtr> *TEHandle;

struct StyleRun
{
    GUEST_STRUCT;
    GUEST<INTEGER> startChar;
    GUEST<INTEGER> styleIndex;
};

struct STElement
{
    GUEST_STRUCT;
    GUEST<INTEGER> stCount;
    GUEST<INTEGER> stHeight;
    GUEST<INTEGER> stAscent;
    GUEST<INTEGER> stFont;
    GUEST<Style> stFace;
    GUEST<Byte> filler;
    GUEST<INTEGER> stSize;
    GUEST<RGBColor> stColor;
};

typedef STElement TEStyleTable[1];
typedef STElement *STPtr;

typedef GUEST<STPtr> *STHandle;

struct LHElement
{
    GUEST_STRUCT;
    GUEST<INTEGER> lhHeight;
    GUEST<INTEGER> lhAscent;
};

typedef LHElement LHTable[1];
typedef LHElement *LHPtr;

typedef GUEST<LHPtr> *LHHandle;

struct TextStyle
{
    GUEST_STRUCT;
    GUEST<INTEGER> tsFont;
    GUEST<Style> tsFace;
    GUEST<Byte> filler;
    GUEST<INTEGER> tsSize;
    GUEST<RGBColor> tsColor;
};

struct ScrpSTElement
{
    GUEST_STRUCT;
    GUEST<LONGINT> scrpStartChar;
    GUEST<INTEGER> scrpHeight;
    GUEST<INTEGER> scrpAscent;
    GUEST<INTEGER> scrpFont;
    GUEST<Style> scrpFace;
    GUEST<Byte> filler;
    GUEST<INTEGER> scrpSize;
    GUEST<RGBColor> scrpColor;
};

typedef ScrpSTElement ScrpSTTable[1];

struct StScrpRec
{
    GUEST_STRUCT;
    GUEST<INTEGER> scrpNStyles;
    GUEST<ScrpSTTable> scrpStyleTab;
};

typedef StScrpRec *StScrpPtr;

typedef GUEST<StScrpPtr> *StScrpHandle;

struct NullSTRec
{
    GUEST_STRUCT;
    GUEST<LONGINT> TEReserved;
    GUEST<StScrpHandle> nullScrap;
};

typedef NullSTRec *NullSTPtr;

typedef GUEST<NullSTPtr> *NullSTHandle;

struct TEStyleRec
{
    GUEST_STRUCT;
    GUEST<INTEGER> nRuns;
    GUEST<INTEGER> nStyles;
    GUEST<STHandle> styleTab;
    GUEST<LHHandle> lhTab;
    GUEST<LONGINT> teRefCon;
    GUEST<NullSTHandle> nullStyle;
    GUEST<StyleRun[1]> runs;
};

typedef TEStyleRec *TEStylePtr;

typedef GUEST<TEStylePtr> *TEStyleHandle;

typedef Byte Chars[1], *CharsPtr, **CharsHandle;

const LowMemGlobal<ProcPtr> TEDoText{ 0xA70 }; // TextEdit IMI-391 (true);
const LowMemGlobal<ProcPtr> TERecal{ 0xA74 }; // TextEdit IMI-391 (false);
const LowMemGlobal<INTEGER> TEScrpLength{ 0xAB0 }; // TextEdit IMI-389 (true);
const LowMemGlobal<Handle> TEScrpHandle{ 0xAB4 }; // TextEdit IMI-389 (true);

DISPATCHER_TRAP(TEDispatch, 0xA83D, StackW);

extern void C_TESetText(Ptr p, LONGINT ln, TEHandle teh);
PASCAL_TRAP(TESetText, 0xA9CF);

extern CharsHandle C_TEGetText(TEHandle teh);
PASCAL_TRAP(TEGetText, 0xA9CB);

extern void C_TESetAlignment(INTEGER j, TEHandle teh);
PASCAL_TRAP(TESetAlignment, 0xA9DF);

extern void C_TEUpdate(const Rect *r, TEHandle teh);
PASCAL_TRAP(TEUpdate, 0xA9D3);

extern void C_TETextBox(Ptr p, LONGINT ln, const Rect *r,
                        INTEGER j);
PASCAL_TRAP(TETextBox, 0xA9CE);
extern void C_TEScroll(INTEGER dh, INTEGER dv, TEHandle teh);
PASCAL_TRAP(TEScroll, 0xA9DD);

extern void C_TEKey(CharParameter thec, TEHandle teh);
PASCAL_TRAP(TEKey, 0xA9DC);

extern void C_TECopy(TEHandle teh);
PASCAL_TRAP(TECopy, 0xA9D5);

extern void C_TECut(TEHandle teh);
PASCAL_TRAP(TECut, 0xA9D6);

extern void C_TEPaste(TEHandle teh);
PASCAL_TRAP(TEPaste, 0xA9DB);

extern void C_TEDelete(TEHandle teh);
PASCAL_TRAP(TEDelete, 0xA9D7);

extern void C_TEInsert(Ptr p, LONGINT ln, TEHandle teh);
PASCAL_TRAP(TEInsert, 0xA9DE);

extern void C_TEPinScroll(INTEGER dh,
                          INTEGER dv, TEHandle teh);
PASCAL_TRAP(TEPinScroll, 0xA812);
extern void ROMlib_teautoloop(TEHandle teh);
extern void C_TESelView(TEHandle teh);
PASCAL_TRAP(TESelView, 0xA811);

extern void C_TEAutoView(Boolean autoflag,
                         TEHandle teh);
PASCAL_TRAP(TEAutoView, 0xA813);
extern TEHandle C_TEStyleNew(const Rect *dst, const Rect *view);
PASCAL_TRAP(TEStyleNew, 0xA83E);

extern void C_TESetStyleHandle(TEStyleHandle theHandle,
                               TEHandle teh);
PASCAL_SUBTRAP(TESetStyleHandle, 0xA83D, 0x0005, TEDispatch);
extern TEStyleHandle C_TEGetStyleHandle(TEHandle teh);
PASCAL_SUBTRAP(TEGetStyleHandle, 0xA83D, 0x0004, TEDispatch);

extern StScrpHandle C_TEGetStyleScrapHandle(TEHandle teh);
PASCAL_SUBTRAP(TEGetStyleScrapHandle, 0xA83D, 0x0006, TEDispatch);

extern void C_TEStyleInsert(Ptr text, LONGINT length,
                            StScrpHandle hST, TEHandle teh);
PASCAL_SUBTRAP(TEStyleInsert, 0xA83D, 0x0007, TEDispatch);
extern INTEGER C_TEGetOffset(Point pt, TEHandle teh);
PASCAL_TRAP(TEGetOffset, 0xA83C);

extern LONGINT C_TEGetPoint(INTEGER offset, TEHandle teh);
PASCAL_SUBTRAP(TEGetPoint, 0xA83D, 0x0008, TEDispatch);

extern int32_t C_TEGetHeight(LONGINT endLine, LONGINT startLine, TEHandle teh);
PASCAL_SUBTRAP(TEGetHeight, 0xA83D, 0x0009, TEDispatch);
extern void C_TEGetStyle(INTEGER offset,
                         TextStyle *theStyle, GUEST<INTEGER> *lineHeight, GUEST<INTEGER> *fontAscent,
                         TEHandle teh);
PASCAL_SUBTRAP(TEGetStyle, 0xA83D, 0x0003, TEDispatch);
extern void C_TEStylePaste(TEHandle teh);
PASCAL_SUBTRAP(TEStylePaste, 0xA83D, 0x0000, TEDispatch);

extern void C_TESetStyle(INTEGER mode, TextStyle *newStyle,
                         Boolean redraw, TEHandle teh);
PASCAL_SUBTRAP(TESetStyle, 0xA83D, 0x0001, TEDispatch);
extern void C_TEReplaceStyle(INTEGER mode,
                             TextStyle *oldStyle, TextStyle *newStyle, Boolean redraw, TEHandle teh);
PASCAL_SUBTRAP(TEReplaceStyle, 0xA83D, 0x0002, TEDispatch);
extern Boolean C_TEContinuousStyle(GUEST<INTEGER> *modep,
                                   TextStyle *thestyle, TEHandle teh);
PASCAL_SUBTRAP(TEContinuousStyle, 0xA83D, 0x000A, TEDispatch);
extern void C_TEUseStyleScrap(LONGINT start, LONGINT stop,
                              StScrpHandle newstyles, Boolean redraw, TEHandle teh);
PASCAL_SUBTRAP(TEUseStyleScrap, 0xA83D, 0x000B, TEDispatch);
extern void C_TECustomHook(INTEGER sel, GUEST<ProcPtr> *addr,
                           TEHandle teh);
PASCAL_SUBTRAP(TECustomHook, 0xA83D, 0x000C, TEDispatch);
extern LONGINT C_TENumStyles(LONGINT start, LONGINT stop,
                             TEHandle teh);
PASCAL_SUBTRAP(TENumStyles, 0xA83D, 0x000D, TEDispatch);
extern void C_TEInit(void);
PASCAL_TRAP(TEInit, 0xA9CC);

extern TEHandle C_TENew(const Rect *dst, const Rect *view);
PASCAL_TRAP(TENew, 0xA9D2);

extern void C_TEDispose(TEHandle teh);
PASCAL_TRAP(TEDispose, 0xA9CD);

extern void C_TEIdle(TEHandle teh);
PASCAL_TRAP(TEIdle, 0xA9DA);

extern void C_TEClick(Point p, Boolean ext, TEHandle teh);
PASCAL_TRAP(TEClick, 0xA9D4);

extern void C_TESetSelect(LONGINT start, LONGINT stop,
                          TEHandle teh);
PASCAL_TRAP(TESetSelect, 0xA9D1);
extern void C_TEActivate(TEHandle teh);
PASCAL_TRAP(TEActivate, 0xA9D8);

extern void C_TEDeactivate(TEHandle teh);
PASCAL_TRAP(TEDeactivate, 0xA9D9);

extern void TESetWordBreak(ProcPtr wb, TEHandle teh);
extern void TESetClickLoop(ProcPtr cp, TEHandle teh);
extern void C_TECalText(TEHandle teh);
PASCAL_TRAP(TECalText, 0xA9D0);

extern OSErr TEFromScrap(void);
extern OSErr TEToScrap(void);
extern Handle TEScrapHandle(void);
extern LONGINT TEGetScrapLength(void);
extern void TESetScrapLength(LONGINT ln);
extern int16_t C_TEFeatureFlag(int16_t feature, int16_t action,
                               TEHandle te);
PASCAL_SUBTRAP(TEFeatureFlag, 0xA83D, 0x000E, TEDispatch);

static_assert(sizeof(TERec) == 98);
static_assert(sizeof(StyleRun) == 4);
static_assert(sizeof(STElement) == 18);
static_assert(sizeof(LHElement) == 4);
static_assert(sizeof(TextStyle) == 12);
static_assert(sizeof(ScrpSTElement) == 20);
static_assert(sizeof(StScrpRec) == 22);
static_assert(sizeof(NullSTRec) == 8);
static_assert(sizeof(TEStyleRec) == 24);
}
#endif /* _TEXTEDIT_H_ */
