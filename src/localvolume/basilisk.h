#pragma once

#include "localvolume.h"
#include "item.h"

namespace Executor
{
class BasiliskItemFactory : public ItemFactory
{
    LocalVolume &volume;

public:
    BasiliskItemFactory(LocalVolume &vol)
        : volume(vol)
    {
    }
    virtual bool isHidden(const fs::directory_entry &e) override;
    virtual ItemPtr createItemForDirEntry(LocalVolume& vol, CNID parID, CNID cnid, const fs::directory_entry& e) override;
    virtual void createFile(const fs::path& parentPath, mac_string_view name) override;
};

class BasiliskFileItem : public FileItem
{
public:
    using FileItem::FileItem;

    virtual FInfo getFInfo() override;
    virtual void setFInfo(FInfo finfo) override;
    virtual std::unique_ptr<OpenFile> open() override;
    virtual std::unique_ptr<OpenFile> openRF() override;

    virtual void deleteItem() override;
    virtual void renameItem(mac_string_view newName) override;
    virtual void moveItem(const fs::path &newParent) override;
};
} // namespace Executor