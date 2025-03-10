/* Copyright 1991 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

/* Forward declarations in ScriptMgr.h (DO NOT DELETE THIS LINE) */

#include <base/common.h>
#include <QuickDraw.h>
#include <IntlUtil.h>
#include <ScriptMgr.h>
#include <MemoryMgr.h>
#include <ToolboxUtil.h>
#include <OSUtil.h>

#include <rsys/hook.h>
#include <quickdraw/quick.h>
#include <quickdraw/cquick.h>
#include <rsys/osutil.h>
#include <osevent/osevent.h>
#include <print/print.h>
#include <sane/floatconv.h>
#include <util/string.h>
#include <mman/mman.h>

#include <ctype.h>

using namespace Executor;

/*
 * NOTE: these are stubs to help me make FileMaker Pro go.
 */

LONGINT Executor::C_GetScriptManagerVariable(INTEGER verb)
{
    LONGINT retval;

    switch(verb)
    {
        case smEnabled:
            /* powerpoint seems to require that at least one script is
         present */
            warning_unimplemented("reporting script manager is enabled");
            /* we currently only have a single script */
            retval = 1;
            break;

        case smKCHRCache:
            retval = US_TO_SYN68K(ROMlib_kchr_ptr());
            break;

        default:
            warning_unexpected("unhandled selector `%d'", verb);
            retval = 0;
    }
    return retval;
}

OSErr Executor::C_SetScriptManagerVariable(INTEGER verb, LONGINT param)
{
    ROMlib_hook(script_notsupported);
    return smVerbNotFound;
}

LONGINT Executor::C_GetScriptVariable(INTEGER script, INTEGER verb)
{
    warning_unimplemented("");
    return 0;
}

OSErr Executor::C_SetScriptVariable(INTEGER script, INTEGER verb, LONGINT param)
{
    warning_unimplemented("");
    return smVerbNotFound;
}

INTEGER Executor::C_FontToScript(INTEGER fontnum)
{
    warning_unimplemented("");
    return 0;
}

/*
 * butchered Transliterate provided for Excel 3.0
 */

static char upper(char);
static char lower(char);

#define LOWERTOUPPEROFFSET 'A' - 'a'
static char upper(char ch)
{
    if(ch >= 'a' && ch <= 'z')
#if 1
        return ch + LOWERTOUPPEROFFSET;
#else /* 0 */
        return ch & ~0x20;
#endif /* 0 */
    else
        return ch;
}

#define UPPERTOLOWEROFFSET ('a' - 'A')
static char lower(char ch)
{
    if(ch >= 'A' && ch <= 'Z')
#if 1
        return ch + UPPERTOLOWEROFFSET;
#else /* 0 */
        return ch | 0x20;
#endif /* 0 */
    else
        return ch;
}

INTEGER Executor::C_Transliterate(Handle srch, Handle dsth, INTEGER target,
                                  LONGINT srcmask)
{
    char *sp, *dp, *ep;

    sp = (char *)*srch;
    dp = (char *)*dsth;
    ep = sp + GetHandleSize(srch);
    if(target & smTransLower)
    {
        if(target & smTransUpper)
            /*-->*/ return -1;
        while(sp < ep)
            *dp++ = lower(*sp++);
    }
    else if(target & smTransUpper)
    {
        while(sp < ep)
            *dp++ = upper(*sp++);
    }
    else
    {
        while(sp < ep)
            *dp++ = *sp++;
    }
    return 0;
}

/*
 * NOTE: These are all recent additions, made just before 1.2.2 was frozen.
 *	 They haven't been tested much, if at all.  In addition, much of
 *	 the code below just tries to return something "reasonable", although
 *	 not necessarily correct!
 */

INTEGER Executor::C_FontScript()
{
    warning_unimplemented("");
    return smRoman;
}

INTEGER Executor::C_IntlScript()
{
    warning_unimplemented("");
    return smRoman;
}

void Executor::C_KeyScript(INTEGER scriptcode)
{
    warning_unimplemented("");
}

INTEGER Executor::C_CharType(Ptr textbufp, INTEGER offset)
{
    INTEGER retval;
    unsigned char c;
    ROMlib_hook(script_notsupported);

    retval = 0;
    c = textbufp[offset];
    if(!isalpha(c))
    {
        retval |= smCharPunct;
        if(isspace(c))
            retval |= smPunctBlank;
        else if(isdigit(c))
            retval |= smPunctNumber;
        else if(ispunct(c))
            retval |= smPunctSymbol;
    }
    else
    {
        retval |= smCharAscii;
        if(islower(c))
            retval |= smCharLower;
        else
            retval |= smCharUpper;
    }
    retval |= smCharLeft;
    retval |= smChar1byte;

    return retval;
}

void Executor::C_MeasureJust(Ptr textbufp, int16_t length, int16_t slop,
                             Ptr charlocs)
{
    if(slop)
        warning_unimplemented("slop = %d", slop);
    MeasureText(length, textbufp, charlocs);
}

void Executor::C_MeasureJustified(Ptr text, int32_t length, Fixed slop,
                              Ptr charLocs, JustStyleCode run_pos, Point numer,
                              Point denom)
{
    GUEST<Point> numerx, denomx;

    warning_unimplemented("slop = %d, run_pos = %d", slop, run_pos);

    numerx.v = numer.v;
    numerx.h = numer.h;
    denomx.v = denom.v;
    denomx.h = denom.h;

    xStdTxMeas(length, (uint8_t *)text, &numerx, &denomx,
               nullptr, (GUEST<int16_t> *)charLocs);
}

Boolean Executor::C_ParseTable(CharByteTable table)
{
    memset(table, 0, sizeof(CharByteTable));
    return true;
}

Boolean Executor::C_FillParseTable(CharByteTable table, ScriptCode script)
{
    /* ### should we even look at `script' */
    memset(table, 0, sizeof(CharByteTable));
    return true;
}

INTEGER Executor::C_CharacterByteType(Ptr textBuf, INTEGER textOffset,
                                      ScriptCode script)
{
    warning_unimplemented("");
    /* Single-byte character */
    return 0;
}

INTEGER Executor::C_CharacterType(Ptr textbufp, INTEGER offset,
                                  ScriptCode script)
{
    warning_unimplemented("");
    return CharType(textbufp, offset);
}

INTEGER Executor::C_TransliterateText(Handle srch, Handle dsth, INTEGER target,
                                      LONGINT srcmask, ScriptCode script)
{
    warning_unimplemented("");
    return Transliterate(srch, dsth, target, srcmask);
}

INTEGER Executor::C_Pixel2Char(Ptr textbufp, INTEGER len, INTEGER slop,
                               INTEGER pixwidth, Boolean *leftsidep)
{
    Point num, den;
    INTEGER retval;

    warning_unimplemented("poorly implemented");

    num.h = 1;
    num.v = 1;
    den.h = 1;
    den.v = 1;
    retval = C_PixelToChar(textbufp, len, slop << 16, pixwidth << 16, leftsidep,
                           0, 0, num, den);
    return retval;
}

INTEGER Executor::C_Char2Pixel(Ptr textbufp, INTEGER len, INTEGER slop,
                               INTEGER offset, SignedByte dir)
{
    INTEGER retval;
    Point num, den;

    warning_unimplemented("poorly implemented");

    num.h = 1;
    num.v = 1;
    den.h = 1;
    den.v = 1;

    retval = C_CharToPixel(textbufp, len, slop << 16, offset, dir, 0,
                           num, den);
    return retval;
}

void Executor::C_FindWord(Ptr textbufp, INTEGER length, INTEGER offset,
                          Boolean leftside, Ptr breaks, GUEST<INTEGER> *offsets)
{
    INTEGER start, stop;
    bool chasing_spaces_p;
    ROMlib_hook(script_notsupported);

    if(!leftside)
        --offset;
    if(offset < 0)
        offset = 0;

    chasing_spaces_p = isspace(textbufp[offset]);
    for(start = offset;
        start > 0 && !isspace(textbufp[start - 1]) == !chasing_spaces_p;
        --start)
        ;

    for(stop = offset;
        stop < length && !isspace(textbufp[stop]) == !chasing_spaces_p;
        ++stop)
        ;

    offsets[0] = start;
    offsets[1] = stop;
    offsets[2] = 0; /* Testing on Brute shows we should zero this memory */
    offsets[3] = 0;
    offsets[4] = 0;
    offsets[5] = 0;
    warning_unimplemented("poorly implemented");
}

void Executor::C_HiliteText(Ptr textbufp, INTEGER firstoffset,
                            INTEGER secondoffset, GUEST<INTEGER> *offsets)
{
    ROMlib_hook(script_notsupported);
    offsets[0] = firstoffset;
    offsets[1] = secondoffset;
    offsets[2] = 0;
    offsets[3] = 0;
    offsets[4] = 0;
    offsets[5] = 0;
}

static int16_t
count_spaces(Ptr textbufp, int16_t length)
{
    int16_t retval;

    retval = 0;
    while(length-- > 0)
        if(*textbufp++ == ' ')
            ++retval;

    return retval;
}

void Executor::C_DrawJust(Ptr textbufp, int16_t length, int16_t slop)
{
    GUEST<Fixed> save_sp_extra_x;
    int n_spaces;

    save_sp_extra_x = PORT_SP_EXTRA(qdGlobals().thePort);
    n_spaces = count_spaces(textbufp, length);
    if(n_spaces)
    {
        Fixed extra;

        extra = save_sp_extra_x + FixRatio(slop, n_spaces);
        PORT_SP_EXTRA(qdGlobals().thePort) = extra;
    }
    DrawText(textbufp, 0, length);
    PORT_SP_EXTRA(qdGlobals().thePort) = save_sp_extra_x;
}

static int
snag_date_part(Ptr text, int *offsetp, LONGINT len)
{
    int retval;

    retval = 0;

    while(*offsetp < len && text[*offsetp] != '/')
    {
        retval = retval * 10 + text[*offsetp] - '0';
        ++*offsetp;
    }

    if(*offsetp < len && text[*offsetp] == '/')
        ++*offsetp;

    return retval;
}

enum
{
    longDateFound = 1,
    dateTimeNotFound = 0x8400
};

String2DateStatus Executor::C_StringToTime(
    Ptr textp, LONGINT len, Ptr cachep, GUEST<LONGINT> *lenusedp,
    GUEST<Ptr> *datetimep)
{
    warning_unimplemented("");
    *lenusedp = 0;
    return (String2DateStatus)dateTimeNotFound;
}

static void
this_date_rec(DateTimeRec *p)
{
    GUEST<ULONGINT> now;

    GetDateTime(&now);
    SecondsToDate(now, p);
}

static int
this_century(void)
{
    DateTimeRec d;
    int retval;

    this_date_rec(&d);
    retval = d.year / 100 * 100;
    return retval;
}

static int
this_millennium(void)
{
    int retval;

    retval = this_century() / 1000 * 1000;
    return retval;
}

String2DateStatus Executor::C_StringToDate(
    Ptr text, int32_t length, DateCachePtr cache,
    GUEST<int32_t> *length_used_ret, LongDatePtr date_time)
{
    String2DateStatus retval;

    if(length <= 10 && (text[1] == '/' || text[2] == '/'))
    {
        int offset, month, day, year;
        int offset_save, year_length;

        offset = 0;
        month = snag_date_part(text, &offset, length);
        day = snag_date_part(text, &offset, length);

        offset_save = offset;
        year = snag_date_part(text, &offset, length);
        year_length = offset - offset_save;

        if(year_length == 3)
            year += this_millennium();
        else if(year_length < 3)
            year += this_century();

        *length_used_ret = offset;

        /* not clear what we should do with other fields, some should probably
	 be zeroed */

        date_time->year = year;
        date_time->month = month;
        date_time->day = day;
        retval = longDateFound;
    }
    else
    {
        *length_used_ret = 0;
        warning_unexpected("");
        retval = (String2DateStatus)dateTimeNotFound;
    }
    warning_unimplemented("");
    return retval;
}

StyledLineBreakCode Executor::C_StyledLineBreak(
    Ptr textp, int32_t length, int32_t text_start, int32_t text_end,
    int32_t flags, GUEST<Fixed> *text_width_fp, GUEST<int32_t> *text_offset)
{
    char *text = (char *)textp;
    /* the index into `text' that began the last word, which is where we
     want to break if the current word extends past the end of the
     current line */
    int last_word_break = -1;
    char current_char;
    int current_index;
    int text_width;
    int width = 0;

    /* ### are we losing information here? */
    text_width = Fix2Long(*text_width_fp);

    for(current_index = text_start, current_char = text[current_index];
        current_index < text_end;
        current_index++, current_char = text[current_index])
    {
        /* ### do we do this? */
        if(current_char == '\r')
        {
            *text_offset = current_index + 1;
            return smBreakWord;
        }

        if(current_index > text_start
           && current_char != ' '
           && text[current_index - 1] == ' ')
        {
            last_word_break = current_index - 1;
        }

        width += CharWidth(current_char);

        if(width > text_width)
        {
            /* we got our char */
            if(last_word_break == -1)
            {
                /* d'oh, we are on the first word */
                if(*text_offset)
                {
                    /* beginning of the line, break here */
                    *text_offset = current_index - 1;
                    return smBreakChar;
                }
                *text_offset = current_index - 1;
                return smBreakWord;
            }
            else
            {
                *text_offset = last_word_break;
                return smBreakWord;
            }
        }
    }
    /* if we got here, that means the run did not extend past the end of
     the current line */
    *text_width_fp = Long2Fix(text_width - width);
    *text_offset = current_index;
    return smBreakOverflow;
}

INTEGER Executor::C_ReplaceText(Handle base_text, Handle subst_text, Str15 key)
{
    INTEGER retval;

    warning_unimplemented("not tested much");

    retval = 0;
    HLockGuard guard(subst_text);
    Ptr p;
    INTEGER len;
    LONGINT offset;
    LONGINT l;

    p = (Ptr)*subst_text;
    len = GetHandleSize(subst_text);
    offset = 0;
    while(retval >= 0 && (l = Munger(base_text, offset, (Ptr)key + 1, key[0], nullptr, 1)) >= 0)
    {
        offset = Munger(base_text,
                        l, (Ptr)key + 1, key[0], p, len);
        if(offset < 0)
            retval = offset;
        else
            ++retval;
    }

    return retval;
}

/* StringToExtended is now StringToExtended */
FormatStatus Executor::C_StringToExtended(
    Str255 string, NumFormatStringRec *formatp, NumberParts *partsp,
    Extended80 *xp) /* TTS TODO */
{
    FormatStatus retval;
    double d;
    char buf[256];

    memcpy(buf, string + 1, string[0]);
    buf[string[0]] = 0;
    sscanf(buf, "%lg", &d);
    ieee_to_x80((ieee_t)d, xp);
    warning_unimplemented("");
    retval = noErr;
    return retval;
}

FormatStatus Executor::C_ExtendedToString(
    Extended80 *xp, NumFormatStringRec *formatp, NumberParts *partsp,
    Str255 string) /* TTS TODO */
{
    ieee_t val;
    FormatStatus retval;
    char buf[256];

    val = x80_to_ieee(xp);
#if !defined(CYGWIN32)
    sprintf(buf, "%Lg", val);
#else
    // FIXME: #warning may lose bits of precision here
    sprintf(buf, "%g", (double)val);
#endif
    str255_from_c_string(string, buf);
    warning_unimplemented("");
    retval = noErr;
    return retval;
}

FormatStatus Executor::C_StringToFormatRec(
    ConstStringPtr in_string, NumberParts *partsp,
    NumFormatStringRec *out_string) /* TTS TODO */
{
    FormatStatus retval;

    warning_unimplemented("");
    retval = 0;
    return retval;
}

ToggleResults Executor::C_ToggleDate(LongDateTime *lsecsp, LongDateField field,
                                     DateDelta delta, INTEGER ch,
                                     TogglePB *paramsp) /* TTS TODO */
{
    ToggleResults retval;

    warning_unimplemented("");
    retval = 0;
    return retval;
}

INTEGER Executor::C_TruncString(INTEGER width, Str255 string,
                                TruncCode code) /* TTS TODO */
{
    warning_unimplemented("");

    /* ### claim we didn't have to truncate the string */
    return Truncated;
}

LONGINT Executor::C_VisibleLength(Ptr textp, LONGINT len)
{
    warning_unimplemented("poorly implemented -- what about other white space");

    warning_trace_info("%.*s", (int)len, textp);
    while(len > 0 && textp[len - 1] == ' ')
        --len;
    return len;
}

void Executor::C_LongDateToSeconds(LongDateRec *ldatep, GUEST<ULONGINT> *secs_outp)
{
    long long secs;
    LONGINT high, low;
    INTEGER hour;

    hour = ldatep->hour;
    if(ldatep->pm && hour < 12)
        hour += 12;

    secs = ROMlib_long_long_secs(ldatep->year, ldatep->month,
                                 ldatep->day, hour,
                                 ldatep->minute, ldatep->second);
    high = secs >> 32;
    low = secs;
    secs_outp[0] = high;
    secs_outp[1] = low;
}

void Executor::C_LongSecondsToDate(GUEST<ULONGINT> *secs_inp, LongDateRec *ldatep)

{
    long long secs;
    INTEGER pm;

    secs = ((long long)secs_inp[0] << 32) | secs_inp[1];
    date_to_swapped_fields(secs, &ldatep->year, &ldatep->month, &ldatep->day,
                           &ldatep->hour, &ldatep->minute, &ldatep->second,
                           &ldatep->dayOfWeek, &ldatep->dayOfYear,
                           &ldatep->weekOfYear);

    pm = (ldatep->hour > 12) ? 1 : 0;
    ldatep->pm = pm;
}

/*
 * NOTE: At least some of these need to be implemented if we want
 *	 Resolve to work
 */

#if 0
ParseTable

PortionText

FindScriptRun

IsSpecialFont

RawPrinterValues

NPixel2Char

CharToPixel

DrawJustified

PortionLine

ReplaceText

TruncText

TrunString

NFindWord

ValidDate

StringToExtended

ExtendedToString

FormatRecToString

StringToFormatRec

ToggleDate

LongSecondsToDate

LongDateToSeconds

IntlTokenize

GetFormatOrder
#endif

OSErr Executor::C_InitDateCache(DateCachePtr cache) /* TTS TODO */
{
    warning_unimplemented("");
    return noErr;
}

INTEGER Executor::C_CharByte(Ptr textBuf, INTEGER textOffset)
{
    warning_unimplemented("");
    /* Single-byte character */
    return 0;
}

Fixed Executor::C_PortionLine(Ptr textPtr, LONGINT textLen,
                              JustStyleCode styleRunPosition, Point numer,
                              Point denom)
{
    Fixed retval;

    warning_unimplemented("");
    retval = 0x10000;
    return retval;
}

void Executor::C_DrawJustified(Ptr textPtr, LONGINT textLength, Fixed slop,
                               JustStyleCode styleRunPosition, Point numer,
                               Point denom)
{
    GUEST<Point> swapped_numer;
    GUEST<Point> swapped_denom;

    warning_unimplemented("poorly implemented");
    swapped_numer.h = numer.h;
    swapped_numer.v = numer.v;
    swapped_denom.h = denom.h;
    swapped_denom.v = denom.v;
    text_helper(textLength, textPtr, &swapped_numer, &swapped_denom, 0, 0,
                text_helper_draw);
}

ScriptRunStatus Executor::C_FindScriptRun(
    Ptr textPtr, LONGINT textLen, GUEST<LONGINT> *lenUsedp)
{
    warning_unimplemented("");
    *lenUsedp = 1;
    return 0;
}

INTEGER Executor::C_PixelToChar(Ptr textBuf, LONGINT textLen, Fixed slop,
                                Fixed pixelWidth, Boolean *leadingEdgep,
                                GUEST<Fixed> *widthRemainingp,
                                JustStyleCode styleRunPosition, Point numer,
                                Point denom)
{
    GUEST<INTEGER> *locs;
    INTEGER retval, i;
    GUEST<Point> swapped_numer;
    GUEST<Point> swapped_denom;
    INTEGER int_pix_width;

    warning_unimplemented("poorly implemented");

    locs = (GUEST<INTEGER> *)alloca(sizeof(INTEGER) * (textLen + 1));

    swapped_numer.h = numer.h;
    swapped_numer.v = numer.v;
    swapped_denom.h = denom.h;
    swapped_denom.v = denom.v;

    int_pix_width = pixelWidth >> 16;

    text_helper(textLen, textBuf, &swapped_numer, &swapped_denom, 0, locs,
                text_helper_measure);

    /* NOTE: we could distribute slop here, or we could adjust text_helper
     to account for slop (probably better in the long run).  Right now,
     we do neither.  Ick. */

    if(int_pix_width >= locs[textLen])
    {
        retval = textLen;
        *leadingEdgep = false;
        *widthRemainingp = pixelWidth - (locs[textLen] << 16);
    }
    else
    {
        *widthRemainingp = -1;
        for(i = 0; int_pix_width > locs[i]; ++i)
            ;
        if((i > 0) && ((int_pix_width - locs[i - 1]) > (locs[i] - int_pix_width)))
        {
            retval = i - 1;
            *leadingEdgep = false;
        }
        else
        {
            retval = i;
            *leadingEdgep = true;
        }
    }
    return retval;
}

INTEGER Executor::C_CharToPixel(Ptr textBuf, LONGINT textLen, Fixed slop,
                                LONGINT offset, INTEGER direction,
                                JustStyleCode styleRunPosition, Point numer,
                                Point denom)
{
    INTEGER retval;
    GUEST<Point> swapped_numer, swapped_denom;

    warning_unimplemented("poorly implemented");

    swapped_numer.h = numer.h;
    swapped_numer.v = numer.v;
    swapped_denom.h = denom.h;
    swapped_denom.v = denom.v;
    retval = text_helper(offset, textBuf, &swapped_numer, &swapped_denom,
                         0, 0, text_helper_measure);
    retval += (slop / textLen) >> 16;
    return retval;
}

void Executor::C_LowercaseText(Ptr textp, INTEGER len, ScriptCode script)
{
    warning_unimplemented("poorly implemented");
}

void Executor::C_UppercaseText(Ptr textp, INTEGER len, ScriptCode script)
{
    ROMlib_UprString((StringPtr)textp, false, len);
}

void Executor::C_StripDiacritics(Ptr textp, INTEGER len, ScriptCode script)
{
    warning_unimplemented("poorly implemented");
}

void Executor::C_UppercaseStripDiacritics(
    Ptr textp, INTEGER len, ScriptCode script)
{
    ROMlib_UprString((StringPtr)textp, true, len);
}

void Executor::C_TextUtilFunctions(
    short selector,
    Ptr textp,
    INTEGER len,
    ScriptCode script)
{
    switch(selector)
    {
        case 0x0000:
            C_LowercaseText(textp, len, script);
            break;
        case 0x0200:
            C_StripDiacritics(textp, len, script);
            break;
        case 0x0400:
            C_UppercaseText(textp, len, script);
            break;
        case 0x0600:
            C_UppercaseStripDiacritics(textp, len, script);
            break;
    }
}

