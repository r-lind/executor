#pragma once

#include "cnidmapper.h"

#include <memory>
#include <lmdb++.h>


namespace Executor
{

class LMDBCNIDMapper : public CNIDMapper
{
    lmdb::env env_;
    lmdb::dbi config_;
    lmdb::dbi mappings_;
    lmdb::dbi directories_;

    struct StoredMapping
    {
        CNID parID;
        CNID cnid;
        fs::path path;
        mac_string macname;
    };

    class BinMapping;
    StoredMapping decodeMapping(CNID cnid, lmdb::val val);

    std::optional<StoredMapping> getMapping(lmdb::txn& txn, CNID cnid);
    void setMapping(lmdb::txn& txn, StoredMapping m);
    void deleteMapping(lmdb::txn& txn, CNID cnid);
    std::vector<CNID> getDirectory(lmdb::txn& txn, CNID cnid);
    void setDirectory(lmdb::txn& txn, CNID cnid, const std::vector<CNID>& contents);
    
    void deleteCNID(lmdb::txn& txn, CNID cnid);
public:
    LMDBCNIDMapper(fs::path root, mac_string volumeName);
    ~LMDBCNIDMapper();

    virtual std::vector<Mapping> mapDirectoryContents(CNID dirID, std::vector<fs::directory_entry> realPaths) override;
    virtual std::optional<Mapping> lookupCNID(CNID cnid) override;
    virtual void deleteCNID(CNID cnid) override;
    virtual void moveCNID(CNID cnid, CNID newParent, mac_string_view newMacName, std::function<fs::path()> fsop) override;
};

}