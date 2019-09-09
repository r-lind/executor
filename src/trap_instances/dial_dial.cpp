#define INSTANTIATE_TRAPS_dial_dial
#include <dial/dial.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void dial_dial() {}
}
}
