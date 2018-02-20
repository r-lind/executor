#include <rsys/cpu.h>
#include <PowerCore.h>

namespace
{
    PowerCore powerCore;
}

PowerCore& Executor::getPowerCore()
{
    return powerCore;
}
