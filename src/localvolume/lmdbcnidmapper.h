#pragma once

#include "cnidmapper.h"

#include <memory>
#include <lmdb++.h>


namespace Executor
{

class LMDBCNIDMapper : public CNIDMapper
{
    lmdb::env env_;
    lmdb::dbi pathToId_;
    lmdb::dbi idToPath_;
public:
    explicit LMDBCNIDMapper(fs::path root);
    ~LMDBCNIDMapper();

//    virtual std::vector<Mapping> mapDirectoryContents(CNID dirID, std::vector<fs::directory_entry> realPaths) override;
//    virtual std::optional<Mapping> lookupCNID(CNID cnid) override;
//    virtual void deleteCNID(CNID cnid) override;
//    virtual void moveCNID(CNID cnid, fs::path path) override;
};

}