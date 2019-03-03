#define INSTANTIATE_TRAPS_list_list
#include <list/list.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void list_list() {}
}
}
