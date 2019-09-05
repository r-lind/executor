#if !defined(_SCRIPTMGR_H_)
#define _SCRIPTMGR_H_

/*
 * Copyright 1991 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

#include <SANE.h>
#include <IntlUtil.h>
#include <rsys/macros.h>
#include <base/lowglobals.h>

#define MODULE_NAME ScriptMgr
#include <base/api-module.h>

namespace Executor
{
typedef INTEGER String2DateStatus;

enum
{
    smRoman = 0,
};

enum
{
    smCharPunct = 0,
    smCharAscii = 1,
    smCharEuro = 7,
};

enum
{
    smPunctNormal = 0x0000,
    smPunctNumber = 0x0100,
    smPunctSymbol = 0x0200,
    smPunctBlank = 0x0300,
};

enum
{
    smCharLeft = 0x0000,
    smCharRight = 0x2000,
};

enum
{
    smCharLower = 0x0000,
    smCharUpper = 0x4000,
};

enum
{
    smChar1byte = 0x0000,
    smChar2byte = 0x8000,
};

enum
{
    smTransAscii = 0,
    smTransNative = 1,
    smTransLower = 16384,
    smTransUpper = 32768,
    smMaskAscii = 1,
    smMaskNative = 2,
    smMaskAll = -1,
};

enum
{
    smScriptVersion = 0,
    smScriptMunged = 2,
    smScriptEnabled = 4,
    smScriptRight = 6,
    smScriptJust = 8,
    smScriptRedraw = 10,
    smScriptSysFond = 12,
    smScriptAppFond = 14,
    smScriptNumber = 16,
    smScriptDate = 18,
    smScriptSort = 20,
    smScriptRsvd1 = 22,
    smScriptRsvd2 = 24,
    smScriptRsvd3 = 26,
    smScriptRsvd4 = 28,
    smScriptRsvd5 = 30,
    smScriptKeys = 32,
    smScriptIcon = 34,
    smScriptPrint = 36,
    smScriptTrap = 38,
    smScriptCreator = 40,
    smScriptFile = 42,
    smScriptName = 44,
};

enum
{
    smVersion = 0,
    smMunged = 2,
    smEnabled = 4,
    smBiDirect = 6,
    smFontForce = 8,
    smIntlForce = 10,
    smForced = 12,
    smDefault = 14,
    smPrint = 16,
    smSysScript = 18,
    smAppScript = 20,
    smKeyScript = 22,
    smSysRef = 24,
    smKeyCache = 26,
    smKeySwap = 28,
};

enum
{
    smKCHRCache = 38
};

enum
{
    smVerbNotFound = (-1),
};

enum
{
    smBreakWord = 0,
    smBreakChar = 1,
    smBreakOverflow = 2,
};

/* TruncText return codes */
enum
{
    NotTruncated = 0,
    Truncated = 1,
    TruncErr = (-1),
};

typedef uint8_t StyledLineBreakCode;

typedef struct DateCacheRec
{
    GUEST_STRUCT;
    GUEST<int16_t[256]> hidden;
} * DateCachePtr;

typedef struct LongDateRec
{
    GUEST_STRUCT;
    GUEST<int16_t> era;
    GUEST<int16_t> year;
    GUEST<int16_t> month;
    GUEST<int16_t> day;
    GUEST<int16_t> hour;
    GUEST<int16_t> minute;
    GUEST<int16_t> second;
    GUEST<int16_t> dayOfWeek;
    GUEST<int16_t> dayOfYear;
    GUEST<int16_t> weekOfYear;
    GUEST<int16_t> pm;
    GUEST<int16_t> res1;
    GUEST<int16_t> res2;
    GUEST<int16_t> res3;
} * LongDatePtr;

typedef INTEGER TruncCode;
typedef int16_t JustStyleCode;

typedef int8_t CharByteTable[256];

typedef int16_t ScriptRunStatus; /* Not sure this is correct, since in IM
				  ScriptRunStatus is a record with two
				  Signed Bytes */

typedef INTEGER FormatStatus;

struct NumFormatStringRec
{
    GUEST_STRUCT;
    GUEST<Byte> fLength;
    GUEST<Byte> fVersion;
    GUEST<SignedByte[254]> data;
};

typedef uint16_t WideChar;

typedef struct
{
    GUEST_STRUCT;
    GUEST<INTEGER> size;
    GUEST<WideChar> data[10];
} WideCharArr;

struct NumberParts
{
    GUEST_STRUCT;
    GUEST<INTEGER> version;
    GUEST<WideChar[31]> data;
    GUEST<WideCharArr> pePlus;
    GUEST<WideCharArr> peMinus;
    GUEST<WideCharArr> peMinusPlus;
    GUEST<WideCharArr> altNumTable;
    GUEST<char[20]> reserved;
};

typedef extended80 Extended80;

typedef short ToggleResults;

typedef unsigned char LongDateField;

typedef char DateDelta;

struct TogglePB
{
    GUEST_STRUCT;
    GUEST<int32_t> togFlags;
    GUEST<ResType> amChars;
    GUEST<ResType> pmChars;
    GUEST<int32_t[4]> reserved;
};

enum
{
    smSystemScript = -1
};

const LowMemGlobal<INTEGER> TESysJust { 0xBAC }; // ScriptMgr ToolEqu.a (true-b);

DISPATCHER_TRAP(ScriptUtil, 0xA8B5, StackL);

extern LONGINT C_VisibleLength(Ptr textp, LONGINT len);
PASCAL_SUBTRAP(VisibleLength, 0xA8B5, 0x84080028, ScriptUtil);

extern void C_LongDateToSeconds(LongDateRec *ldatep,
                                        GUEST<ULONGINT> *secs_outp);
PASCAL_SUBTRAP(LongDateToSeconds, 0xA8B5, 0x8008FFF2, ScriptUtil);

extern void C_LongSecondsToDate(GUEST<ULONGINT> *secs_inp,
                                        LongDateRec *ldatep);
PASCAL_SUBTRAP(LongSecondsToDate, 0xA8B5, 0x8008FFF0, ScriptUtil);

extern LONGINT C_GetScriptManagerVariable(INTEGER verb);
PASCAL_SUBTRAP(GetScriptManagerVariable, 0xA8B5, 0x84020008, ScriptUtil);

extern OSErr C_SetScriptManagerVariable(INTEGER verb, LONGINT param);
PASCAL_SUBTRAP(SetScriptManagerVariable, 0xA8B5, 0x8206000A, ScriptUtil);

extern LONGINT C_GetScriptVariable(INTEGER script, INTEGER verb);
PASCAL_SUBTRAP(GetScriptVariable, 0xA8B5, 0x8404000C, ScriptUtil);

extern OSErr C_SetScriptVariable(INTEGER script, INTEGER verb,
                                     LONGINT param);
PASCAL_SUBTRAP(SetScriptVariable, 0xA8B5, 0x8208000E, ScriptUtil);
extern INTEGER C_FontToScript(INTEGER fontnum);
PASCAL_SUBTRAP(FontToScript, 0xA8B5, 0x82020006, ScriptUtil);

extern INTEGER C_Transliterate(Handle srch, Handle dsth,
                                           INTEGER target, LONGINT srcmask);
PASCAL_SUBTRAP(Transliterate, 0xA8B5, 0x820E0018, ScriptUtil);
extern INTEGER C_FontScript(void);
PASCAL_SUBTRAP(FontScript, 0xA8B5, 0x82000000, ScriptUtil);

extern INTEGER C_IntlScript(void);
PASCAL_SUBTRAP(IntlScript, 0xA8B5, 0x82000002, ScriptUtil);

extern void C_KeyScript(INTEGER scriptcode);
PASCAL_SUBTRAP(KeyScript, 0xA8B5, 0x80020004, ScriptUtil);

extern INTEGER C_CharType(Ptr textbufp, INTEGER offset);
PASCAL_SUBTRAP(CharType, 0xA8B5, 0x82060012, ScriptUtil);

extern void C_MeasureJust(Ptr textbufp, INTEGER length,
                                      INTEGER slop, Ptr charlocs);
PASCAL_SUBTRAP(MeasureJust, 0xA8B5, 0x800C0020, ScriptUtil);
extern INTEGER C_Pixel2Char(Ptr textbufp, INTEGER len,
                                        INTEGER slop, INTEGER pixwidth, BOOLEAN *leftsidep);
PASCAL_SUBTRAP(Pixel2Char, 0xA8B5, 0x820E0014, ScriptUtil);
extern INTEGER C_Char2Pixel(Ptr textbufp, INTEGER len,
                                        INTEGER slop, INTEGER offset, SignedByte dir);
PASCAL_SUBTRAP(Char2Pixel, 0xA8B5, 0x820C0016, ScriptUtil);
extern void C_FindWord(Ptr textbufp, INTEGER length,
                                   INTEGER offset, BOOLEAN leftside, Ptr breaks, GUEST<INTEGER> *offsets);
PASCAL_SUBTRAP(FindWord, 0xA8B5, 0x8012001A, ScriptUtil);
extern void C_HiliteText(Ptr textbufp, INTEGER firstoffset,
                                     INTEGER secondoffset, GUEST<INTEGER> *offsets);
PASCAL_SUBTRAP(HiliteText, 0xA8B5, 0x800E001C, ScriptUtil);
extern void C_DrawJust(Ptr textbufp, INTEGER length,
                                   INTEGER slop);
PASCAL_SUBTRAP(DrawJust, 0xA8B5, 0x8008001E, ScriptUtil);
extern String2DateStatus C_StringToTime(Ptr textp,
                                                   LONGINT len, Ptr cachep, GUEST<LONGINT> *lenusedp, GUEST<Ptr> *datetimep);
PASCAL_SUBTRAP(StringToTime, 0xA8B5, 0x8214FFF4, ScriptUtil);
extern INTEGER GetMBarHeight(void);
extern INTEGER GetSysDirection(void);
extern void SetSysDirection(INTEGER just);
extern OSErr C_InitDateCache(DateCachePtr theCache);
PASCAL_SUBTRAP(InitDateCache, 0xA8B5, 0x8204FFF8, ScriptUtil);

extern INTEGER C_CharByte(Ptr textBuf, INTEGER textOffset);
PASCAL_SUBTRAP(CharByte, 0xA8B5, 0x82060010, ScriptUtil);

extern String2DateStatus C_StringToDate(Ptr text, int32_t length,
                                                   DateCachePtr cache,
                                                   GUEST<int32_t> *length_used_ret,
                                                   LongDatePtr date_time);
PASCAL_SUBTRAP(StringToDate, 0xA8B5, 0x8214FFF6, ScriptUtil);
extern StyledLineBreakCode C_StyledLineBreak(Ptr textp, int32_t length,
                                                         int32_t text_start, int32_t text_end,
                                                         int32_t flags,
                                                         GUEST<Fixed> *text_width_fp,
                                                         GUEST<int32_t> *text_offset);
PASCAL_SUBTRAP(StyledLineBreak, 0xA8B5, 0x821CFFFE, ScriptUtil);

extern void C_MeasureJustified(Ptr text, int32_t length,
                                       Fixed slop, Ptr charLocs,
                                       JustStyleCode run_pos,
                                       Point numer, Point denom);
PASCAL_SUBTRAP(MeasureJustified, 0xA8B5, 0x801A0034, ScriptUtil);

extern Boolean C_ParseTable(CharByteTable table);
PASCAL_SUBTRAP(ParseTable, 0xA8B5, 0x82040022, ScriptUtil);
extern Boolean C_FillParseTable(CharByteTable table,
                                            ScriptCode script);
PASCAL_SUBTRAP(FillParseTable, 0xA8B5, 0xC2040022, ScriptUtil);

extern INTEGER C_ReplaceText(Handle base_text, Handle subst_text,
                                         Str15 key);
PASCAL_SUBTRAP(ReplaceText, 0xA8B5, 0x820CFFDC, ScriptUtil);
extern INTEGER C_TruncString(INTEGER width, Str255 string,
                                         TruncCode code);
PASCAL_SUBTRAP(TruncString, 0xA8B5, 0x8208FFE0, ScriptUtil);

extern FormatStatus C_StringToExtended(Str255 string, NumFormatStringRec *formatp,
                                                   NumberParts *partsp, Extended80 *xp);
PASCAL_SUBTRAP(StringToExtended, 0xA8B5, 0x8210FFE6, ScriptUtil);

extern FormatStatus C_ExtendedToString(Extended80 *xp,
                                                   NumFormatStringRec *formatp, NumberParts *partsp, Str255 string);
PASCAL_SUBTRAP(ExtendedToString, 0xA8B5, 0x8210FFE8, ScriptUtil);

extern FormatStatus C_StringToFormatRec(ConstStringPtr in_string,
                                                    NumberParts *partsp, NumFormatStringRec *out_string);
PASCAL_SUBTRAP(StringToFormatRec, 0xA8B5, 0x820CFFEC, ScriptUtil);

extern ToggleResults C_ToggleDate(LongDateTime *lsecsp,
                                              LongDateField field, DateDelta delta, INTEGER ch, TogglePB *paramsp);
PASCAL_SUBTRAP(ToggleDate, 0xA8B5, 0x820EFFEE, ScriptUtil);

extern Fixed C_PortionLine(
    Ptr textPtr,
    LONGINT textLen,
    JustStyleCode styleRunPosition,
    Point numer,
    Point denom);
PASCAL_SUBTRAP(PortionLine, 0xA8B5, 0x84120036, ScriptUtil);

extern void C_DrawJustified(
    Ptr textPtr,
    LONGINT textLength,
    Fixed slop,
    JustStyleCode styleRunPosition,
    Point numer,
    Point denom);
PASCAL_SUBTRAP(DrawJustified, 0xA8B5, 0x80160032, ScriptUtil);

extern ScriptRunStatus C_FindScriptRun(
    Ptr textPtr,
    LONGINT textLen,
    GUEST<LONGINT> *lenUsedp);
PASCAL_SUBTRAP(FindScriptRun, 0xA8B5, 0x820C0026, ScriptUtil);

extern INTEGER C_PixelToChar(
    Ptr textBuf,
    LONGINT textLen,
    Fixed slop,
    Fixed pixelWidth,
    BOOLEAN *leadingEdgep,
    GUEST<Fixed> *widthRemainingp,
    JustStyleCode styleRunPosition,
    Point numer,
    Point denom);
PASCAL_SUBTRAP(PixelToChar, 0xA8B5, 0x8222002E, ScriptUtil);

extern INTEGER C_CharToPixel(
    Ptr textBuf,
    LONGINT textLen,
    Fixed slop,
    LONGINT offset,
    INTEGER direction,
    JustStyleCode styleRunPosition,
    Point numer,
    Point denom);
PASCAL_SUBTRAP(CharToPixel, 0xA8B5, 0x821C0030, ScriptUtil);

BEGIN_EXECUTOR_ONLY
extern void C_LowercaseText(
    Ptr textp,
    INTEGER len,
    ScriptCode script);
PASCAL_FUNCTION(LowercaseText); //, 0xA8B5, ScriptUtil);

extern void C_UppercaseText(
    Ptr textp,
    INTEGER len,
    ScriptCode script);
PASCAL_FUNCTION(UppercaseText); //, 0xA8B5, ScriptUtil);

extern void C_StripDiacritics(
    Ptr textp,
    INTEGER len,
    ScriptCode script);
PASCAL_FUNCTION(StripDiacritics); //, 0xA8B5, ScriptUtil);

extern void C_UppercaseStripDiacritics(
    Ptr textp,
    INTEGER len,
    ScriptCode script);
PASCAL_FUNCTION(UppercaseStripDiacritics); //, 0xA8B5, ScriptUtil);

    // the above four functions are actually one entry point on 68K:
extern void C_TextUtilFunctions(
    short selector,
    Ptr textp,
    INTEGER len,
    ScriptCode script);
PASCAL_SUBTRAP(TextUtilFunctions, 0xA8B5, 0x800AFFB6, ScriptUtil);
END_EXECUTOR_ONLY

extern INTEGER C_CharacterByteType(Ptr textBuf, INTEGER textOffset,
                                   ScriptCode script);
PASCAL_SUBTRAP(CharacterByteType, 0xA8B5, 0xC2060010, ScriptUtil);

extern INTEGER C_CharacterType(Ptr textbufp, INTEGER offset,
                               ScriptCode script);
PASCAL_SUBTRAP(CharacterType, 0xA8B5, 0xC2060012, ScriptUtil);

extern INTEGER C_TransliterateText(Handle srch, Handle dsth, INTEGER target,
                                   LONGINT srcmask, ScriptCode script);
PASCAL_SUBTRAP(TransliterateText, 0xA8B5, 0xC20E0018, ScriptUtil);

static_assert(sizeof(DateCacheRec) == 512);
static_assert(sizeof(LongDateRec) == 28);
static_assert(sizeof(NumFormatStringRec) == 256);
static_assert(sizeof(WideChar) == 2);
static_assert(sizeof(WideCharArr) == 22);
static_assert(sizeof(NumberParts) == 172);
static_assert(sizeof(Extended80) == 10);
static_assert(sizeof(ToggleResults) == 2);
static_assert(sizeof(LongDateField) == 1);
static_assert(sizeof(DateDelta) == 1);
static_assert(sizeof(TogglePB) == 28);
}

#endif /* _SCRIPTMGR_H_ */
