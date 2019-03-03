#define INSTANTIATE_TRAPS_textedit_tesave
#include <textedit/tesave.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void textedit_tesave() {}
}
}
