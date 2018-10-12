#pragma once

#include "localvolume.h"
#include "item.h"

namespace Executor
{
class MacFileItem : public FileItem
{
public:
    using FileItem::FileItem;

    virtual ItemInfo getInfo() override;
    virtual void setInfo(ItemInfo info) override;
    virtual std::unique_ptr<OpenFile> open() override;
    virtual std::unique_ptr<OpenFile> openRF() override;
};


class MacItemFactory : public ItemFactory
{
    virtual ItemPtr createItemForDirEntry(ItemCache& itemcache, CNID parID, CNID cnid,
        const fs::directory_entry& e, mac_string_view macname) override;
    virtual void createFile(const fs::path& parentPath, mac_string_view name) override;
};
}