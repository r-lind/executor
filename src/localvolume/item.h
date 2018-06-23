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


class FileItem : public Item
{
public:
    using Item::Item;

    virtual FInfo getFInfo() = 0;
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

    virtual std::unique_ptr<OpenFile> open();
    virtual std::unique_ptr<OpenFile> openRF();
};

class BasiliskFileItem : public FileItem
{
public:
    using FileItem::FileItem;

    virtual FInfo getFInfo();
    virtual std::unique_ptr<OpenFile> open();
    virtual std::unique_ptr<OpenFile> openRF();
};


}