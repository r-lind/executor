#include "mac.h"
#include "host-os-config.h"
#include "plain.h"

#ifdef MACOSX
using namespace Executor;

#include <sys/types.h>
#include <unistd.h>
#include <sys/xattr.h>

class MacResourceFork : public PlainDataFork
{
public:
    using PlainDataFork::PlainDataFork;

    virtual size_t getEOF() override;
    virtual void setEOF(size_t sz) override;
    virtual size_t read(size_t offset, void *p, size_t n) override;
    virtual size_t write(size_t offset, void *p, size_t n) override;
};

// NOTE:
// The resource fork is available as filename/..namedfork/rsrc
// as a more-or-less regular file.
// However, on APFS volumes, ftruncate does not work on that file.
// On Mojave, removexattr doesn't work while that file is open.
// So, the code below opens the data fork and does everything
// via the xattr API. There is no API for shortening an xattr,
// but as ftruncate doesn't work anyway, and the maximum size of
// a resource fork is 16MB, we just read the whole thing into RAM,
// remove the attribute, and write it again at the desired size.

size_t MacResourceFork::getEOF()
{
    errno = 0;
    ssize_t sz = fgetxattr(fd, XATTR_RESOURCEFORK_NAME, nullptr, 0, 0, 0);
    if(errno == ENOATTR)
        return 0;
    if(sz < 0)
        throw OSErrorException(ioErr);
    return size_t(sz);
}

void MacResourceFork::setEOF(size_t sz)
{
    errno = 0;
    ssize_t curSz = fgetxattr(fd, XATTR_RESOURCEFORK_NAME, nullptr, 0, 0, 0);
    if(curSz < 0 && errno != ENOATTR)
        curSz = 0;
    if(curSz < 0)
        throw OSErrorException(ioErr);

    if(sz > 16 * 1024 * 1024)
        sz = 16 * 1024 * 1024;   // 16 MB is the maximum size for a resfork

    if(sz > curSz)
    {
        fsetxattr(fd, XATTR_RESOURCEFORK_NAME, "", 1, sz - 1, 0);
    }
    else if(sz < curSz)
    {
        void *buf = malloc(sz);
        fgetxattr(fd, XATTR_RESOURCEFORK_NAME, buf, sz, 0, 0);
        fremovexattr(fd, XATTR_RESOURCEFORK_NAME, 0);
        fsetxattr(fd, XATTR_RESOURCEFORK_NAME, buf, sz, 0, 0);
        free(buf);
    }
}

size_t MacResourceFork::read(size_t offset, void *p, size_t n)
{
    return fgetxattr(fd, XATTR_RESOURCEFORK_NAME, p, n, offset, 0);
}

size_t MacResourceFork::write(size_t offset, void *p, size_t n)
{
    int success = fsetxattr(fd, XATTR_RESOURCEFORK_NAME, p, n, offset, 0);
    if(success < 0)
        throw OSErrorException(ioErr);
    return n;
}

ItemPtr MacItemFactory::createItemForDirEntry(ItemCache& itemcache, CNID parID, CNID cnid,
    const fs::directory_entry& e, mac_string_view macname)
{
    if(fs::is_regular_file(e.path()))
    {
        return std::make_shared<MacFileItem>(itemcache, parID, cnid, e.path(), macname);
    }
    return nullptr;
}

void MacItemFactory::createFile(const fs::path& path)
{
    PlainDataFork data(path, PlainDataFork::create);
}

std::unique_ptr<OpenFile> MacFileItem::open()
{
    return std::make_unique<PlainDataFork>(path());
}
std::unique_ptr<OpenFile> MacFileItem::openRF()
{
    return std::make_unique<MacResourceFork>(path());
}

ItemInfo MacFileItem::getInfo()
{
    ItemInfo info = {0};

    getxattr(path().string().c_str(), XATTR_FINDERINFO_NAME, &info, 32, 0, 0);

    return info;
}

void MacFileItem::setInfo(ItemInfo info)
{
    setxattr(path().string().c_str(), XATTR_FINDERINFO_NAME, &info, 32, 0, 0);
    
    // TODO: errors
}
#endif
