#include "localvolume.h"
#include "item.h"

using namespace Executor;

bool AppleDoubleHandler::isHidden(const fs::directory_entry& e)
{
    if(e.path().filename().string().substr(0,1) == "%")
        return true;
    if(e.path().filename().string().substr(0,2) == "._")
        return true;
    return false;
}


ItemPtr AppleDoubleHandler::handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e)
{
    if(fs::is_regular_file(e.path()))
    {
        fs::path adpath = e.path().parent_path() / ("%" + e.path().filename().string());
        
        if(fs::is_regular_file(adpath))
            return std::make_shared<AppleDoubleFileItem>(parent, e.path());
    }
    return nullptr;
}

std::unique_ptr<OpenFile> AppleDoubleFileItem::open()
{
    return std::make_unique<PlainDataFork>(path_);
}
std::unique_ptr<OpenFile> AppleDoubleFileItem::openRF()
{
    fs::path adpath = path().parent_path() / ("%" + path().filename().string());
    return std::make_unique<AppleSingleDoubleFork>(std::make_unique<PlainDataFork>(adpath), 2);
}

FInfo AppleDoubleFileItem::getFInfo()
{
    fs::path adpath = path().parent_path() / ("%" + path().filename().string());

    FInfo info = {0};

    AppleSingleDoubleFork(std::make_unique<PlainDataFork>(adpath), 9).read(0, &info, sizeof(info));

    return info;
}

void AppleDoubleFileItem::setFInfo(FInfo info)
{
    fs::path adpath = path().parent_path() / ("%" + path().filename().string());

    AppleSingleDoubleFork(std::make_unique<PlainDataFork>(adpath), 9).write(0, &info, sizeof(info));
}

void AppleDoubleFileItem::deleteItem()
{
    fs::remove(path());
    fs::path adpath = path().parent_path() / ("%" + path().filename().string());
    boost::system::error_code ec;
    fs::remove(adpath, ec);
}

void AppleDoubleFileItem::renameItem(mac_string_view newName)
{
    fs::path newFN = toUnicodeFilename(newName);
    fs::path newPath = path().parent_path() / newFN;

    fs::path adpathOld = path().parent_path() / ("%" + path().filename().string());
    fs::path adpathNew = path().parent_path() / ("%" + newFN.string());

    fs::rename(path(), newPath);

    boost::system::error_code ec;
    fs::rename(adpathOld, adpathNew, ec);

    path_ = std::move(newPath);
    name_ = newName;
}


AppleSingleDoubleFork::AppleSingleDoubleFork(std::unique_ptr<OpenFile> aFile, int entry)
    : file(std::move(aFile))
{
    uint16_t n;
    file->read(24, &guestref(n), 2);
    for(int i = 0; i < n; i++)
    {
        size_t offset = 26 + i * sizeof(desc);
        file->read(offset, &desc, sizeof(desc));
        if(CL(desc.entryId) == entry)
        {
            descOffset = offset;
            break;
        }
    }
}

AppleSingleDoubleFork::~AppleSingleDoubleFork()
{
}

size_t AppleSingleDoubleFork::getEOF()
{
    return CL(desc.length);
}
void AppleSingleDoubleFork::setEOF(size_t sz)
{
    
}
size_t AppleSingleDoubleFork::read(size_t offset, void *p, size_t n)
{
    if(offset + n > CL(desc.length))
        n = CL(desc.length) - offset;

    return file->read(offset + CL(desc.offset), p, n);
}
size_t AppleSingleDoubleFork::write(size_t offset, void *p, size_t n)
{
    return 0;
}
