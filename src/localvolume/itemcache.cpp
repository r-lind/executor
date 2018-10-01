#include "itemcache.h"
#include "item.h"
#include "cnidmapper.h"

using namespace Executor;

ItemCache::ItemCache(fs::path root,
                mac_string_view volumeName,
                std::unique_ptr<CNIDMapper> cnidMapper,
                ItemFactory *itemFactory
                )
    : cnidMapper(std::move(cnidMapper))
    , itemFactory(itemFactory)
{
    items[2] = rootDirItem = std::make_shared<DirectoryItem>(*this, 1, 2, root, volumeName);
}

ItemCache::~ItemCache()
{
}

void ItemCache::cleanDirectoryCache()
{
    using namespace std::chrono_literals;
    const auto cacheExpiryTime = 1s;
    const size_t maxCachedDirectories = 20;
    const auto now = std::chrono::steady_clock::now();
    
    while(!cachedDirectories.empty()
        && (cachedDirectories.front().timestamp + cacheExpiryTime < now
            || cachedDirectories.size() > maxCachedDirectories))
    {
        cachedDirectories.front().directory->clearCache();
        cachedDirectories.pop_front();
    }
}

void ItemCache::cacheDirectory(DirectoryItemPtr item)
{
    cleanDirectoryCache();
    if(!item->isCached())
    {
        item->populateCache();
        cachedDirectories.push_back({item, std::chrono::steady_clock::now()});
    }
}

void ItemCache::flushDirectoryCache(DirectoryItemPtr item)
{
    if(item->isCached())
    {
        item->clearCache();
        cachedDirectories.remove_if([&](const auto& p) { return p.directory == item; });
    }
}

void ItemCache::flushDirectoryCache(CNID dirID)
{
    auto it = items.find(dirID);
    if(it != items.end())
    {
        if(auto item = it->second.lock())
        {
            if(auto dir = std::dynamic_pointer_cast<DirectoryItem>(item))
                flushDirectoryCache(dir);
        }
    }
}

ItemPtr ItemCache::getItemForDirEntry(CNID parID, const fs::directory_entry& entry)
{
    if(itemFactory->isHidden(entry))
        return {};

    CNID cnid = cnidMapper->cnidForPath(entry.path());

    auto itemIt = items.find(cnid);
    if(itemIt != items.end())
    {
        if(auto itemPtr = itemIt->second.lock())
        {
            assert(itemPtr->cnid() == cnid);
            assert(itemPtr->parID() == parID);
            assert(itemPtr->path() == entry.path());
            return itemPtr;
        }
    }

    ItemPtr item = itemFactory->createItemForDirEntry(*this, parID, cnid, entry);

    if(item)
        items.emplace(item->cnid(), item);

    return item;
}

void ItemCache::noteItemFreed(CNID cnid)
{
    items.erase(cnid);
}

ItemPtr ItemCache::tryResolve(CNID cnid)
{
    {
        auto it = items.find(cnid);
        if(it != items.end())
            if(ItemPtr item = it->second.lock())
                return item;
    }

    fs::path path;
    if(auto optionalPath = cnidMapper->pathForCNID(cnid))
        path = *optionalPath;
    else
        return {};

    CNID parID = cnidMapper->cnidForPath(path.parent_path());

    if(ItemPtr item = itemFactory->createItemForDirEntry(*this, parID, cnid, fs::directory_entry(path)))
    {
        items.emplace(item->cnid(), item);
        return item;
    }
    else
    {
        cnidMapper->deleteCNID(cnid);
        return {};
    }
}

ItemPtr ItemCache::tryResolve(fs::path path)
{
    return tryResolve(cnidMapper->cnidForPath(path));
}

void ItemCache::deleteItem(ItemPtr item)
{
    item->deleteItem();
    
    items.erase(item->cnid());
    cnidMapper->deleteCNID(item->cnid());
    flushDirectoryCache(item->parID());
}

void ItemCache::renameItem(ItemPtr item, mac_string_view newName)
{
    item->renameItem(newName);

    cnidMapper->moveCNID(item->cnid(), item->path());
    flushDirectoryCache(item->parID());
}

void ItemCache::moveItem(ItemPtr item, DirectoryItemPtr newParent)
{
    flushDirectoryCache(item->parID());
    
    item->moveItem(newParent->path());
    cnidMapper->moveCNID(item->cnid(), item->path());

    flushDirectoryCache(newParent);
}

