#include "mac.h"
#include "host-os-config.h"
#include "plain.h"

#ifdef MACOSX
using namespace Executor;

#include <sys/xattr.h>

ItemPtr MacHandler::handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e)
{
    if(fs::is_regular_file(e.path()))
    {
        return std::make_shared<MacFileItem>(parent, e.path());
    }
    return nullptr;
}

void MacHandler::createFile(const fs::path& parentPath, mac_string_view name)
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
    return std::make_unique<PlainDataFork>(rsrc);
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