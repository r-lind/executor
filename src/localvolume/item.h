#pragma once

#include "macstrings.h"
#include "filesystem.h"
#include <memory>
#include <chrono>
#include <map>

#include <rsys/macros.h>

namespace Executor
{
class LocalVolume;
class OpenFile;

using CNID = long;

class DirectoryItem;
class Item
{
protected:
    LocalVolume& volume_;
    fs::path path_;
    mac_string name_;
    CNID cnid_;
    CNID parID_;
    Item(LocalVolume& vol, fs::path p);

public:
    Item(const DirectoryItem& parent, fs::path p);
    virtual ~Item() = default;

    operator const fs::path& () const { return path_; }

    const fs::path& path() const { return path_; }

    CNID parID() const { return parID_; }
    CNID cnid() const { return cnid_; }

    const mac_string& name() const { return name_; }

    virtual void deleteItem();
    virtual void renameItem(mac_string_view newName);
};

using ItemPtr = std::shared_ptr<Item>;

class DirectoryItem : public Item
{
    std::vector<ItemPtr>    contents_;
    std::map<mac_string, ItemPtr> contents_by_name_;
    std::chrono::steady_clock::time_point   cache_timestamp_;
    bool    cache_valid_ = false;

public:
    DirectoryItem(const DirectoryItem& parent, fs::path p);
    DirectoryItem(LocalVolume& vol, fs::path p);

    void flushCache();
    void updateCache();

    ItemPtr resolve(mac_string_view name);
    ItemPtr resolve(int index);

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

class PlainFileItem : public FileItem
{
public:
    using FileItem::FileItem;

    virtual FInfo getFInfo()
    {
        return FInfo {
            TICKX("TEXT"),
            TICKX("ttxt"),
            CWC(0),  // fdFlags
            { CWC(0), CWC(0) },   // fdLocation
            CWC(0)   // fdFldr
        };
    }

    virtual void setFInfo(FInfo finfo)
    {
    }


    virtual std::unique_ptr<OpenFile> open();
    virtual std::unique_ptr<OpenFile> openRF();
};

class BasiliskFileItem : public FileItem
{
public:
    using FileItem::FileItem;

    virtual FInfo getFInfo();
    virtual void setFInfo(FInfo finfo);
    virtual std::unique_ptr<OpenFile> open();
    virtual std::unique_ptr<OpenFile> openRF();

    virtual void deleteItem();
    virtual void renameItem(mac_string_view newName);
};

class AppleDoubleFileItem : public FileItem
{
public:
    using FileItem::FileItem;

    virtual FInfo getFInfo();
    virtual void setFInfo(FInfo finfo);
    virtual std::unique_ptr<OpenFile> open();
    virtual std::unique_ptr<OpenFile> openRF();

    virtual void deleteItem();
    virtual void renameItem(mac_string_view newName);
};


}