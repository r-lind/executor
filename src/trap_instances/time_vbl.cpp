#define INSTANTIATE_TRAPS_time_vbl
#include <time/vbl.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void time_vbl() {}
}
}
