#define INSTANTIATE_TRAPS_rsys_emustubs
#include <rsys/emustubs.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void rsys_emustubs() {}
}
}
