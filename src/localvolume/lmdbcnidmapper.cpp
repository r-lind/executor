#include "lmdbcnidmapper.h"
#include <rsys/file.h>
#include <set>
#include <OSUtil.h>
#include <iostream>

using namespace Executor;

namespace
{
    class BinMapping
    {
        std::vector<char> data_;
        lmdb::val val_;
        CNID cnid;
        lmdb::val cnidval_;
    public:
        BinMapping(const CNIDMapper::Mapping& mapping);

        lmdb::val& val() { return val_; }
        lmdb::val& cnidval() { return cnidval_; }
    };

    CNIDMapper::Mapping decodeMapping(CNID cnid, lmdb::val val)
    {
        char *data = val.data();
        CNID parId = *reinterpret_cast<CNID*>(data);
        data += sizeof(CNID);
        uint8_t macNameLen = *reinterpret_cast<uint8_t*>(data);
        data++;
        mac_string macName(reinterpret_cast<unsigned char*>(data), macNameLen);
        data += macNameLen;
        uint16_t pathLen = *reinterpret_cast<uint16_t*>(data);
        data += sizeof(uint16_t);
        std::string pathString(data, pathLen);

        return CNIDMapper::Mapping {
            parId, cnid,
            fs::path(std::move(pathString)),
            std::move(macName)
        };
    }
    
    BinMapping::BinMapping(const CNIDMapper::Mapping& mapping)
        : data_(sizeof(CNID) + 1 + mapping.macname.size() + 2 + mapping.path.string().size())
        , val_(data_.data(), data_.size())
        , cnid(mapping.cnid)
        , cnidval_(&cnid, sizeof(CNID))
    {
        char *data = data_.data();
        *reinterpret_cast<CNID*>(data) = mapping.parID;
        data += sizeof(CNID);
        *reinterpret_cast<uint8_t*>(data) = mapping.macname.size();
        data++;
        memcpy(data, mapping.macname.data(), mapping.macname.size());
        data += mapping.macname.size();
        *reinterpret_cast<uint16_t*>(data) = mapping.path.string().size();
        data += sizeof(uint16_t);
        memcpy(data, mapping.path.string().data(), mapping.path.string().size());
    }

}

LMDBCNIDMapper::LMDBCNIDMapper(fs::path root, mac_string volumeName)
    : env_(lmdb::env::create())
{
    fs::create_directories(ROMlib_DirectoryMap);
    env_.set_max_dbs(3);
    env_.open(ROMlib_DirectoryMap.string().c_str());

    auto txn = lmdb::txn::begin(env_);
    config_ = lmdb::dbi::open(txn, "config", MDB_CREATE);
    mappings_ = lmdb::dbi::open(txn, "mappings", MDB_CREATE | MDB_INTEGERKEY);
    directories_ = lmdb::dbi::open(txn, "directories", MDB_CREATE | MDB_INTEGERKEY);
    
    BinMapping rootMapping({
        1, 2,
        root,
        volumeName
    });
    
    mappings_.put(txn, rootMapping.cnidval(), rootMapping.val());

    txn.commit();
}

LMDBCNIDMapper::~LMDBCNIDMapper()
{
}

std::optional<CNIDMapper::Mapping> LMDBCNIDMapper::getMapping(lmdb::txn& txn, CNID cnid)
{
    lmdb::val mapping;

    if(mappings_.get(txn, lmdb::val(&cnid, sizeof(cnid)), mapping))
        return decodeMapping(cnid, mapping);
    else
        return {};
}

void LMDBCNIDMapper::setMapping(lmdb::txn& txn, Mapping m)
{
    BinMapping binMapping{m};
    mappings_.put(txn, binMapping.cnidval(), binMapping.val());
}

void LMDBCNIDMapper::deleteMapping(lmdb::txn& txn, CNID cnid)
{

}

std::vector<CNID> LMDBCNIDMapper::getDirectory(lmdb::txn& txn, CNID cnid)
{
    lmdb::val cachedContents;
    
    directories_.get(txn, lmdb::val(&cnid, sizeof(cnid)), cachedContents);

    auto cacheBegin = cachedContents.data<CNID>();
    auto cacheEnd = cacheBegin + cachedContents.size() / sizeof(CNID);

    return { cacheBegin, cacheEnd };
}

void LMDBCNIDMapper::setDirectory(lmdb::txn& txn, CNID cnid, const std::vector<CNID>& contents)
{
    lmdb::val contentVal = lmdb::val(contents.data(), contents.size() * sizeof(CNID));    
    directories_.put(txn, lmdb::val(&cnid, sizeof(cnid)), contentVal);
}
    

std::vector<CNIDMapper::Mapping> LMDBCNIDMapper::mapDirectoryContents(CNID dirID,
        std::vector<fs::path> realPaths)
{
    auto txn = lmdb::txn::begin(env_);

    std::sort(realPaths.begin(), realPaths.end());

    std::vector<Mapping> dirMappings;
    dirMappings.reserve(realPaths.size());

    std::vector<CNID> newContents;
    newContents.reserve(realPaths.size());

    std::vector<Mapping> mappings;

    for(CNID cnid : getDirectory(txn, dirID))
    {
        if(auto optMapping = getMapping(txn, cnid))
            mappings.push_back(std::move(*optMapping));
    }
    
    CNID newCNID;
    {
        auto cursor = lmdb::cursor::open(txn, mappings_);
        lmdb::val cnidval;
        if(cursor.get(cnidval, MDB_LAST))
            newCNID = *cnidval.data<CNID>() + 1;
        else
            newCNID = 3;
    }

    std::set<mac_string> usedNames;

    auto cachedMappingIt = mappings.begin(), cachedMappingEnd = mappings.end();
    for(auto& path : realPaths)
    {
        while(cachedMappingIt != cachedMappingEnd && cachedMappingIt->path < path)
        {
            // discard mapping
            deleteCNID(txn, cachedMappingIt->cnid);
            ++cachedMappingIt;
        }

        if(cachedMappingIt != cachedMappingEnd && cachedMappingIt->path == path)
        {
            // use existing mapping
            dirMappings.push_back(*cachedMappingIt);
            newContents.push_back(cachedMappingIt->cnid);

            ++cachedMappingIt;
        }
        else
        {
            // new mapping
            mac_string macname;
            const fs::path& name = path.filename();
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
                dirID, newCNID++,
                std::move(path),
                std::move(macname)
            };
            dirMappings.push_back(newMapping);
            newContents.push_back(newMapping.cnid);

            setMapping(txn, newMapping);
        }
    }

    setDirectory(txn, dirID, newContents);

    txn.commit();

    return dirMappings;
}

std::optional<CNIDMapper::Mapping> LMDBCNIDMapper::lookupCNID(CNID cnid)
{
    auto txn = lmdb::txn::begin(env_);

    auto mapping = getMapping(txn, cnid);

    if(!mapping)
        return {};
        
    if(fs::exists(mapping->path))
        return mapping;
    else
    {
        deleteCNID(txn, cnid);
        txn.commit();
        return {};
    }
}

void LMDBCNIDMapper::deleteCNID(lmdb::txn& txn, CNID cnid)
{
    auto mapping = getMapping(txn, cnid);
    if(!mapping)
        return;
    
    CNID parID = mapping->parID;

    auto cachedContents = getDirectory(txn, parID);

    cachedContents.erase(
        std::remove(cachedContents.begin(), cachedContents.end(), cnid),
        cachedContents.end());

    setDirectory(txn, parID, cachedContents);

    deleteMapping(txn, cnid);

    // deleteCNID is only invoked for empty directories by ItemCacher,
    // but mapDirectoryContents and lookupCNID above also invoke it
    // on files and directories that have been deleted or moved by other programs.
    for(CNID child : getDirectory(txn, cnid))
        deleteCNID(txn, child);
    directories_.del(txn, lmdb::val(&cnid, sizeof(cnid)));
}

void LMDBCNIDMapper::deleteCNID(CNID cnid)
{
    auto txn = lmdb::txn::begin(env_);
    deleteCNID(txn, cnid);
    txn.commit();
}
void LMDBCNIDMapper::moveCNID(CNID cnid, CNID newParent, mac_string_view newMacName, std::function<fs::path()> fsop)
{
    auto txn = lmdb::txn::begin(env_);

    auto mapping = getMapping(txn, cnid);
    if(!mapping)
        return;

    CNID oldParent = mapping->parID;
    if(newParent == 0)
        newParent = oldParent;
    if(oldParent == newParent && newMacName.empty())
        return;

    fs::path newPath = fsop();

    mapping->path = newPath;
    mapping->parID = newParent;
    if(!newMacName.empty())
        mapping->macname = newMacName;

    setMapping(txn, *mapping);

    auto cachedContents = getDirectory(txn, oldParent);
    cachedContents.erase(
        std::remove(cachedContents.begin(), cachedContents.end(), cnid),
        cachedContents.end());

    if(newParent != oldParent)
    {
        setDirectory(txn, oldParent, cachedContents);
        cachedContents = getDirectory(txn, newParent);
    }

    cachedContents.insert(
        std::upper_bound(cachedContents.begin(), cachedContents.end(), cnid,
            [&](CNID a, CNID b) {
                return getMapping(txn, a)->path < getMapping(txn, b)->path;
            }),
        cnid);

    setDirectory(txn, newParent, cachedContents);

    txn.commit();
}
