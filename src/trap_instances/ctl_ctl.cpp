#define INSTANTIATE_TRAPS_ctl_ctl
#include <ctl/ctl.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void ctl_ctl() {}
}
}
