#if !defined(_DIRTYRECT_H_)
#define _DIRTYRECT_H_
namespace Executor
{
extern void dirty_rect_accrue(int top, int left, int bottom, int right);
extern void dirty_rect_update_screen(void);
}

#endif /* !_DIRTYRECT_H_ */
