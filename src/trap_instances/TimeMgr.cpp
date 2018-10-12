#define INSTANTIATE_TRAPS_TimeMgr
#include <TimeMgr.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void TimeMgr() {}
}
}
