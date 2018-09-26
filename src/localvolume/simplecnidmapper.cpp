#include "simplecnidmapper.h"

using namespace Executor;

SimpleCNIDMapper::SimpleCNIDMapper(fs::path root)
{
    pathToId[root] = 2;
    idToPath[2] = root;
}

CNID SimpleCNIDMapper::cnidForPath(fs::path path)
{
    CNID cnid;
    auto idIt = pathToId.find(path);
    if(idIt != pathToId.end())
        cnid = idIt->second;
    else
    {
        cnid = nextCNID++;
        pathToId.emplace(path, cnid);
        idToPath.emplace(cnid, path);
    }

    return cnid;
}

std::optional<fs::path> SimpleCNIDMapper::pathForCNID(CNID cnid)
{
    auto it = idToPath.find(cnid);
    if(it == idToPath.end())
        return {};
    else
        return it->second;
}

void SimpleCNIDMapper::deleteCNID(CNID cnid)
{
    auto it = idToPath.find(cnid);
    if(it == idToPath.end())
        return;
    
    pathToId.erase(it->second);
    idToPath.erase(it);
}

void SimpleCNIDMapper::moveCNID(CNID cnid, fs::path path)
{
    auto it = idToPath.find(cnid);
    if(it == idToPath.end())
        return;
    
    pathToId.erase(it->second);
    pathToId.emplace(path, cnid);
    it->second = path;
}

