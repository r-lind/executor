#if !defined(_SRCBLT_H_)
#define _SRCBLT_H_

#include <QuickDraw.h>
#include <quickdraw/cquick.h>

extern "C" {

#if !defined(ARCH_PROVIDES_RAW_SRCBLT)
#define USE_PORTABLE_SRCBLT
#endif

extern void srcblt_rgn(Executor::RgnHandle rh, int mode, int log2_bpp,
                       const Executor::blt_bitmap_t *src, const Executor::blt_bitmap_t *dst,
                       Executor::GUEST<Executor::Point> *src_origin, Executor::GUEST<Executor::Point> *dst_origin,
                       uint32_t fg_color, uint32_t bk_color) asm("_srcblt_rgn");

extern void srcblt_bitmap(void)
#if !defined(USE_PORTABLE_SRCBLT)
    asm("_srcblt_bitmap")
#endif
        ;

extern char srcblt_nop asm("_srcblt_nop");

#define SRCBLT_ARRAY

extern const void **srcblt_noshift_stubs SRCBLT_ARRAY[8];
extern const void **srcblt_noshift_fgbk_stubs SRCBLT_ARRAY[8];
#if defined(USE_PORTABLE_SRCBLT)
extern const void **srcblt_shift_stubs[8];
extern const void **srcblt_shift_fgbk_stubs[8];
#elif defined(i386)
extern const void **srcblt_shift_i486_stubs SRCBLT_ARRAY[8];
extern const void **srcblt_shift_i386_stubs SRCBLT_ARRAY[8];
extern const void **srcblt_shift_fgbk_i486_stubs SRCBLT_ARRAY[8];
extern const void **srcblt_shift_fgbk_i386_stubs SRCBLT_ARRAY[8];
#else
#error "unsupported architecture-specific blitter target"
#endif

#undef SRCBLT_ARRAY

extern int srcblt_log2_bpp asm("_srcblt_log2_bpp");

extern const Executor::INTEGER *srcblt_rgn_start asm("_srcblt_rgn_start");

extern const void **srcblt_stub_table asm("_srcblt_stub_table");

extern int32_t srcblt_x_offset asm("_srcblt_x_offset");

extern int32_t srcblt_src_row_bytes asm("_srcblt_src_row_bytes");
extern int32_t srcblt_dst_row_bytes asm("_srcblt_dst_row_bytes");

extern uint32_t srcblt_fg_color asm("_srcblt_fg_color");
extern uint32_t srcblt_bk_color asm("_srcblt_bk_color");

extern char *srcblt_src_baseaddr asm("_srcblt_src_baseaddr");
extern char *srcblt_dst_baseaddr asm("_srcblt_dst_baseaddr");

extern int srcblt_shift_offset asm("_srcblt_shift_offset");

extern bool srcblt_reverse_scanlines_p asm("_srcblt_reverse_scanlines_p");
}
#endif /* !_SRCBLT_H_ */
