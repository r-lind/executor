#pragma once

#include "cnidmapper.h"
#include <map>
#include <unordered_map>

namespace Executor
{

class SimpleCNIDMapper : public CNIDMapper
{
    long nextCNID = 3;
    std::map<fs::path, CNID> pathToId;
    std::unordered_map<CNID, fs::path> idToPath; 
public:
    SimpleCNIDMapper(fs::path root);

    virtual CNID cnidForPath(fs::path path) override;
    virtual std::optional<fs::path> pathForCNID(CNID cnid) override;
    virtual void deleteCNID(CNID cnid) override;
    virtual void moveCNID(CNID cnid, fs::path path) override;
};

}