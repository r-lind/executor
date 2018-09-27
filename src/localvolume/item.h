#pragma once

#include <rsys/macstrings.h>
#include <rsys/filesystem.h>
#include <FileMgr.h>

#include <memory>
#include <map>

namespace Executor
{
class ItemCache;
class LocalVolume;
class OpenFile;

using CNID = long;

class Item;
class DirectoryItem;

using ItemPtr = std::shared_ptr<Item>;
using DirectoryItemPtr = std::shared_ptr<DirectoryItem>;

class Item
{
protected:
    ItemCache& itemcache_;
    CNID parID_;
    CNID cnid_;
    fs::path path_;
    mac_string name_;

public:
    Item(ItemCache& itemcache, CNID parID, CNID cnid, fs::path p, mac_string_view name = {});
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


class ItemFactory
{
public:
    virtual ~ItemFactory() = default;

    virtual bool isHidden(const fs::directory_entry& e) { return false; }
    virtual ItemPtr createItemForDirEntry(ItemCache& itemcache, CNID parID, CNID cnid, const fs::directory_entry& e) = 0;
    virtual void createFile(const fs::path& parentPath, mac_string_view name)
        { throw std::logic_error("createFile unimplemented"); }
};


class DirectoryItemFactory : public ItemFactory
{
public:
    virtual ItemPtr createItemForDirEntry(ItemCache& itemcache, CNID parID, CNID cnid, const fs::directory_entry& e) override;
};

class DirectoryItem : public Item
{
    std::vector<ItemPtr>    contents_;
    std::vector<ItemPtr>    files_;
    std::map<mac_string, ItemPtr> contents_by_name_;
    bool    cache_valid_ = false;

    friend class ItemCache;
    void clearCache();
    void populateCache();
    bool isCached() const { return cache_valid_; }
public:
    using Item::Item;

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
