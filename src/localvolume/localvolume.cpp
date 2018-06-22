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


OSErr LocalVolume::PBHRename(HParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBHCreate(HParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBDirCreate(HParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBHDelete(HParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
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


OSErr LocalVolume::PBHOpen(HParmBlkPtr pb, BOOLEAN async)
{
    try
    {  
        ItemPtr item = resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), CL(pb->fileParam.ioDirID));
        if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
        {
            FCBExtension& fcbx = openFCBX();
            fcbx.access = fileItem->open();
            fcbx.fcb->fcbFlNum = CL(42);
            pb->ioParam.ioRefNum = CW(fcbx.refNum);
            return noErr;
        }
        else
            return paramErr;    // TODO: what's the correct error?
    }
    catch(const OSErrorException& err)
    {
        return err.code;
    }
}
OSErr LocalVolume::PBHOpenRF(HParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBHGetFInfo(HParmBlkPtr pb, BOOLEAN async)
{
    try
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
            return noErr;
        }
        else
            return paramErr;
    }
    catch(const OSErrorException& err)
    {
        return err.code;
    }
}
OSErr LocalVolume::PBGetFInfo(ParmBlkPtr pb, BOOLEAN async)
{
try
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
            return noErr;
        }
        else
            return paramErr;
    }
    catch(const OSErrorException& err)
    {
        return err.code;
    }    return paramErr;
}
OSErr LocalVolume::PBSetFInfo(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBHSetFInfo(HParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}

OSErr LocalVolume::PBGetCatInfo(CInfoPBPtr pb, BOOLEAN async)
{
    try
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

        return noErr;
    }
    catch(const OSErrorException& err)
    {
        return err.code;
    }
}
OSErr LocalVolume::PBSetCatInfo(CInfoPBPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBCatMove(CMovePBPtr pb, BOOLEAN async)
{
    return paramErr;
}

OSErr LocalVolume::PBOpen(ParmBlkPtr pb, BOOLEAN async)
{
    try
    {  
        ItemPtr item = resolve(MR(pb->ioParam.ioNamePtr), CW(pb->ioParam.ioVRefNum), 0);
        if(FileItem *fileItem = dynamic_cast<FileItem*>(item.get()))
        {
            FCBExtension& fcbx = openFCBX();
            fcbx.access = fileItem->open();
            fcbx.fcb->fcbFlNum = CL(42);
            pb->ioParam.ioRefNum = CW(fcbx.refNum);
            return noErr;
        }
        else
            return paramErr;    // TODO: what's the correct error?
    }
    catch(const OSErrorException& err)
    {
        return err.code;
    }
}
OSErr LocalVolume::PBOpenRF(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBCreate(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBDelete(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBOpenWD(WDPBPtr pb, BOOLEAN async)
{
    ItemPtr item = resolve(MR(pb->ioNamePtr), CW(pb->ioVRefNum), CL(pb->ioWDDirID));
    std::cout << "PBOpenWD: " << item->path() << std::endl;

    long dirID;
    if(DirectoryItem *dirItem = dynamic_cast<DirectoryItem*>(item.get()))
        dirID = dirItem->dirID();
    else
        dirID = item->parID();

    return ROMlib_mkwd(pb, &vcb, dirID, Cx(pb->ioWDProcID));
}
OSErr LocalVolume::PBSetFLock(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBHSetFLock(HParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBRstFLock(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBHRstFLock(HParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBSetFVers(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBRename(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBSetVInfo(HParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBFlushVol(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBUnmountVol(ParmBlkPtr pb)
{
    return paramErr;
}
OSErr LocalVolume::PBEject(ParmBlkPtr pb)
{
    return paramErr;
}
OSErr LocalVolume::PBOffLine(ParmBlkPtr pb)
{
    return paramErr;
}
OSErr LocalVolume::PBRead(ParmBlkPtr pb, BOOLEAN async)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    size_t n = fcbx.access->read(CL(fcbx.fcb->fcbCrPs), MR(pb->ioParam.ioBuffer), CL(pb->ioParam.ioReqCount));
    pb->ioParam.ioActCount = CL(n);
    fcbx.fcb->fcbCrPs = CL( CL(fcbx.fcb->fcbCrPs) + n );
    return noErr;
}
OSErr LocalVolume::PBWrite(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBClose(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBAllocate(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBAllocContig(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBSetEOF(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBLockRange(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBUnlockRange(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBGetFPos(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBSetFPos(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBGetEOF(ParmBlkPtr pb, BOOLEAN async)
{
    auto& fcbx = getFCBX(CW(pb->ioParam.ioRefNum));
    size_t n = fcbx.access->getEOF();
    //read(CL(fcbx.fcb->fcbCrPs), MR(pb->ioParam.ioBuffer), CL(pb->ioParam.ioReqCount));
    pb->ioParam.ioMisc = CL(n);
    return noErr;
}
OSErr LocalVolume::PBFlushFile(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
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