#define INSTANTIATE_TRAPS_rsys_gestalt
#include <rsys/gestalt.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void rsys_gestalt() {}
}
}
