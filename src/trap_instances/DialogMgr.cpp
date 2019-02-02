#define INSTANTIATE_TRAPS_DialogMgr
#include <DialogMgr.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void DialogMgr() {}
}
}
