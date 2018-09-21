#define INSTANTIATE_TRAPS_rsys_cfm
#include <rsys/cfm.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void rsys_cfm() {}
}
}
