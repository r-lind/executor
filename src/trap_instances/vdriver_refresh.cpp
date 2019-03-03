#define INSTANTIATE_TRAPS_vdriver_refresh
#include <vdriver/refresh.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void vdriver_refresh() {}
}
}
