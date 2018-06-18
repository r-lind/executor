#pragma once

#include "macstrings.h"
#include "filesystem.h"
#include <memory>
#include <chrono>
#include <map>

namespace Executor
{
class LocalVolume;


class DirectoryItem;
class Item
{
protected:
    LocalVolume& volume_;
    fs::path path_;
    mac_string name_;
    long parID_;
    Item(LocalVolume& vol, fs::path p);

public:
    Item(const DirectoryItem& parent, fs::path p);
    virtual ~Item() = default;

    operator const fs::path& () const { return path_; }

    const fs::path& path() const { return path_; }

    long parID() const { return parID_; }
    const mac_string& name() const { return name_; }
};

using ItemPtr = std::shared_ptr<Item>;

class DirectoryCache
{

};

class DirectoryItem : public Item
{
    std::vector<ItemPtr>    contents_;
    std::map<mac_string, ItemPtr> contents_by_name_;
    std::chrono::steady_clock::time_point   cache_timestamp_;
    bool    cache_valid_ = false;

    long dirID_;

public:
    DirectoryItem(const DirectoryItem& parent, fs::path p, long dirID);
    DirectoryItem(LocalVolume& vol, fs::path p);

    void flushCache();
    void updateCache();

    ItemPtr resolve(mac_string_view name);
    ItemPtr resolve(int index);

    long dirID() const { return dirID_; }
};


}