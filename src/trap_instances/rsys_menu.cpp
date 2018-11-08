#define INSTANTIATE_TRAPS_rsys_menu
#include <menu/menu.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void rsys_menu() {}
}
}
