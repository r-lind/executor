#pragma once

#include <rsys/macstrings.h>
#include <rsys/filesystem.h>
#include <memory>
#include <chrono>
#include <map>
#include <iostream>

#include <rsys/macros.h>

namespace Executor
{
class LocalVolume;
class OpenFile;

using CNID = long;

template<class T>
class InstanceCounter
{
public:
    static int count;

    void report()
    {
        std::cerr << "Number of " << typeid(T).name() << " instances: " << count << std::endl << std::flush;
    }

    InstanceCounter()
    {
        ++count;
        report();
    }

    ~InstanceCounter()
    {
        --count;
        report();
    }
};
template<class T> 
int InstanceCounter<T>::count = 0; 

class DirectoryItem;
class Item
{
protected:
    LocalVolume& volume_;
    CNID parID_;
    CNID cnid_;
    fs::path path_;
    mac_string name_;

    InstanceCounter<Item> counter_;
    Item(LocalVolume& vol, fs::path p);

public:
    Item(LocalVolume& vol, CNID parID, CNID cnid, fs::path p);
    virtual ~Item();

    operator const fs::path& () const { return path_; }

    const fs::path& path() const { return path_; }

    CNID parID() const { return parID_; }
    CNID cnid() const { return cnid_; }

    const mac_string& name() const { return name_; }

    virtual void deleteItem();
    virtual void renameItem(mac_string_view newName);
    virtual void moveItem(const fs::path& newParent);
};

class DirectoryItem;

using ItemPtr = std::shared_ptr<Item>;
using DirectoryItemPtr = std::shared_ptr<DirectoryItem>;

class DirectoryItem : public Item
{
    std::vector<ItemPtr>    contents_;
    std::vector<ItemPtr>    files_;
    std::map<mac_string, ItemPtr> contents_by_name_;
    bool    cache_valid_ = false;

    InstanceCounter<DirectoryItem> counter_;

    friend class LocalVolume;
    void clearCache();
    void populateCache();
    bool isCached() const { return cache_valid_; }
public:
    DirectoryItem(LocalVolume& vol, CNID parID, CNID cnid, fs::path p);
    DirectoryItem(LocalVolume& vol, fs::path p);

    ItemPtr tryResolve(mac_string_view name);
    ItemPtr resolve(int index, bool includeDirectories);

    long dirID() const { return cnid(); }

    int countItems() { return contents_.size(); }

    virtual void deleteItem();
};


class FileItem : public Item
{
    friend class Executor::LocalVolume;
    short dataWriteAccessRefNum = -1;
    short resWriteAccessRefNum = -1;

public:
    using Item::Item;


    virtual FInfo getFInfo() = 0;
    virtual void setFInfo(FInfo finfo) = 0;
    virtual std::unique_ptr<OpenFile> open() = 0;
    virtual std::unique_ptr<OpenFile> openRF() = 0;
};

}
