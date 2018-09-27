#include "item.h"
#include "localvolume.h"

#include <OSUtil.h>
#include <iostream>

using namespace Executor;

Item::Item(LocalVolume& vol, fs::path p)
    : volume_(vol), path_(std::move(p))
{
    name_ = toMacRomanFilename(path_.filename());

    parID_ = 1;
    cnid_ = 2;
}

Item::Item(LocalVolume& vol, CNID parID, CNID cnid, fs::path p)
    : volume_(vol), parID_(parID), cnid_(cnid), path_(std::move(p))
{
    name_ = toMacRomanFilename(path_.filename());
}

Item::~Item()
{
    volume_.noteItemFreed(cnid_);
}

void Item::deleteItem()
{
    fs::remove(path());
}

void Item::renameItem(mac_string_view newName)
{
    fs::path newPath = path().parent_path() / toUnicodeFilename(newName);
    fs::rename(path(), newPath);
    path_ = std::move(newPath);
    name_ = newName;
}

void Item::moveItem(const fs::path& newParent)
{
    fs::path newPath = newParent / path().filename();
    fs::rename(path(), newPath);
    path_ = std::move(newPath);
}

DirectoryItem::DirectoryItem(LocalVolume& vol, fs::path p)
    : Item(vol, std::move(p))
{
    name_ = vol.getVolumeName();
}

DirectoryItem::DirectoryItem(LocalVolume& vol, CNID parID, CNID cnid, fs::path p)
    : Item(vol, parID, cnid, std::move(p))
{
}

void DirectoryItem::clearCache()
{
    cache_valid_ = false;
    contents_.clear();
    contents_by_name_.clear();
    files_.clear();
}

void DirectoryItem::populateCache()
{
    if(cache_valid_)
        return;
    
    for(const auto& e : fs::directory_iterator(path_))
    {
        if(ItemPtr item = volume_.getItemForDirEntry(cnid(), e))
        {
            mac_string nameUpr = item->name();
            ROMlib_UprString(nameUpr.data(), false, nameUpr.size());
            auto inserted = contents_by_name_.emplace(nameUpr, item).second;
            if(inserted)
            {
                contents_.push_back(item);
                if(!dynamic_cast<DirectoryItem*>(item.get()))
                    files_.push_back(item);
            }
            else
            {
                std::cerr << "duplicate name mapping: " << e.path() << std::endl; 
            }
        }
    }

    cache_valid_ = true;
}

ItemPtr DirectoryItem::tryResolve(mac_string_view name)
{
    assert(cache_valid_);
    mac_string nameUpr { name };
    ROMlib_UprString(nameUpr.data(), false, nameUpr.size());
    auto it = contents_by_name_.find(nameUpr);
    if(it != contents_by_name_.end())
        return it->second;
    return {};
}

ItemPtr DirectoryItem::resolve(int index, bool includeDirectories)
{
    assert(cache_valid_);
    const auto& array = includeDirectories ? contents_ : files_;
    if(index >= 1 && index <= array.size())
        return array[index-1];
    throw OSErrorException(fnfErr);
}


void DirectoryItem::deleteItem()
{
    boost::system::error_code ec;
    fs::remove(path() / ".rsrc", ec);
    fs::remove(path() / ".finf", ec);       // TODO: individual ItemFactories should provide this info

    fs::remove(path(), ec);

    if(ec)
    {
        if(ec == boost::system::errc::directory_not_empty)
            throw OSErrorException(fBsyErr);
        else
            throw OSErrorException(paramErr);
    }
}
