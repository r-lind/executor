#include "x_keysym.h"
#include <rsys/keyboard.h>

#define LINEFD 0x36

#define R11 0x84
#define R13 0x85
#define R15 0x86
#define R7 0x87
#define R9 0x88

static uint8_t latin_one_table_data[] = {
    MKV_PRINT_SCREEN,
    NOTKEY, /* 1 */
    NOTKEY, /* 2 */
    NOTKEY, /* 3 */
    NOTKEY, /* 4 */
    NOTKEY, /* 5 */
    NOTKEY, /* 6 */
    NOTKEY, /* 7 */
    NOTKEY, /* 8 */
    NOTKEY, /* 9 */
    NOTKEY, /* 10 */
    NOTKEY, /* 11 */
    NOTKEY, /* 12 */
    NOTKEY, /* 13 */
    NOTKEY, /* 14 */
    NOTKEY, /* 15 */
    NOTKEY, /* 16 */
    NOTKEY, /* 17 */
    NOTKEY, /* 18 */
    NOTKEY, /* 19 */
    NOTKEY, /* 20 */
    NOTKEY, /* 21 */
    NOTKEY, /* 22 */
    NOTKEY, /* 23 */
    NOTKEY, /* 24 */
    NOTKEY, /* 25 */
    NOTKEY, /* 26 */
    NOTKEY, /* 27 */
    NOTKEY, /* 28 */
    NOTKEY, /* 29 */
    NOTKEY, /* 30 */
    NOTKEY, /* 31 */
    MKV_SPACE, /* SPACE */
    MKV_1, /* ! */
    MKV_SLASH, /* ? */
    MKV_3, /* # */
    MKV_4, /* $ */
    MKV_5, /* % */
    MKV_7, /* & */
    MKV_TICK, /* ' */
    MKV_9, /* ( */
    MKV_0, /* ) */
    MKV_8, /* * */
    MKV_EQUAL, /* + */
    MKV_COMMA, /* , */
    MKV_MINUS, /* - */
    MKV_PERIOD, /* . */
    MKV_SLASH, /* / */
    MKV_0, /* 0 */
    MKV_1, /* 1 */
    MKV_2, /* 2 */
    MKV_3, /* 3 */
    MKV_4, /* 4 */
    MKV_5, /* 5 */
    MKV_6, /* 6 */
    MKV_7, /* 7 */
    MKV_8, /* 8 */
    MKV_9, /* 9 */
    MKV_SEMI, /* : */
    MKV_SEMI, /* ; */
    MKV_COMMA, /* < */
    MKV_EQUAL, /* = */
    MKV_PERIOD, /* > */
    MKV_SLASH, /* ? */
    MKV_2, /* @ */
    MKV_a, /* A */
    MKV_b, /* B */
    MKV_c, /* C */
    MKV_d, /* D */
    MKV_e, /* E */
    MKV_f, /* F */
    MKV_g, /* G */
    MKV_h, /* H */
    MKV_i, /* I */
    MKV_j, /* J */
    MKV_k, /* K */
    MKV_l, /* L */
    MKV_m, /* M */
    MKV_n, /* N */
    MKV_o, /* O */
    MKV_p, /* P */
    MKV_q, /* Q */
    MKV_r, /* R */
    MKV_s, /* S */
    MKV_t, /* T */
    MKV_u, /* U */
    MKV_v, /* V */
    MKV_w, /* W */
    MKV_x, /* X */
    MKV_y, /* Y */
    MKV_z, /* Z */
    MKV_LEFTBRACKET, /* [ */
    MKV_BACKSLASH, /* \ */
    MKV_RIGHTBRACKET, /* ] */
    MKV_6, /* ^ */
    MKV_MINUS, /* _ */
    MKV_BACKTICK, /* ` */
    MKV_a, /* a */
    MKV_b, /* b */
    MKV_c, /* c */
    MKV_d, /* d */
    MKV_e, /* e */
    MKV_f, /* f */
    MKV_g, /* g */
    MKV_h, /* h */
    MKV_i, /* i */
    MKV_j, /* j */
    MKV_k, /* k */
    MKV_l, /* l */
    MKV_m, /* m */
    MKV_n, /* n */
    MKV_o, /* o */
    MKV_p, /* p */
    MKV_q, /* q */
    MKV_r, /* r */
    MKV_s, /* s */
    MKV_t, /* t */
    MKV_u, /* u */
    MKV_v, /* v */
    MKV_w, /* w */
    MKV_x, /* x */
    MKV_y, /* y */
    MKV_z, /* z */
};

static uint8_t misc_table_data[] = {
    MKV_BACKSPACE, /* 8 back space */
    MKV_TAB, /* 9 tab */
    LINEFD, /* 10 line feed */
    MKV_NUMCLEAR, /* 11 clear */
    NOTKEY, /* 12 */
    MKV_RETURN, /* 13 return */
    NOTKEY, /* 14 */
    NOTKEY, /* 15 */
    NOTKEY, /* 16 */
    NOTKEY, /* 17 */
    NOTKEY, /* 18 */
    MKV_PAUSE, /* 19 pause */
    NOTKEY, /* 20 */
    NOTKEY, /* 21 */
    NOTKEY, /* 22 */
    NOTKEY, /* 23 */
    NOTKEY, /* 24 */
    NOTKEY, /* 25 */
    NOTKEY, /* 26 */
    MKV_ESCAPE, /* 27 escape */
    NOTKEY, /* 28 */
    NOTKEY, /* 29 */
    NOTKEY, /* 30 */
    NOTKEY, /* 31 */
    MKV_RIGHTCNTL, /* 32 multi-key character preface */
    NOTKEY, /* 33 kanji */
    NOTKEY, /* 34 */
    NOTKEY, /* 35 */
    NOTKEY, /* 36 */
    NOTKEY, /* 37 */
    NOTKEY, /* 38 */
    NOTKEY, /* 39 */
    NOTKEY, /* 40 */
    NOTKEY, /* 41 */
    NOTKEY, /* 42 */
    NOTKEY, /* 43 */
    NOTKEY, /* 44 */
    NOTKEY, /* 45 */
    NOTKEY, /* 46 */
    NOTKEY, /* 47 */
    NOTKEY, /* 48 */
    NOTKEY, /* 49 */
    NOTKEY, /* 50 */
    NOTKEY, /* 51 */
    NOTKEY, /* 52 */
    NOTKEY, /* 53 */
    NOTKEY, /* 54 */
    NOTKEY, /* 55 */
    NOTKEY, /* 56 */
    NOTKEY, /* 57 */
    NOTKEY, /* 58 */
    NOTKEY, /* 59 */
    NOTKEY, /* 60 */
    NOTKEY, /* 61 */
    NOTKEY, /* 62 */
    NOTKEY, /* 63 */
    NOTKEY, /* 64 */
    NOTKEY, /* 65 */
    NOTKEY, /* 66 */
    NOTKEY, /* 67 */
    NOTKEY, /* 68 */
    NOTKEY, /* 69 */
    NOTKEY, /* 70 */
    NOTKEY, /* 71 */
    NOTKEY, /* 72 */
    NOTKEY, /* 73 */
    NOTKEY, /* 74 */
    NOTKEY, /* 75 */
    NOTKEY, /* 76 */
    NOTKEY, /* 77 */
    NOTKEY, /* 78 */
    NOTKEY, /* 79 */
    MKV_HOME, /* 80 home */
    MKV_LEFTARROW, /* left arrow */
    MKV_UPARROW, /* up arrow */
    MKV_RIGHTARROW, /* right arrow */
    MKV_DOWNARROW, /* down arrow */
    MKV_PAGEUP, /* prior */
    MKV_PAGEDOWN, /* next */
    MKV_END, /* end */
    NOTKEY, /* 88 begin */
    NOTKEY, /* 89 */
    NOTKEY, /* 90 */
    NOTKEY, /* 91 */
    NOTKEY, /* 92 */
    NOTKEY, /* 93 */
    NOTKEY, /* 94 */
    NOTKEY, /* 95 */
    NOTKEY, /* 96 select */
    NOTKEY, /* print */
    NOTKEY, /* execute */
    MKV_HELP, /* 99 insert/help */
    NOTKEY, /* 100 */
    NOTKEY, /* 101 undo */
    NOTKEY, /* redo */
    NOTKEY, /* menu */
    NOTKEY, /* find */
    NOTKEY, /* cancel */
    MKV_HELP, /* help */
    NOTKEY, /* 107 break */
    NOTKEY, /* 108 */
    NOTKEY, /* 109 */
    NOTKEY, /* 110 */
    NOTKEY, /* 111 */
    NOTKEY, /* 112 */
    NOTKEY, /* 113 */
    NOTKEY, /* 114 */
    NOTKEY, /* 115 */
    NOTKEY, /* 116 */
    NOTKEY, /* 117 */
    NOTKEY, /* 118 */
    NOTKEY, /* 119 */
    NOTKEY, /* 120 */
    NOTKEY, /* 121 */
    NOTKEY, /* 122 */
    NOTKEY, /* 123 */
    NOTKEY, /* 124 */
    MKV_SCROLL_LOCK, /* 125 unassigned (but scroll lock remapped) */
    MKV_LEFTOPTION, /* 126 mode switch (also scroll lock) */
    MKV_NUMCLEAR, /* 127 num lock */
    NOTKEY, /* 128 key space */
    NOTKEY, /* 129 */
    NOTKEY, /* 130 */
    NOTKEY, /* 131 */
    NOTKEY, /* 132 */
    NOTKEY, /* 133 */
    NOTKEY, /* 134 */
    NOTKEY, /* 135 */
    NOTKEY, /* 136 */
    NOTKEY, /* 137 key tab */
    NOTKEY, /* 138 */
    NOTKEY, /* 139 */
    NOTKEY, /* 140 */
    MKV_NUMENTER, /* 141 key enter */
    NOTKEY, /* 142 */
    NOTKEY, /* 143 */
    NOTKEY, /* 144 */
    NOTKEY, /* 145 key f1 */
    NOTKEY, /* key f2 */
    NOTKEY, /* key f3 */
    NOTKEY, /* 148 key f4 */

    MKV_NUM7, /* 149 */
    MKV_NUM4, /* 150 */
    MKV_NUM8, /* 151 */
    MKV_NUM6, /* 152 */
    MKV_NUM2, /* 153 */
    MKV_NUM9, /* 154 */
    MKV_NUM3, /* 155 */
    MKV_NUM1, /* 156 */
    MKV_NUM5, /* 157 */
    MKV_NUM0, /* 158 */
    MKV_NUMPOINT, /* 159 */

    NOTKEY, /* 160 */
    NOTKEY, /* 161 */
    NOTKEY, /* 162 */
    NOTKEY, /* 163 */
    NOTKEY, /* 164 */
    NOTKEY, /* 165 */
    NOTKEY, /* 166 */
    NOTKEY, /* 167 */
    NOTKEY, /* 168 */
    NOTKEY, /* 169 */
    MKV_NUMDIVIDE, /* 170 key multiply */
    MKV_NUMPLUS, /* key plus */
    NOTKEY, /* key comma */
    MKV_NUMMULTIPLY, /* key minus */
    MKV_NUMPOINT, /* key decimal point */
    MKV_NUMEQUAL, /* key divide */
    MKV_NUM0, /* key 0 */
    MKV_NUM1, /* key 1 */
    MKV_NUM2, /* key 2 */
    MKV_NUM3, /* key 3 */
    MKV_NUM4, /* key 4 */
    MKV_NUM5, /* key 5 */
    MKV_NUM6, /* key 6 */
    MKV_NUM7, /* key 7 */
    MKV_NUM8, /* key 8 */
    MKV_NUM9, /* 185 */
    NOTKEY, /* 186 */
    NOTKEY, /* 187 */
    NOTKEY, /* 188 */
    MKV_NUMEQUAL, /* 189 key equals */
    MKV_F1, /* f1 */
    MKV_F2, /* f2 */
    MKV_F3, /* f3 */
    MKV_F4, /* f4 */
    MKV_F5, /* f5 */
    MKV_F6, /* f6 */
    MKV_F7, /* f7 */
    MKV_F8, /* f8 */
    MKV_F9, /* f9 */
    MKV_F10, /* f10 */
    MKV_F11, /* l1 */
    MKV_F12, /* l2 */
    MKV_F13, /* l3 */
    MKV_F14, /* l4 */
    MKV_F15, /* l5 */
    NOTKEY, /* l6 */ /* I don't know what these ones are */
    NOTKEY, /* l7 */
    NOTKEY, /* l8 */
    NOTKEY, /* l9 */
    NOTKEY, /* l10 */
    MKV_HELP, /* r1 */
    MKV_HOME, /* r2 */
    MKV_PAGEUP, /* r3 */
    MKV_DELFORWARD, /* r4 */
    MKV_END, /* r5 */
    MKV_PAGEDOWN, /* r6 */
    R7, /* r7 */
    MKV_UPARROW, /* r8 */
    R9, /* r9 */
    MKV_LEFTARROW, /* r10 */
    R11, /* r11 */
    MKV_RIGHTARROW, /* r12 */
    R13, /* r13 */
    MKV_DOWNARROW, /* r14 */
    R15, /* r15 */
    MKV_LEFTSHIFT, /* left shift */
    MKV_RIGHTSHIFT, /* right shift */
    MKV_LEFTCNTL, /* left control */
    MKV_RIGHTCNTL, /* right control */
    MKV_CAPS, /* caps lock */
    NOTKEY, /* shift lock */
    /* cliff wants left Alt/Meta key to be `clover', the right
     Alt/Meta key to be `alt/option'.  and so it is;

     don't change these unless you also change `COMMONSTATE' in x.c
     to agree with them */
    MKV_CLOVER, /* left meta */
    MKV_RIGHTOPTION, /* right meta */
    MKV_CLOVER, /* left alt */
    MKV_RIGHTOPTION, /* right alt */
    NOTKEY, /* left super */
    NOTKEY, /* right super */
    NOTKEY, /* left hyper */
    NOTKEY, /* 238 right hyper */
    NOTKEY, /* 239 */
    NOTKEY, /* 240 */
    NOTKEY, /* 241 */
    NOTKEY, /* 242 */
    NOTKEY, /* 243 */
    NOTKEY, /* 244 */
    NOTKEY, /* 245 */
    NOTKEY, /* 246 */
    NOTKEY, /* 247 */
    NOTKEY, /* 248 */
    NOTKEY, /* 249 */
    NOTKEY, /* 250 */
    NOTKEY, /* 251 */
    NOTKEY, /* 252 */
    NOTKEY, /* 253 */
    NOTKEY, /* 254 */
    MKV_DELFORWARD, /* 255 delete */
};

key_table_t key_tables[] = {
    /* latin one table */
    { 0, 0, std::size(latin_one_table_data), latin_one_table_data },
    /* misc table */
    { 0xFF, 8, std::size(misc_table_data), misc_table_data },
    { 0, 0, 0, nullptr },
};