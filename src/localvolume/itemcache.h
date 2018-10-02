#pragma once

#include <rsys/filesystem.h>
#include <rsys/macstrings.h>
#include <memory>
#include <chrono>
#include <unordered_map>

namespace Executor
{
    
class Item;
class ItemFactory;
class CNIDMapper;
class DirectoryItem;

using ItemPtr = std::shared_ptr<Item>;
using DirectoryItemPtr = std::shared_ptr<DirectoryItem>;

using CNID = int32_t;

class ItemCache
{
    std::unique_ptr<CNIDMapper> cnidMapper;
    ItemFactory *itemFactory;

    std::unordered_map<CNID, std::weak_ptr<Item>> items; 
    
    DirectoryItemPtr rootDirItem;

    struct CachedDirectory
    {
        DirectoryItemPtr directory;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::list<CachedDirectory> cachedDirectories;

    void cleanDirectoryCache();
public:
    ItemCache(fs::path root, mac_string_view volumeName,
                std::unique_ptr<CNIDMapper> cnidMapper,
                ItemFactory *itemFactory
                );
    ~ItemCache();

    void noteItemFreed(CNID cnid);

    ItemPtr tryResolve(CNID cnid);
    ItemPtr tryResolve(fs::path path);

    ItemPtr getItemForDirEntry(CNID parID, const fs::directory_entry& path);

    void cacheDirectory(DirectoryItemPtr item);
    void flushDirectoryCache(DirectoryItemPtr item);
    void flushDirectoryCache(CNID dirID);

    void deleteItem(ItemPtr item);
    void renameItem(ItemPtr item, mac_string_view newName);
    void moveItem(ItemPtr item, DirectoryItemPtr newParent);
};

}
