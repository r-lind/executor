#pragma once

#include "localvolume.h"
#include "item.h"

namespace Executor
{
class AppleDoubleHandler : public MetaDataHandler
{
    LocalVolume &volume;

public:
    AppleDoubleHandler(LocalVolume &vol)
        : volume(vol)
    {
    }
    virtual bool isHidden(const fs::directory_entry &e);
    virtual ItemPtr handleDirEntry(const DirectoryItem &parent, const fs::directory_entry &e);
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
    virtual void moveItem(const fs::path &newParent);
};

class AppleSingleDoubleFork : public OpenFile
{
    struct EntryDescriptor
    {
        GUEST_STRUCT;
        GUEST<uint32_t> entryId;
        GUEST<uint32_t> offset;
        GUEST<uint32_t> length;
    };
    std::unique_ptr<OpenFile> file;
    EntryDescriptor desc;
    uint32_t descOffset;

public:
    AppleSingleDoubleFork(std::unique_ptr<OpenFile> file, int entry);
    ~AppleSingleDoubleFork();

    virtual size_t getEOF() override;
    virtual void setEOF(size_t sz) override;
    virtual size_t read(size_t offset, void *p, size_t n) override;
    virtual size_t write(size_t offset, void *p, size_t n) override;
};
} // namespace Executor