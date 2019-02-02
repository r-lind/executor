#define INSTANTIATE_TRAPS_SANE
#include <SANE.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void SANE() {}
}
}
