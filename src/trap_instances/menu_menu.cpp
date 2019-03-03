#define INSTANTIATE_TRAPS_menu_menu
#include <menu/menu.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void menu_menu() {}
}
}
