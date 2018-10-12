#pragma once

#include "localvolume.h"
#include "item.h"
#include "openfile.h"

namespace Executor
{
class PlainFileItem : public FileItem
{
public:
    using FileItem::FileItem;

    virtual ItemInfo getInfo() override;
    virtual void setInfo(ItemInfo info) override;

    virtual std::unique_ptr<OpenFile> open() override;
    virtual std::unique_ptr<OpenFile> openRF() override;
};

class PlainDataFork : public OpenFile
{
protected:
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
