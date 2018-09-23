#pragma once

#include "localvolume.h"
#include "item.h"

namespace Executor
{
class PlainFileItem : public FileItem
{
public:
    using FileItem::FileItem;

    virtual FInfo getFInfo()
    {
        return FInfo{
            TICKX("TEXT"),
            TICKX("ttxt"),
            CWC(0), // fdFlags
            { CWC(0), CWC(0) }, // fdLocation
            CWC(0) // fdFldr
        };
    }

    virtual void setFInfo(FInfo finfo)
    {
    }

    virtual std::unique_ptr<OpenFile> open();
    virtual std::unique_ptr<OpenFile> openRF();
};

class PlainDataFork : public OpenFile
{
    int fd;
    fs::path path_;

public:
    struct create_t {};
    static constexpr create_t create = {};

    PlainDataFork(fs::path path);
    PlainDataFork(fs::path path, create_t);
    ~PlainDataFork();

    virtual size_t getEOF() override;
    virtual void setEOF(size_t sz) override;
    virtual size_t read(size_t offset, void *p, size_t n) override;
    virtual size_t write(size_t offset, void *p, size_t n) override;
};
} // namespace Executor
