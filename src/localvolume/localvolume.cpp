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
        return volume.lookupDirectory(parent, e.path());
    }
    return nullptr;
}

ItemPtr ExtensionHandler::handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e)
{
    if(fs::is_regular_file(e.path()))
    {
        return std::make_shared<FileItem>(parent, e.path());
    }
    return nullptr;
}

Item::Item(LocalVolume& vol, fs::path p)
    : volume_(vol), path_(std::move(p))
{
    name_ = toMacRomanFilename(path_.filename());

    parID_ = 2;
}

Item::Item(const DirectoryItem& parent, fs::path p)
    : volume_(parent.volume_), path_(std::move(p))
{
    name_ = toMacRomanFilename(path_.filename());

    parID_ = parent.dirID();
}

std::unique_ptr<OpenFile> FileItem::open()
{
    return std::make_unique<PlainDataFork>(path_);
}

PlainDataFork::PlainDataFork(fs::path path)
    : stream(path)
{
}
PlainDataFork::~PlainDataFork()
{
}

size_t PlainDataFork::getEOF()
{
    stream.seekg(0, std::ios::end);
    return stream.tellg();
}
void PlainDataFork::setEOF(size_t sz)
{
    
}
size_t PlainDataFork::read(size_t offset, void *p, size_t n)
{
    stream.seekg(offset);
    stream.read((char*)p, n);
    stream.clear(); // ?
    return stream.gcount();
}
size_t PlainDataFork::write(size_t offset, void *p, size_t n)
{
    return 0;
}


DirectoryItem::DirectoryItem(LocalVolume& vol, fs::path p)
    : Item(vol, std::move(p)), dirID_(2)
{
}

DirectoryItem::DirectoryItem(const DirectoryItem& parent, fs::path p, long dirID)
    : Item(parent, std::move(p)), dirID_(dirID)
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
        for(auto& handler : volume_.handlers)
        {
            if(ItemPtr item = handler->handleDirEntry(*this, e))
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

LocalVolume::LocalVolume(VCB& vcb, fs::path root)
    : Volume(vcb), root(root)
{
    pathToId[root] = 2;
    directories_[2] = std::make_shared<DirectoryItem>(*this, root);

    handlers.push_back(std::make_unique<DirectoryHandler>(*this));
    handlers.push_back(std::make_unique<ExtensionHandler>(*this));
}

std::shared_ptr<DirectoryItem> LocalVolume::lookupDirectory(const DirectoryItem& parent, const fs::path& path)
{
    auto [it, inserted] = pathToId.emplace(path, nextDirectory);
    if(inserted)
    {
        DirID id = nextDirectory++;
        auto dir = std::make_shared<DirectoryItem>(parent, path, id);
        directories_.emplace(id, dir);
        return dir;
    }
    else
        return directories_.at(it->second);
}

std::shared_ptr<DirectoryItem> LocalVolume::resolve(short vRef, long dirID)
{
    if(dirID)
    {
        auto it = directories_.find(dirID);
        if(it == directories_.end())
            throw OSErrorException(fnfErr);
        else
            return it->second;
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

    // TODO: handle pathnames

    std::shared_ptr<DirectoryItem> dir = resolve(vRef, dirID);
    return dir->resolve(name);
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


void LocalVolume::PBHRename(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBHCreate(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBDirCreate(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBHDelete(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}

struct LocalVolume::FCBExtension
{
    filecontrolblock *fcb;
    short refNum;
    std::unique_ptr<OpenFile> access;

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


void LocalVolume::PBHOpen(HParmBlkPtr pb)
{
    ItemPtr item = resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), CL(pb->fileParam.ioDirID));
    if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        FCBExtension& fcbx = openFCBX();
        fcbx.access = fileItem->open();
        fcbx.fcb->fcbFlNum = CL(42);
        pb->ioParam.ioRefNum = CW(fcbx.refNum);
    }
    else
        throw OSErrorException(paramErr);    // TODO: what's the correct error?
}
void LocalVolume::PBHOpenRF(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBHGetFInfo(HParmBlkPtr pb)
{
    StringPtr inputName = MR(pb->fileParam.ioNamePtr);
    if(CW(pb->fileParam.ioFDirIndex) > 0)
        inputName = nullptr;
    
        // negative index means what?
    ItemPtr item = resolve(PascalStringView(inputName),
        CW(pb->fileParam.ioVRefNum), CL(pb->fileParam.ioDirID), CW(pb->fileParam.ioFDirIndex));

    std::cout << item->path() << std::endl;
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
    ItemPtr item = resolve(PascalStringView(inputName),
        CW(pb->fileParam.ioVRefNum), 0, CW(pb->fileParam.ioFDirIndex));

    std::cout << item->path() << std::endl;
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
void LocalVolume::PBSetFInfo(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBHSetFInfo(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}

void LocalVolume::PBGetCatInfo(CInfoPBPtr pb)
{
    StringPtr inputName = MR(pb->hFileInfo.ioNamePtr);
    if(CW(pb->hFileInfo.ioFDirIndex) > 0)
        inputName = nullptr;
    ItemPtr item = resolve(PascalStringView(inputName),
        CW(pb->hFileInfo.ioVRefNum), CL(pb->hFileInfo.ioDirID), CW(pb->hFileInfo.ioFDirIndex));

    std::cout << item->path() << std::endl;
    if(StringPtr outputName = MR(pb->hFileInfo.ioNamePtr))
    {
        const mac_string name = item->name();
        size_t n = std::min(name.size(), (size_t)255);
        memcpy(outputName+1, name.data(), n);
        outputName[0] = n;
    }
    
    if(DirectoryItem *dirItem = dynamic_cast<DirectoryItem*>(item.get()))
    {
        pb->dirInfo.ioFlAttrib = ATTRIB_ISADIR;
        pb->dirInfo.ioDrDirID = CL(dirItem->dirID());
        pb->dirInfo.ioDrParID = CL(dirItem->parID());

    }
    else if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
            pb->hFileInfo.ioFlAttrib = 0;
            pb->hFileInfo.ioFlFndrInfo = fileItem->getFInfo();
    }
}
void LocalVolume::PBSetCatInfo(CInfoPBPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBCatMove(CMovePBPtr pb)
{
    throw OSErrorException(paramErr);
}

void LocalVolume::PBOpen(ParmBlkPtr pb)
{
    ItemPtr item = resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), 0);
    if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
    {
        FCBExtension& fcbx = openFCBX();
        fcbx.access = fileItem->open();
        fcbx.fcb->fcbFlNum = CL(42);
        pb->ioParam.ioRefNum = CW(fcbx.refNum);
    }
    else
        throw OSErrorException(paramErr);    // TODO: what's the correct error?
}
void LocalVolume::PBOpenRF(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBCreate(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBDelete(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
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
void LocalVolume::PBRename(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBSetVInfo(HParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBFlushVol(ParmBlkPtr pb)
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
void LocalVolume::PBRead(ParmBlkPtr pb)
{
    PBSetFPos(pb);
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    size_t n = fcbx.access->read(CL(fcbx.fcb->fcbCrPs), MR(pb->ioParam.ioBuffer), CL(pb->ioParam.ioReqCount));
    pb->ioParam.ioActCount = CL(n);
    fcbx.fcb->fcbCrPs = CL( CL(fcbx.fcb->fcbCrPs) + n );
}
void LocalVolume::PBWrite(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBClose(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBAllocate(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBAllocContig(ParmBlkPtr pb)
{
    throw OSErrorException(paramErr);
}
void LocalVolume::PBSetEOF(ParmBlkPtr pb)
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
void LocalVolume::PBGetFPos(ParmBlkPtr pb)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    pb->ioParam.ioReqCount = CL(0);
    pb->ioParam.ioActCount = CL(0);
    pb->ioParam.ioPosMode = CW(0);
    pb->ioParam.ioPosOffset = fcbx.fcb->fcbCrPs;
}

void LocalVolume::PBSetFPos(ParmBlkPtr pb)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    size_t eof = fcbx.access->getEOF();
    size_t newPos = CL(fcbx.fcb->fcbCrPs);
    
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
    if(newPos > eof)
    {
        newPos = eor;
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
void LocalVolume::PBGetEOF(ParmBlkPtr pb)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    size_t n = fcbx.access->getEOF();
    pb->ioParam.ioMisc = CL(n);
}
void LocalVolume::PBFlushFile(ParmBlkPtr pb)
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
