#define INSTANTIATE_TRAPS_WindowMgr
#include <WindowMgr.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void WindowMgr() {}
}
}
