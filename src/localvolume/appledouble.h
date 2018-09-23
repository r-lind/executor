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
    virtual ItemPtr handleDirEntry(const DirectoryItem& parent, const fs::directory_entry &e);
    virtual void createFile(const fs::path& parentPath, mac_string_view name);
};

class AppleSingleDoubleFile;

class AppleDoubleFileItem : public FileItem
{
    std::weak_ptr<AppleSingleDoubleFile> openedFile;

    std::shared_ptr<AppleSingleDoubleFile> access();
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

class AppleSingleDoubleFile
{
    struct EntryDescriptor
    {
        GUEST_STRUCT;
        GUEST<uint32_t> entryId;
        GUEST<uint32_t> offset;
        GUEST<uint32_t> length;
    };
    std::unique_ptr<OpenFile> file;
    std::vector<EntryDescriptor> descriptors;

    using EntryDescriptorIterator = std::vector<EntryDescriptor>::iterator;
    EntryDescriptorIterator findEntry(uint32_t entryId);
    EntryDescriptorIterator setEOFCommon(uint32_t entryId, size_t sz, bool allowShrink);

public:
    struct create_single_t {};
    struct create_double_t {};

    static constexpr create_single_t create_single = {};
    static constexpr create_double_t create_double = {};

    AppleSingleDoubleFile(std::unique_ptr<OpenFile> file);
    AppleSingleDoubleFile(std::unique_ptr<OpenFile> file, create_single_t);
    AppleSingleDoubleFile(std::unique_ptr<OpenFile> file, create_double_t);
    ~AppleSingleDoubleFile();

    size_t getEOF(uint32_t entryId);
    void setEOF(uint32_t entryId, size_t sz);
    size_t read(uint32_t entryId, size_t offset, void *p, size_t n);
    size_t write(uint32_t entryId, size_t offset, void *p, size_t n);
};

class AppleSingleDoubleFork : public OpenFile
{
    std::shared_ptr<AppleSingleDoubleFile> file;
    uint32_t entryId;
public:
    AppleSingleDoubleFork(std::shared_ptr<AppleSingleDoubleFile> file, uint32_t entryId);
    ~AppleSingleDoubleFork();

    virtual size_t getEOF() override;
    virtual void setEOF(size_t sz) override;
    virtual size_t read(size_t offset, void *p, size_t n) override;
    virtual size_t write(size_t offset, void *p, size_t n) override;
};
} // namespace Executor