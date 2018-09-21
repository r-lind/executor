#pragma once

#include "localvolume.h"
#include "item.h"

namespace Executor
{
class MacFileItem : public FileItem
{
public:
    using FileItem::FileItem;

    virtual FInfo getFInfo();
    virtual void setFInfo(FInfo finfo);
    virtual std::unique_ptr<OpenFile> open();
    virtual std::unique_ptr<OpenFile> openRF();
};


class MacHandler : public MetaDataHandler
{
    virtual ItemPtr handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e);
};
}