#include "simplecnidmapper.h"
#include <set>
#include <OSUtil.h>

using namespace Executor;

SimpleCNIDMapper::SimpleCNIDMapper(fs::path root, mac_string volumeName)
{
    mappings_[2] = {
        1, 2,
        fs::directory_entry(root),
        volumeName
    };
}

std::vector<CNIDMapper::Mapping> SimpleCNIDMapper::mapDirectoryContents(CNID dirID,
        std::vector<fs::directory_entry> realPaths)
{
    auto& cachedContents = directories_[dirID];
    
    std::sort(realPaths.begin(), realPaths.end());

    std::vector<Mapping> dirMappings;
    dirMappings.reserve(realPaths.size());

    std::vector<CNID> newContents;
    newContents.reserve(realPaths.size());

    auto cacheIt = cachedContents.begin(), cacheEnd = cachedContents.end();
    Mapping* mapping = cacheIt != cacheEnd ? &mappings_.at(*cacheIt) : nullptr;
    
    std::set<mac_string> usedNames;

    for(auto& entry : realPaths)
    {
        while(mapping && mapping->entry.path() < entry.path())
        {
            // TODO: discard mapping
            ++cacheIt;
            mapping = cacheIt != cacheEnd ? &mappings_.at(*cacheIt) : nullptr;
        }

        if(mapping && mapping->entry.path() == entry.path())
        {
            // use mapping
            dirMappings.push_back(*mapping);
            newContents.push_back(mapping->cnid);
        }
        else
        {
            mac_string macname;
            const fs::path& name = entry.path();
            int index = 0;

            bool nameIsFree;
            do
            {
                macname = toMacRomanFilename(name, index++);
                mac_string nameUpr = macname;
                ROMlib_UprString(nameUpr.data(), false, nameUpr.size());
                nameIsFree = usedNames.emplace(std::move(nameUpr)).second;
            } while(!nameIsFree);

            Mapping newMapping {
                dirID, nextCNID_++,
                std::move(entry),
                std::move(macname)
            };
            dirMappings.push_back(newMapping);
            newContents.push_back(newMapping.cnid);
            mappings_.emplace(newMapping.cnid, std::move(newMapping));
        }
    }

    cachedContents = std::move(newContents);

    return dirMappings;
}

std::optional<CNIDMapper::Mapping> SimpleCNIDMapper::lookupCNID(CNID cnid)
{
    auto it = mappings_.find(cnid);
    if(it == mappings_.end())
        return {};
    return it->second;
}


void SimpleCNIDMapper::deleteCNID(CNID cnid)
{
    auto it = mappings_.find(cnid);
    mappings_.find(cnid);
    if(it == mappings_.end())
        return;
    CNID parID = it->second.parID;

    auto& cachedContents = directories_[parID];

    cachedContents.erase(
        std::remove(cachedContents.begin(), cachedContents.end(), cnid),
        cachedContents.end());

    // TODO: clean up (including subdirectories)
}

void SimpleCNIDMapper::moveCNID(CNID cnid, CNID newParent, std::function<fs::path()> fsop)
{
    auto it = mappings_.find(cnid);
    mappings_.find(cnid);
    if(it == mappings_.end())
        return;

    CNID oldParent = it->second.parID;
    if(oldParent == newParent)
        return;

    fs::path newPath = fsop();

    it->second.entry = fs::directory_entry(newPath);
    it->second.parID = newParent;

    auto& cachedContentsFrom = directories_[oldParent];
    cachedContentsFrom.erase(
        std::remove(cachedContentsFrom.begin(), cachedContentsFrom.end(), cnid),
        cachedContentsFrom.end());

    auto& cachedContentsTo = directories_[newParent];
    cachedContentsTo.insert(
        std::upper_bound(cachedContentsTo.begin(), cachedContentsTo.end(), cnid,
            [&](CNID a, CNID b) {
                return mappings_.at(a).entry.path() < mappings_.at(b).entry.path();
            }),
        cnid);
}

void SimpleCNIDMapper::renameCNID(CNID cnid, mac_string_view newMacName, std::function<fs::path()> fsop)
{
    auto it = mappings_.find(cnid);
    mappings_.find(cnid);
    if(it == mappings_.end())
        return;

    fs::path newPath = fsop();

    it->second.entry = fs::directory_entry(newPath);
    it->second.macname = newMacName;
    CNID parID = it->second.parID;

    auto& cachedContents = directories_[parID];
    
    std::sort(cachedContents.begin(), cachedContents.end(),
        [&](CNID a, CNID b) {
            return mappings_.at(a).entry.path() < mappings_.at(b).entry.path();
        });
}


