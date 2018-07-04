#include "localvolume.h"
#include <rsys/common.h>
#include <rsys/byteswap.h>
#include <FileMgr.h>
#include <MemoryMgr.h>
#include <rsys/file.h>
#include <rsys/hfs.h>
#include <map>
#include <iostream>
#include "item.h"

using namespace Executor;


ItemPtr DirectoryHandler::handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e)
{
    if(fs::is_directory(e.path()))
    {
        return std::make_shared<DirectoryItem>(parent, e.path());
    }
    return nullptr;
}

ItemPtr ExtensionHandler::handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e)
{
    if(fs::is_regular_file(e.path()))
    {
        return std::make_shared<PlainFileItem>(parent, e.path());
    }
    return nullptr;
}


Item::Item(LocalVolume& vol, fs::path p)
    : volume_(vol), path_(std::move(p))
{
    name_ = toMacRomanFilename(path_.filename());

    parID_ = 1;
    cnid_ = 2;
}

Item::Item(const DirectoryItem& parent, fs::path p)
    : volume_(parent.volume_), path_(std::move(p))
{
    name_ = toMacRomanFilename(path_.filename());

    parID_ = parent.dirID();
    cnid_ = volume_.newCNID();
}

void Item::deleteItem()
{
    fs::remove(path());
}

void Item::renameItem(mac_string_view newName)
{
    fs::path newPath = path().parent_path() / toUnicodeFilename(newName);
    fs::rename(path(), newPath);
    path_ = std::move(newPath);
    name_ = newName;
}

void Item::moveItem(const fs::path& newParent)
{
    fs::path newPath = newParent / path().filename();
    fs::rename(path(), newPath);
    path_ = std::move(newPath);
}

std::unique_ptr<OpenFile> PlainFileItem::open()
{
    return std::make_unique<PlainDataFork>(path_);
}
std::unique_ptr<OpenFile> PlainFileItem::openRF()
{
    return std::make_unique<EmptyFork>();
}


DirectoryItem::DirectoryItem(LocalVolume& vol, fs::path p)
    : Item(vol, std::move(p))
{
    name_ = vol.getVolumeName();
}

DirectoryItem::DirectoryItem(const DirectoryItem& parent, fs::path p)
    : Item(parent, std::move(p))
{
}

void DirectoryItem::flushCache()
{
    cache_valid_ = false;
    contents_.clear();
    contents_by_name_.clear();
}
void DirectoryItem::updateCache()
{
    auto now = std::chrono::steady_clock::now();
    if(cache_valid_)
    {
        using namespace std::chrono_literals;
        if(now > cache_timestamp_ + 1s)
        {
            std::cout << "flushed.\n";
            flushCache();
        }
    }
    if(cache_valid_)
        return;
    
    for(const auto& e : fs::directory_iterator(path_))
    {
        if(ItemPtr item = volume_.getItemForDirEntry(*this, e))
        {
            mac_string nameUpr = item->name();
            ROMlib_UprString(nameUpr.data(), false, nameUpr.size());
            auto [it, inserted] = contents_by_name_.emplace(nameUpr, item);
            if(inserted)
            {
                contents_.push_back(item);
            }
            else
            {
                std::cout << "duplicate name mapping: " << e.path() << std::endl; 
            }
        }
    }

    cache_timestamp_ = now;
    cache_valid_ = true;
}

ItemPtr DirectoryItem::resolve(mac_string_view name)
{
    updateCache();
    mac_string nameUpr { name };
    ROMlib_UprString(nameUpr.data(), false, nameUpr.size());
    auto it = contents_by_name_.find(nameUpr);
    if(it != contents_by_name_.end())
        return it->second;
    throw OSErrorException(fnfErr);
}

ItemPtr DirectoryItem::resolve(int index)
{
    updateCache();
    if(index >= 1 && index <= contents_.size())
        return contents_[index-1];
    throw OSErrorException(fnfErr);
}

void DirectoryItem::deleteItem()
{
    boost::system::error_code ec;
    fs::remove(path() / ".rsrc", ec);
    fs::remove(path() / ".finf", ec);       // TODO: individual handlers should provide this info

    fs::remove(path(), ec);

    if(ec)
    {
        if(ec == boost::system::errc::directory_not_empty)
            throw OSErrorException(fBsyErr);
        else
            throw OSErrorException(paramErr);
    }
}

LocalVolume::LocalVolume(VCB& vcb, fs::path root)
    : Volume(vcb), root(root)
{
    pathToId[root] = 2;
    items[2] = std::make_shared<DirectoryItem>(*this, root);

    handlers.push_back(std::make_unique<DirectoryHandler>(*this));
#ifdef MACOSX
    handlers.push_back(std::make_unique<MacHandler>());
#else
    handlers.push_back(std::make_unique<AppleDoubleHandler>(*this));
    handlers.push_back(std::make_unique<BasiliskHandler>(*this));
#endif
    handlers.push_back(std::make_unique<ExtensionHandler>(*this));
}

ItemPtr LocalVolume::getItemForDirEntry(const DirectoryItem& parent, const fs::directory_entry& entry)
{
    for(auto& handler : handlers)
        if(handler->isHidden(entry))
            return ItemPtr();

    auto it = pathToId.find(entry.path());
    if(it != pathToId.end())
        return items.at(it->second);

    for(auto& handler : handlers)
    {
        if(ItemPtr item = handler->handleDirEntry(parent, entry))
        {
            pathToId.emplace(entry.path(), item->cnid());
            items.emplace(item->cnid(), item);
            return item;
        }
    }

    return ItemPtr();
}

CNID LocalVolume::newCNID()
{
    return nextCNID++;
}

std::shared_ptr<DirectoryItem> LocalVolume::resolve(short vRef, long dirID)
{
    if(dirID)
    {
        auto it = items.find(dirID);
        if(it == items.end())
            throw OSErrorException(fnfErr);
        else if(auto dirItem = std::dynamic_pointer_cast<DirectoryItem>(it->second))
            return dirItem;
        else
            throw OSErrorException(fnfErr); // not a directory
    }
    else if(vRef == 0)
    {
        return resolve(CW(vcb.vcbVRefNum), CL(DefDirID));
    }
    else if(ISWDNUM(vRef))
    {
        auto wdp = WDNUMTOWDP(vRef);
        return resolve(CW(vcb.vcbVRefNum), CL(wdp->dirid));
    }
    else
    {
        // "poor man's search path": not implemented
        return resolve(CW(vcb.vcbVRefNum), 2);
    }
}
ItemPtr LocalVolume::resolve(mac_string_view name, short vRef, long dirID)
{
    if(name.empty())
        return resolve(vRef, dirID);

    size_t colon = name.find(':');

    if(colon != mac_string_view::npos)
    {
        std::shared_ptr<DirectoryItem> dir = resolve(vRef, colon == 0 ? dirID : 2);
        return resolveRelative(dir, mac_string_view(name.begin() + colon, name.end()));
    }
    else
    {
        std::shared_ptr<DirectoryItem> dir = resolve(vRef, dirID);
        return dir->resolve(name);
    }
}

ItemPtr LocalVolume::resolveRelative(const std::shared_ptr<DirectoryItem>& base, mac_string_view name)
{
    auto p = name.begin();

    if(p == name.end())
        return base;

    ++p;

    if(p == name.end())
        return base;

    auto colon = std::find(p, name.end(), ':');

    ItemPtr item;
    
    if(colon == p)
    {
        if(base->parID() == 1)
            throw OSErrorException(dirNFErr);
        item = items.at(base->parID());
    }
    else
        item = base->resolve(mac_string_view(p, colon));

    if(colon == name.end())
        return item;
    else if(auto dir = std::dynamic_pointer_cast<DirectoryItem>(item))
    {
        return resolveRelative(dir, mac_string_view(colon, name.end()));
    }
    else
        throw OSErrorException(fnfErr);
}

ItemPtr LocalVolume::resolve(short vRef, long dirID, short index)
{
    std::shared_ptr<DirectoryItem> dir = resolve(vRef, dirID);
    return dir->resolve(index);
}

ItemPtr LocalVolume::resolve(mac_string_view name, short vRef, long dirID, short index)
{
    if(index > 0)
        return resolve(vRef, dirID, index);
    else if(index == 0 && !name.empty())
        return resolve(name, vRef, dirID);
    else
        return resolve(vRef, dirID);
}

mac_string LocalVolume::getVolumeName() const
{
    return mac_string(mac_string_view(vcb.vcbVN));
}


struct LocalVolume::FCBExtension
{
    filecontrolblock *fcb;
    short refNum;
    std::unique_ptr<OpenFile> access;
    std::shared_ptr<FileItem> file;

    FCBExtension() = default;
    FCBExtension(filecontrolblock *fcb, short refNum)
        : fcb(fcb), refNum(refNum)
    {}
};

LocalVolume::FCBExtension& LocalVolume::getFCBX(short refNum)
{
    if(refNum < 0 || refNum >= fcbExtensions.size())
        throw OSErrorException(paramErr);
    return fcbExtensions[refNum];
}

LocalVolume::FCBExtension& LocalVolume::openFCBX()
{
    filecontrolblock* fcb = ROMlib_getfreefcbp();
    short refNum = (char *)fcb - (char *)MR(LM(FCBSPtr));

    memset(fcb, 0, sizeof(*fcb));
    fcb->fcbVPtr = RM(&vcb);

    if(fcbExtensions.size() < refNum + 1)
        fcbExtensions.resize(refNum+1);
    fcbExtensions[refNum] = FCBExtension(fcb, refNum);

    return fcbExtensions[refNum];
}

void LocalVolume::openCommon(GUEST<short>& refNum, ItemPtr item, Fork fork, int8_t permission)
{
    if(auto fileItem = std::dynamic_pointer_cast<FileItem>(item))
    {
        short& writeAccessRef = fork == Fork::resource ? fileItem->resWriteAccessRefNum : fileItem->dataWriteAccessRefNum;

        if(permission != fsRdPerm)
        {
            if(writeAccessRef > 0)
            {
                refNum = CW(writeAccessRef);
                throw OSErrorException(opWrErr);
            }
        }

        auto access = fork == Fork::resource ? fileItem->openRF() : fileItem->open();
        FCBExtension& fcbx = openFCBX();
        fcbx.access = std::move(access);
        fcbx.fcb->fcbFlNum = CL(fileItem->cnid());
        fcbx.file = fileItem;
        refNum = CW(fcbx.refNum);

        if(permission != fsRdPerm)
        {
            writeAccessRef = fcbx.refNum;
        }

    }
    else
        throw OSErrorException(paramErr);    // TODO: what's the correct error?
}

void LocalVolume::PBHOpenDF(HParmBlkPtr pb)
{
    openCommon(pb->ioParam.ioRefNum, resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), CL(pb->fileParam.ioDirID)), Fork::data, pb->ioParam.ioPermssn);
}
void LocalVolume::PBHOpenRF(HParmBlkPtr pb)
{
    openCommon(pb->ioParam.ioRefNum, resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), CL(pb->fileParam.ioDirID)), Fork::resource, pb->ioParam.ioPermssn);
}

void LocalVolume::PBOpenDF(ParmBlkPtr pb)
{
    openCommon(pb->ioParam.ioRefNum, resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), 0), Fork::data, pb->ioParam.ioPermssn);
}
void LocalVolume::PBOpenRF(ParmBlkPtr pb)
{
    openCommon(pb->ioParam.ioRefNum, resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), 0), Fork::resource, pb->ioParam.ioPermssn);
}


void LocalVolume::PBHGetFInfo(HParmBlkPtr pb)
{
    StringPtr inputName = MR(pb->fileParam.ioNamePtr);
    if(CW(pb->fileParam.ioFDirIndex) > 0)
        inputName = nullptr;
    
        // negative index means what?
    ItemPtr item = resolve(PascalStringView(inputName),
        CW(pb->fileParam.ioVRefNum), CL(pb->fileParam.ioDirID), CW(pb->fileParam.ioFDirIndex));

    std::cout << "HGetFInfo: " << item->path() << std::endl;
    if(StringPtr outputName = MR(pb->fileParam.ioNamePtr))
    {
        const mac_string name = item->name();
        size_t n = std::min(name.size(), (size_t)255);
        memcpy(outputName+1, name.data(), n);
        outputName[0] = n;
    }
    
    if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        pb->fileParam.ioFlAttrib = 0;
        pb->fileParam.ioFlFndrInfo = fileItem->getFInfo();
    }
    else
        throw OSErrorException(paramErr);
}
void LocalVolume::PBGetFInfo(ParmBlkPtr pb)
{
    StringPtr inputName = MR(pb->fileParam.ioNamePtr);
    if(CW(pb->fileParam.ioFDirIndex) > 0)
        inputName = nullptr;
    
        // negative index means what?
    ItemPtr item = resolve(inputName,
        CW(pb->fileParam.ioVRefNum), 0, CW(pb->fileParam.ioFDirIndex));

    std::cout << "GetFInfo: " << item->path() << std::endl;
    if(StringPtr outputName = MR(pb->fileParam.ioNamePtr))
    {
        const mac_string name = item->name();
        size_t n = std::min(name.size(), (size_t)255);
        memcpy(outputName+1, name.data(), n);
        outputName[0] = n;
    }
    
    if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        pb->fileParam.ioFlAttrib = 0;   // TODO
        pb->fileParam.ioFlFndrInfo = fileItem->getFInfo();
        pb->fileParam.ioFlCrDat = 0;    // TODO
        pb->fileParam.ioFlMdDat = 0;    // TODO
    }
    else
        throw OSErrorException(paramErr);
}

void LocalVolume::setFInfoCommon(Item& item, ParmBlkPtr pb)
{
    if(FileItem *fitem = dynamic_cast<FileItem*>(&item))
    {
        fitem->setFInfo(pb->fileParam.ioFlFndrInfo);
    }
    else
        throw OSErrorException(paramErr);   // TODO: item is a directory
    
    // pb->fileParam.ioFlCrDat      TODO
    // pb->fileParam.ioFlMdDat      TODO
}
void LocalVolume::PBSetFInfo(ParmBlkPtr pb)
{
    setFInfoCommon(*resolve(MR(pb->fileParam.ioNamePtr), CW(pb->fileParam.ioVRefNum), 0), pb);
}
void LocalVolume::PBHSetFInfo(HParmBlkPtr pb)
{
    setFInfoCommon(*resolve(MR(pb->fileParam.ioNamePtr), CW(pb->fileParam.ioVRefNum), CL(pb->fileParam.ioDirID)),
        reinterpret_cast<ParmBlkPtr>(pb));
}

void LocalVolume::PBGetCatInfo(CInfoPBPtr pb)
{
    StringPtr inputName = MR(pb->hFileInfo.ioNamePtr);
    if(CW(pb->hFileInfo.ioFDirIndex) != 0)
        inputName = nullptr;
    ItemPtr item = resolve(PascalStringView(inputName),
        CW(pb->hFileInfo.ioVRefNum), CL(pb->hFileInfo.ioDirID), CW(pb->hFileInfo.ioFDirIndex));

    std::cout.clear();
    std::cout << "GetCatInfo: " << item->path() << std::endl;
    if(StringPtr outputName = MR(pb->hFileInfo.ioNamePtr))
    {
        if(!inputName)
        {
            const mac_string name = item->name();
            size_t n = std::min(name.size(), (size_t)255);
            memcpy(outputName+1, name.data(), n);
            outputName[0] = n;
        }
    }
    
    if(DirectoryItem *dirItem = dynamic_cast<DirectoryItem*>(item.get()))
    {
        pb->dirInfo.ioFlAttrib = ATTRIB_ISADIR;
        pb->dirInfo.ioDrDirID = CL(dirItem->dirID());

        pb->dirInfo.ioDrParID = CL(dirItem->parID());

        pb->dirInfo.ioDrNmFls = CW(dirItem->countItems());

        // ioACUser
        // ioDrUserWds
        // ioDrCrDat
        // ioDrMdDat
        // ioDrBkDat
        // ioDrFndrInfo
    }
    else if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        pb->hFileInfo.ioFlAttrib = 0;
        pb->hFileInfo.ioFlFndrInfo = fileItem->getFInfo();

        pb->hFileInfo.ioVRefNum = vcb.vcbVRefNum;
        pb->hFileInfo.ioFlParID = CL(fileItem->parID());
        pb->hFileInfo.ioDirID = CL(fileItem->cnid());
        // ioFlStBlk
        // ioFlLgLen
        // ioFlPyLen
        // ioFlRStBlk
        // ioFlRPyLen

        // ioFlCrDat
        // ioFlMdDat
        // ioFlBkDat
        // ioFlXFndrInfo
        // ioFlClpSiz
    }
}
void LocalVolume::PBSetCatInfo(CInfoPBPtr pb)
{
    throw OSErrorException(paramErr);
}

LocalVolume::NonexistentFile LocalVolume::resolveForCreate(mac_string_view name, short vRefNum, CNID dirID)
{
    std::shared_ptr<DirectoryItem> parent;
    auto colon = name.rfind(':');

    if(colon == mac_string_view::npos)
        parent = resolve(vRefNum, dirID);
    else
    {
        parent = std::dynamic_pointer_cast<DirectoryItem>(resolve(mac_string_view(name.begin(), colon), vRefNum, dirID));
        if(!parent)
            throw OSErrorException(dirNFErr);
        name = mac_string_view(name.begin() + colon+1, name.end());
    }

    try
    {
        parent->resolve(name);
    }
    catch(OSErrorException& e)
    {
        if(e.code == fnfErr)
            ;
        else if(e.code == noErr)
            throw OSErrorException(dupFNErr);
        else
            throw;
    }

    return NonexistentFile { parent, name };
}

void LocalVolume::createCommon(NonexistentFile file)
{
    fs::path parentPath = file.parent->path();
    fs::path fn = toUnicodeFilename(file.name);

    // TODO: this is the responsibility of the BasiliskHandler class
    fs::create_directory(parentPath / ".rsrc");
    fs::create_directory(parentPath / ".finf");

    fs::ofstream(parentPath / fn);
    fs::ofstream(parentPath / ".rsrc" / fn);
    FInfo info = {0};
    fs::ofstream(parentPath / ".finf" / fn).write((char*)&info, sizeof(info));
    file.parent->flushCache();
}
void LocalVolume::PBCreate(ParmBlkPtr pb)
{
    createCommon(resolveForCreate(MR(pb->ioParam.ioNamePtr), CW(pb->fileParam.ioVRefNum), 0));
}
void LocalVolume::PBHCreate(HParmBlkPtr pb)
{
    createCommon(resolveForCreate(MR(pb->ioParam.ioNamePtr), CW(pb->fileParam.ioVRefNum), CL(pb->fileParam.ioDirID)));
}

void LocalVolume::PBDirCreate(HParmBlkPtr pb)
{
    auto [parent, name] = resolveForCreate(MR(pb->ioParam.ioNamePtr), CW(pb->fileParam.ioVRefNum), CL(pb->fileParam.ioDirID));
    fs::path fn = toUnicodeFilename(name);
    fs::create_directory(parent->path() / fn);

    parent->flushCache();

    pb->fileParam.ioDirID = CL(parent->resolve(name)->cnid());
}


void LocalVolume::deleteCommon(ItemPtr item)
{
    item->deleteItem();
    
    items.erase(item->cnid());
    pathToId.erase(item->path());
    dynamic_cast<DirectoryItem&>(*items.at(item->parID())).flushCache();
}

void LocalVolume::PBDelete(ParmBlkPtr pb)
{
    deleteCommon(resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), 0));
}
void LocalVolume::PBHDelete(HParmBlkPtr pb)
{
    deleteCommon(resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), CL(pb->fileParam.ioDirID)));
}

void LocalVolume::renameCommon(ItemPtr item, mac_string_view newName)
{
    if(newName.find(':') != mac_string_view::npos)
        throw OSErrorException(bdNamErr);

    auto& parent = dynamic_cast<DirectoryItem&>(*items.at(item->parID()));
    try
    {
        parent.resolve(newName);
    }
    catch(OSErrorException& e)
    {
        if(e.code == fnfErr)
            ;
        else if(e.code == noErr)
            throw OSErrorException(dupFNErr);
        else
            throw;
    }

    fs::path oldPath = item->path();
    item->renameItem(newName);

    pathToId.erase(oldPath);
    pathToId.emplace(item->path(), item->cnid());
    parent.flushCache();
}


void LocalVolume::PBRename(ParmBlkPtr pb)
{
    renameCommon(resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), 0), 
                MR(guest_cast<StringPtr>(pb->ioParam.ioMisc)));
}

void LocalVolume::PBHRename(HParmBlkPtr pb)
{
    renameCommon(resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), CL(pb->fileParam.ioDirID)), 
                MR(guest_cast<StringPtr>(pb->ioParam.ioMisc)));
}

void LocalVolume::PBOpenWD(WDPBPtr pb)
{
    ItemPtr item = resolve(MR(pb->ioNamePtr), CW(pb->ioVRefNum), CL(pb->ioWDDirID));
    std::cout << "PBOpenWD: " << item->path() << std::endl;

    long dirID;
    if(DirectoryItem *dirItem = dynamic_cast<DirectoryItem*>(item.get()))
        dirID = dirItem->dirID();
    else
        dirID = item->parID();

    OSErr err =  ROMlib_mkwd(pb, &vcb, dirID, Cx(pb->ioWDProcID));
    if(err)
        throw OSErrorException(err);
}

void LocalVolume::PBRead(ParmBlkPtr pb)
{
    pb->ioParam.ioActCount = 0;
    PBSetFPos(pb);
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    size_t req = CL(pb->ioParam.ioReqCount);
    size_t act = fcbx.access->read(CL(fcbx.fcb->fcbCrPs), MR(pb->ioParam.ioBuffer), req);
    pb->ioParam.ioActCount = CL(act);
    pb->ioParam.ioPosOffset = fcbx.fcb->fcbCrPs = CL( CL(fcbx.fcb->fcbCrPs) + act );

    if(act < req)
        throw OSErrorException(eofErr);
}
void LocalVolume::PBWrite(ParmBlkPtr pb)
{
    pb->ioParam.ioActCount = 0;
    setFPosCommon(pb, false);
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    size_t n = fcbx.access->write(CL(fcbx.fcb->fcbCrPs), MR(pb->ioParam.ioBuffer), CL(pb->ioParam.ioReqCount));
    pb->ioParam.ioActCount = CL(n);
    pb->ioParam.ioPosOffset = fcbx.fcb->fcbCrPs = CL( CL(fcbx.fcb->fcbCrPs) + n );
}
void LocalVolume::PBClose(ParmBlkPtr pb)
{
    short refNum = CW(pb->ioParam.ioRefNum);
    auto& fcbx = getFCBX(refNum);
    fcbx.fcb->fcbFlNum = 0;

    if(fcbx.file->resWriteAccessRefNum == refNum)
        fcbx.file->resWriteAccessRefNum = -1;
    if(fcbx.file->dataWriteAccessRefNum == refNum)
        fcbx.file->dataWriteAccessRefNum = -1;

    fcbx = FCBExtension();
}

void LocalVolume::PBGetFPos(ParmBlkPtr pb)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    pb->ioParam.ioReqCount = CL(0);
    pb->ioParam.ioActCount = CL(0);
    pb->ioParam.ioPosMode = CW(0);
    pb->ioParam.ioPosOffset = fcbx.fcb->fcbCrPs;
}

void LocalVolume::setFPosCommon(ParmBlkPtr pb, bool checkEOF)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    ssize_t eof = (ssize_t) fcbx.access->getEOF();
    ssize_t newPos = CL(fcbx.fcb->fcbCrPs);
    
    switch(CW(pb->ioParam.ioPosMode))
    {
        case fsAtMark:
            break;
        case fsFromStart:
            newPos = CL(pb->ioParam.ioPosOffset);
            break;
        case fsFromLEOF:
            newPos = eof + CL(pb->ioParam.ioPosOffset);
            break;
        case fsFromMark:
            newPos = CL(fcbx.fcb->fcbCrPs) + CL(pb->ioParam.ioPosOffset);
            break;
    }

    OSErr err = noErr;
    if(newPos > eof && checkEOF)
    {
        newPos = eof;
        err = eofErr;
    }
    
    if(newPos < 0)
    {
        newPos = CL(fcbx.fcb->fcbCrPs);     // MacOS 9 behavior.
                                            // System 6 actually sets fcbCrPs to a negative value.
        err = posErr;
    }

    pb->ioParam.ioPosOffset = fcbx.fcb->fcbCrPs = CL(newPos);

    if(err)
        throw OSErrorException(err);
}
void LocalVolume::PBSetFPos(ParmBlkPtr pb)
{
    setFPosCommon(pb, true);
}
void LocalVolume::PBGetEOF(ParmBlkPtr pb)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    size_t n = fcbx.access->getEOF();
    pb->ioParam.ioMisc = CL(n);
}

void LocalVolume::PBSetEOF(ParmBlkPtr pb)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    fcbx.access->setEOF(CL(pb->ioParam.ioMisc));
}

void LocalVolume::PBCatMove(CMovePBPtr pb)
{
    ItemPtr item = resolve(MR(pb->ioNamePtr), CW(pb->ioVRefNum), CL(pb->ioDirID));
    auto newParent = std::dynamic_pointer_cast<DirectoryItem>(resolve(MR(pb->ioNewName), CW(pb->ioVRefNum), CL(pb->ioNewDirID)));

    if(!newParent)
        throw OSErrorException(dirNFErr);   // check this?

    auto name = item->name();

    auto &parent = dynamic_cast<DirectoryItem &>(*items.at(item->parID()));
    try
    {
        newParent->resolve(mac_string_view(name.data(), name.size()));
    }
    catch(OSErrorException &e)
    {
        if(e.code == fnfErr)
            ;
        else if(e.code == noErr)
            throw OSErrorException(dupFNErr);
        else
            throw;
    }

    fs::path oldPath = item->path();
    item->moveItem(newParent->path());

    pathToId.erase(oldPath);
    pathToId.emplace(item->path(), item->cnid());
    parent.flushCache();
    newParent->flushCache();
}

void LocalVolume::PBFlushFile(ParmBlkPtr pb)
{
}
void LocalVolume::PBFlushVol(ParmBlkPtr pb)
{
}

void LocalVolume::PBAllocate(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBAllocContig(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBLockRange(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBUnlockRange(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBSetFLock(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBHSetFLock(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBRstFLock(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBHRstFLock(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBSetFVers(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}

void LocalVolume::PBSetVInfo(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBUnmountVol(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBEject(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBOffLine(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}

void Executor::initLocalVol()
{
    VCBExtra *vp;
    GUEST<THz> savezone;

    savezone = LM(TheZone);
    LM(TheZone) = LM(SysZone);
    vp = (VCBExtra *)NewPtr(sizeof(VCBExtra));
    LM(TheZone) = savezone;

    if(!vp)
        return;
    memset(vp, 0, sizeof(VCBExtra));
    vp->vcb.vcbDrvNum = CWC(42);//pb->ioParam.ioVRefNum;

    --ROMlib_nextvrn;
    vp->vcb.vcbVRefNum = CW(ROMlib_nextvrn);

    
    strcpy((char *)vp->vcb.vcbVN + 1, "vol");
    vp->vcb.vcbVN[0] = strlen((char *)vp->vcb.vcbVN+1);

    vp->vcb.vcbSigWord = CWC(0x4244); /* IMIV-188 */
    vp->vcb.vcbFreeBks = CWC(20480); /* arbitrary */
    vp->vcb.vcbCrDate = 0; /* I'm lying here */
    vp->vcb.vcbVolBkUp = 0;
    vp->vcb.vcbAtrb = CWC(VNONEJECTABLEBIT);
    vp->vcb.vcbNmFls = CWC(100);
    vp->vcb.vcbNmAlBlks = CWC(300);
    vp->vcb.vcbAlBlkSiz = CLC(512);
    vp->vcb.vcbClpSiz = CLC(1);
    vp->vcb.vcbAlBlSt = CWC(10);
    vp->vcb.vcbNxtCNID = CLC(1000);
    if(!LM(DefVCBPtr))
    {
        LM(DefVCBPtr) = RM((VCB *)vp);
        LM(DefVRefNum) = vp->vcb.vcbVRefNum;
        DefDirID = CLC(2); /* top level */
    }
    Enqueue((QElemPtr)vp, &LM(VCBQHdr));

    vp->volume = new LocalVolume(vp->vcb, "/");
}
