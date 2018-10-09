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

    virtual void setEOF(size_t sz) override;
};

void MacResourceFork::setEOF(size_t sz)
{
    ftruncate(fd, sz);
    
    // ftruncate does not work for resouce forks on APFS volumes on High Sierra.
    size_t actual = lseek(fd, 0, SEEK_END);

    // FIXME: ... and on Mojave, removexattr fails while the file is open.
    if(actual > sz)
    {
        if(sz < 16 * 1024 * 1024)   // 16 MB is the maximum size for a resfork
        {
            std::string dfpath = path_.parent_path().parent_path().string();

            void *buf = malloc(sz);
            getxattr(dfpath.c_str(), "com.apple.ResourceFork", buf, sz,
                    0, 0);
            removexattr(dfpath.c_str(), "com.apple.ResourceFork", 0);
            setxattr(dfpath.c_str(), "com.apple.ResourceFork", buf, sz,
                    0, 0);
            free(buf);
        }
    }
    else if(actual < sz)
    {
        pwrite(fd, "", 1, sz-1);
    }
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

void MacItemFactory::createFile(const fs::path& parentPath, mac_string_view name)
{
    fs::path fn = toUnicodeFilename(name);
    fs::path path = parentPath / fn;

    PlainDataFork data(path, PlainDataFork::create);
}

std::unique_ptr<OpenFile> MacFileItem::open()
{
    return std::make_unique<PlainDataFork>(path_);
}
std::unique_ptr<OpenFile> MacFileItem::openRF()
{
    fs::path rsrc = path() / "..namedfork/rsrc";
    return std::make_unique<MacResourceFork>(rsrc, PlainDataFork::create);
}

FInfo MacFileItem::getFInfo()
{
    struct
    {
        FInfo finfo;
        FXInfo fxinfo;
    } buffer;
    
    if(getxattr(path().string().c_str(), XATTR_FINDERINFO_NAME, &buffer, 32, 0, 0) < 0)
        memset(&buffer, 0, sizeof(buffer));

    return buffer.finfo;
}

void MacFileItem::setFInfo(FInfo info)
{
    struct
    {
        FInfo finfo;
        FXInfo fxinfo;
    } buffer;
    
    if(getxattr(path().string().c_str(), XATTR_FINDERINFO_NAME, &buffer, 32, 0, 0) < 0)
        memset(&buffer, 0, sizeof(buffer));
    
    buffer.finfo = info;
    
    setxattr(path().string().c_str(), XATTR_FINDERINFO_NAME, &buffer, 32, 0, 0);
    
    // TODO: errors
}
#endif