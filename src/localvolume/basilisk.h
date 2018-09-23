#pragma once

#include "localvolume.h"
#include "item.h"

namespace Executor
{
class BasiliskHandler : public MetaDataHandler
{
    LocalVolume &volume;

public:
    BasiliskHandler(LocalVolume &vol)
        : volume(vol)
    {
    }
    virtual bool isHidden(const fs::directory_entry &e);
    virtual ItemPtr handleDirEntry(const DirectoryItem &parent, const fs::directory_entry &e);
    virtual void createFile(const fs::path& parentPath, mac_string_view name);
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
    virtual void moveItem(const fs::path &newParent);
};
} // namespace Executor