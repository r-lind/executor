#include "appledouble.h"
#include "plain.h"

using namespace Executor;

bool AppleDoubleItemFactory::isHidden(const fs::directory_entry& e)
{
    if(e.path().filename().string().substr(0,1) == "%")
        return true;
    if(e.path().filename().string().substr(0,2) == "._")
        return true;
    return false;
}

ItemPtr AppleDoubleItemFactory::createItemForDirEntry(ItemCache& itemcache, CNID parID, CNID cnid,
    const fs::directory_entry& e, mac_string_view macname)
{
    if(fs::is_regular_file(e.path()))
    {
        fs::path adpath = e.path().parent_path() / ("%" + e.path().filename().string());
        
        if(fs::is_regular_file(adpath))
            return std::make_shared<AppleDoubleFileItem>(itemcache, parID, cnid, e.path(), macname);
    }
    return nullptr;
}

void AppleDoubleItemFactory::createFile(const fs::path& parentPath, mac_string_view name)
{
    fs::path fn = toUnicodeFilename(name);
    fs::path path = parentPath / fn;
    fs::path adpath = parentPath / ("%" + fn.filename().string());

    PlainDataFork data(path, PlainDataFork::create);
    AppleSingleDoubleFile rsrc(std::make_unique<PlainDataFork>(adpath, PlainDataFork::create),
                                AppleSingleDoubleFile::create_double);
}

std::shared_ptr<AppleSingleDoubleFile> AppleDoubleFileItem::access()
{
    std::shared_ptr<AppleSingleDoubleFile> p;
    if((p = openedFile.lock()))
        return p;
    else
    {
        fs::path adpath = path().parent_path() / ("%" + path().filename().string());
        p = std::make_shared<AppleSingleDoubleFile>(std::make_unique<PlainDataFork>(adpath));
        openedFile = p;
        return p;
    }
}
std::unique_ptr<OpenFile> AppleDoubleFileItem::open()
{
    return std::make_unique<PlainDataFork>(path_);
}
std::unique_ptr<OpenFile> AppleDoubleFileItem::openRF()
{
    return std::make_unique<AppleSingleDoubleFork>(access(), 2);
}

FInfo AppleDoubleFileItem::getFInfo()
{
    FInfo info = {0};

    access()->read(9, 0, &info, sizeof(info));

    return info;
}

void AppleDoubleFileItem::setFInfo(FInfo info)
{
    access()->write(9, 0, &info, sizeof(info));
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

void AppleDoubleFileItem::moveItem(const fs::path& newParent)
{
    fs::path newPath = newParent / path().filename();

    fs::path adpathOld = path().parent_path() / ("%" + path().filename().string());
    fs::path adpathNew = newParent / ("%" + path().filename().string());

    fs::rename(path(), newPath);

    boost::system::error_code ec;
    fs::rename(adpathOld, adpathNew, ec);

    path_ = std::move(newPath);
}


ItemPtr AppleSingleItemFactory::createItemForDirEntry(ItemCache& itemcache, CNID parID, CNID cnid,
    const fs::directory_entry& e, mac_string_view macname)
{
    if(!fs::is_regular(e.path()))
        return nullptr;
    uint64_t magic = 0;
    fs::ifstream(e.path(), std::ios::binary).read((char*)&guestref(magic), 8);

    if(magic == 0x0005160000020000)
        return std::make_shared<AppleSingleFileItem>(itemcache, parID, cnid, e.path(), macname);
    else
        return nullptr;
}

void AppleSingleItemFactory::createFile(const fs::path& parentPath, mac_string_view name)
{
    fs::path fn = toUnicodeFilename(name);
    fs::path path = parentPath / fn;

    AppleSingleDoubleFile rsrc(std::make_unique<PlainDataFork>(path, PlainDataFork::create),
                                AppleSingleDoubleFile::create_single);
}


std::shared_ptr<AppleSingleDoubleFile> AppleSingleFileItem::access()
{
    std::shared_ptr<AppleSingleDoubleFile> p;
    if((p = openedFile.lock()))
        return p;
    else
    {
        p = std::make_shared<AppleSingleDoubleFile>(std::make_unique<PlainDataFork>(path()));
        openedFile = p;
        return p;
    }
}
std::unique_ptr<OpenFile> AppleSingleFileItem::open()
{
    return std::make_unique<AppleSingleDoubleFork>(access(), 1);
}
std::unique_ptr<OpenFile> AppleSingleFileItem::openRF()
{
    return std::make_unique<AppleSingleDoubleFork>(access(), 2);
}

FInfo AppleSingleFileItem::getFInfo()
{
    FInfo info = {0};

    access()->read(9, 0, &info, sizeof(info));

    return info;
}

void AppleSingleFileItem::setFInfo(FInfo info)
{
    access()->write(9, 0, &info, sizeof(info));
}

AppleSingleDoubleFile::AppleSingleDoubleFile(std::unique_ptr<OpenFile> aFile)
    : file(std::move(aFile))
{
    //uint64_t magicAndVersion;
    //file->read(0, &guestref(magicAndVersion), 8);

    uint16_t n;
    file->read(24, &guestref(n), 2);
    descriptors.resize(n);
    file->read(26, descriptors.data(), sizeof(EntryDescriptor) * n);
    std::sort(descriptors.begin(), descriptors.end(), [](auto& a, auto& b) { return CL(a.offset) < CL(b.offset); });
}

AppleSingleDoubleFile::AppleSingleDoubleFile(std::unique_ptr<OpenFile> aFile, create_single_t)
    : file(std::move(aFile))
{
    uint64_t magicAndVersion = 0x0005160000020000;
    file->write(0, &guestref(magicAndVersion), 8);
    uint16_t zero = 0;
    file->write(24, &zero, 2);
}

AppleSingleDoubleFile::AppleSingleDoubleFile(std::unique_ptr<OpenFile> aFile, create_double_t)
    : file(std::move(aFile))
{
    uint64_t magicAndVersion = 0x0005160700020000;
    file->write(0, &guestref(magicAndVersion), 8);
    uint16_t zero = 0;
    file->write(24, &zero, 2);
}

AppleSingleDoubleFile::~AppleSingleDoubleFile()
{

}

size_t AppleSingleDoubleFile::getEOF(uint32_t entryId)
{
    auto p = findEntry(entryId);
    if(p != descriptors.end())
        return CL(p->length);
    return 0;
}

AppleSingleDoubleFile::EntryDescriptorIterator AppleSingleDoubleFile::findEntry(uint32_t entryId)
{
    return std::find_if(descriptors.begin(), descriptors.end(),
        [entryId](auto& desc) { return CL(desc.entryId) == entryId; });
}

AppleSingleDoubleFile::EntryDescriptorIterator AppleSingleDoubleFile::setEOFCommon(uint32_t entryId, size_t sz, bool allowShrink)
{
    // figure out which entry we are talking about,
    // and insert a new one if necessary
    auto p = findEntry(entryId);

    if(p == descriptors.end())
    {
        if(sz == 0)
            return p;
        p = descriptors.insert(p, EntryDescriptor{CL(entryId), CL(0), CL(0)});
    }
    
    // keep a record of the previous layout
    auto oldDescriptors = descriptors;

    // update EOF for selected entry
    if(allowShrink || CL(p->length) < sz)
        p->length = CL(sz);

    // update offsets for entries
    uint32_t offset = 24 + 2 + sizeof(EntryDescriptor) * descriptors.size();
    for(auto& desc : descriptors)
    {
        if(offset > CL(desc.offset))
        {
            desc.offset = CL(offset + 256);
        }
        else if(CL(desc.offset) - offset > 512)
        {
            desc.offset = CL(offset + 256); 
        }
        offset = CL(desc.offset) + CL(desc.length);
    }

    // shift data around.
    // 
    for(auto src = oldDescriptors.begin(), dst = descriptors.begin();
        dst != descriptors.end(); ++src, ++dst)
    {
        uint32_t dstOffset = CL(dst->offset);
        uint32_t srcOffset = CL(src->offset);        
        if(dstOffset < srcOffset)
        {
            uint32_t n = std::min(CL(dst->length), CL(src->length));

            while(n)
            {
                const uint32_t blockSize = 4096;
                uint32_t n1 = std::min(blockSize, n);
                uint8_t buf[blockSize];
                file->read(srcOffset, buf, n1);
                file->write(dstOffset, buf, n1);
                srcOffset += n1;
                dstOffset += n1;
                n -= n1;
            }
        }
    }

    for(auto src = oldDescriptors.rbegin(), dst = descriptors.rbegin();
        dst != descriptors.rend(); ++src, ++dst)
    {
        uint32_t dstOffset = CL(dst->offset);
        uint32_t srcOffset = CL(src->offset);        
        if(dstOffset > srcOffset)
        {
            uint32_t n = std::min(CL(dst->length), CL(src->length));
            dstOffset += n;
            srcOffset += n;
            while(n)
            {
                const uint32_t blockSize = 4096;
                uint32_t n1 = std::min(blockSize, n);
                uint8_t buf[blockSize];
                srcOffset -= n1;
                dstOffset -= n1;
                file->read(srcOffset, buf, n1);
                file->write(dstOffset, buf, n1);
                n -= n1;
            }
        }
    }

    // update headers
    {
        uint16_t n = descriptors.size();
        file->write(24, &guestref(n), 2);
        file->write(26, descriptors.data(), sizeof(EntryDescriptor) * n);
    }

    return p;
}

void AppleSingleDoubleFile::setEOF(uint32_t entryId, size_t sz)
{
    setEOFCommon(entryId, sz, true);
}

size_t AppleSingleDoubleFile::read(uint32_t entryId, size_t offset, void *p, size_t n)
{
    auto descIt = findEntry(entryId);
    if(descIt != descriptors.end())
    {
        if(offset >= CL(descIt->length))
            return 0;
        if(offset + n > CL(descIt->length))
            n = CL(descIt->length) - offset;
        return file->read(offset + CL(descIt->offset), p, n);
    }
    return 0;
}

size_t AppleSingleDoubleFile::write(uint32_t entryId, size_t offset, void *p, size_t n)
{
    if(n == 0)
        return 0;
    auto descIt = setEOFCommon(entryId, offset + n, false);
    return file->write(CL(descIt->offset) + offset, p, n);
}


AppleSingleDoubleFork::AppleSingleDoubleFork(std::shared_ptr<AppleSingleDoubleFile> file, uint32_t entryId)
    : file(file), entryId(entryId)
{

}

AppleSingleDoubleFork::~AppleSingleDoubleFork()
{
}

size_t AppleSingleDoubleFork::getEOF()
{
    return file->getEOF(entryId);
}
void AppleSingleDoubleFork::setEOF(size_t sz)
{
    file->setEOF(entryId, sz);
}
size_t AppleSingleDoubleFork::read(size_t offset, void *p, size_t n)
{
    return file->read(entryId, offset, p, n);
}
size_t AppleSingleDoubleFork::write(size_t offset, void *p, size_t n)
{
    return file->write(entryId, offset, p, n);
}
