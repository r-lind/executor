#pragma once

#define RAMBASEDBIT (1 << 6)
#define DRIVEROPENBIT (1 << 5)

namespace Executor
{
struct driverinfo
{
    DriverUPP open;
    DriverUPP prime;
    DriverUPP ctl;
    DriverUPP status;
    DriverUPP close;
    StringPtr name;
    INTEGER refnum;
};

void RegisterDriver(const driverinfo& di);
}
