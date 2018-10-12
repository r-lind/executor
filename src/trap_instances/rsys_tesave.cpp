#define INSTANTIATE_TRAPS_rsys_tesave
#include <rsys/tesave.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void rsys_tesave() {}
}
}
