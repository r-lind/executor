#if !defined(_SEGMENT_H_)
#define _SEGMENT_H_

extern char ROMlib_exit;

#include <SegmentLdr.h>

namespace Executor
{
struct finderinfo
{
    GUEST_STRUCT;
    GUEST<INTEGER> message;
    GUEST<INTEGER> count;
    GUEST<AppFile[1]> files;
};

extern void ROMlib_seginit(LONGINT argc, char **argv);
extern void empty_timer_queues(void);

static_assert(sizeof(finderinfo) == 268);
}
#endif /* !_SEGMENT_H_ */
