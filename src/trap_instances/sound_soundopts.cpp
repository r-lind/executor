#define INSTANTIATE_TRAPS_sound_soundopts
#include <sound/soundopts.h>

// Function for preventing the linker from considering the static constructors in this module unused
namespace Executor {
namespace ReferenceTraps {
    void sound_soundopts() {}
}
}
