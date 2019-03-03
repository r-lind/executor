#define INSTANTIATE_TRAPS_print_print
#include <print/print.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void print_print() {}
}
}
