#pragma once

#include <rsys/filesystem.h>
#include <rsys/macstrings.h>
#include <optional>

namespace Executor
{

using CNID = int32_t;

class CNIDMapper
{
public:
    virtual ~CNIDMapper() = default;

    struct Mapping
    {
        CNID parID;
        CNID cnid;
        fs::path path;
        mac_string macname;
    };

    virtual std::vector<Mapping> mapDirectoryContents(CNID dirID, std::vector<fs::path> realPaths) = 0;
    virtual std::optional<Mapping> lookupCNID(CNID cnid) = 0;

    virtual void deleteCNID(CNID cnid) = 0;
    virtual void moveCNID(CNID cnid, CNID newParent, mac_string_view newMacName, std::function<fs::path()> fsop) = 0;
};

}
