#define INSTANTIATE_TRAPS_wind_wind
#include <wind/wind.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void wind_wind() {}
}
}
