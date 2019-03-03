#define INSTANTIATE_TRAPS_base_emustubs
#include <base/emustubs.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void base_emustubs() {}
}
}
