#include "lmdbcnidmapper.h"
#include <iostream>

using namespace Executor;

LMDBCNIDMapper::LMDBCNIDMapper(fs::path root)
    : env_(lmdb::env::create())
{
    boost::system::error_code ec;
    fs::create_directory("executor.mdb", ec);
    env_.set_max_dbs(2);
    env_.open("executor.mdb");

    auto txn = lmdb::txn::begin(env_);
    pathToId_ = lmdb::dbi::open(txn, "pathToId", MDB_CREATE);
    idToPath_ = lmdb::dbi::open(txn, "idToPath", MDB_CREATE | MDB_INTEGERKEY);
    
    CNID newCNID = 2;
    auto path = root.string();
    lmdb::val pathval(path);
    lmdb::val cnidval(&newCNID, sizeof(newCNID));
    pathToId_.put(txn, pathval, cnidval);
    idToPath_.put(txn, cnidval, pathval);

    txn.commit();
}

LMDBCNIDMapper::~LMDBCNIDMapper()
{
}

CNID LMDBCNIDMapper::cnidForPath(fs::path path)
{
    auto txn = lmdb::txn::begin(env_);
    
    lmdb::val cnidval;

    if(pathToId_.get(txn, lmdb::val(path.string()), cnidval))
        return *cnidval.data<CNID>();

    CNID newCNID;
    {
        auto cursor = lmdb::cursor::open(txn, idToPath_);

        if(cursor.get(cnidval, MDB_LAST))
            newCNID = *cnidval.data<CNID>() + 1;
        else
            newCNID = 3;
    }

    lmdb::val pathval(path.string());
    cnidval = { &newCNID, sizeof(newCNID) };
    pathToId_.put(txn, pathval, cnidval);
    idToPath_.put(txn, cnidval, pathval);

    txn.commit();

    return newCNID;
}

std::optional<fs::path> LMDBCNIDMapper::pathForCNID(CNID cnid)
{
    auto txn = lmdb::txn::begin(env_, nullptr, MDB_RDONLY);

    lmdb::val cnidval(&cnid, sizeof(cnid));
    lmdb::val val;
    
    if(!idToPath_.get(txn, cnidval, val))
        return {};

    return fs::path(std::string(val.data(), val.data() + val.size()));
}

void LMDBCNIDMapper::deleteCNID(CNID cnid)
{
    auto txn = lmdb::txn::begin(env_);

    lmdb::val cnidval(&cnid, sizeof(cnid));
    lmdb::val pathval;
    
    if(!idToPath_.get(txn, cnidval, pathval))
        return;

    pathToId_.del(txn,pathval);
    idToPath_.del(txn,cnidval);

    txn.commit();
}

void LMDBCNIDMapper::moveCNID(CNID cnid, fs::path path)
{
    auto txn = lmdb::txn::begin(env_);

    lmdb::val cnidval(&cnid, sizeof(cnid));
    lmdb::val pathval;
    
    if(!idToPath_.get(txn, cnidval, pathval))
        return;

    pathToId_.del(txn, pathval);
    pathval = path.string();
    pathToId_.put(txn, pathval, cnidval);
    idToPath_.put(txn, cnidval, pathval);

    txn.commit();
}

