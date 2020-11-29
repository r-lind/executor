#include "wayland.h"
#include <vdriver/threaded_vdriver.h>

using DefaultVDriver = Executor::TThreadedVideoDriver<WaylandVideoDriver>;
