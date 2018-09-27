#pragma once

#include "localvolume.h"
#include "item.h"

namespace Executor
{
class MacFileItem : public FileItem
{
public:
    using FileItem::FileItem;

    virtual FInfo getFInfo() override;
    virtual void setFInfo(FInfo finfo) override;
    virtual std::unique_ptr<OpenFile> open() override;
    virtual std::unique_ptr<OpenFile> openRF() override;
};


class MacItemFactory : public ItemFactory
{
    virtual ItemPtr createItemForDirEntry(LocalVolume& vol, CNID parID, CNID cnid, const fs::directory_entry& e) override;
    virtual void createFile(const fs::path& parentPath, mac_string_view name) override;
};
}