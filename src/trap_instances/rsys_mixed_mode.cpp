#define INSTANTIATE_TRAPS_rsys_mixed_mode
#include <rsys/mixed_mode.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void rsys_mixed_mode() {}
}
}
