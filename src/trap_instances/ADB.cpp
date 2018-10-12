#define INSTANTIATE_TRAPS_ADB
#include <ADB.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void ADB() {}
}
}
