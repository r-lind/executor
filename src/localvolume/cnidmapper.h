#pragma once

#include <rsys/filesystem.h>
#include <optional>

namespace Executor
{

using CNID = long;

class CNIDMapper
{
public:
    virtual CNID cnidForPath(fs::path path) = 0;
    virtual std::optional<fs::path> pathForCNID(CNID cnid) = 0;
    virtual void deleteCNID(CNID cnid) = 0;
    virtual void moveCNID(CNID cnid, fs::path path) = 0;
};

}