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
    LMDBCNIDMapper(fs::path root);
    ~LMDBCNIDMapper();

    virtual CNID cnidForPath(fs::path path) override;
    virtual std::optional<fs::path> pathForCNID(CNID cnid) override;
    virtual void deleteCNID(CNID cnid) override;
    virtual void moveCNID(CNID cnid, fs::path path) override;
};

}