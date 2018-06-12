#include <rsys/common.h>
#include <rsys/byteswap.h>
#include <rsys/volume.h>
#include <FileMgr.h>
#include <MemoryMgr.h>
#include <rsys/file.h>
#include <rsys/hfs.h>
#include <boost/filesystem.hpp>
#include <map>
#include <iostream>

using namespace Executor;
namespace fs = boost::filesystem;

class OSErrorException : public std::runtime_error
{
public:
    OSErr code;

    OSErrorException(OSErr err) : std::runtime_error("oserror"), code(err) {}
};


class LocalVolume : public Volume
{
    fs::path root;
    long nextDirectory = 3;
    std::map<long, fs::path> idToPath;
    std::map<fs::path, long> pathToId;

    class Item;

    long lookupDirID(const fs::path& path);

    Item resolve(short vRef, long dirID);
    Item resolve(const unsigned char* name, short vRef, long dirID);
    Item resolve(short vRef, long dirID, short index);
    Item resolve(const unsigned char* name, short vRef, long dirID, short index);
public:
    LocalVolume(VCB& vcb, fs::path root);

    virtual OSErr PBHRename(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHCreate(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBDirCreate(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHDelete(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHOpen(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHOpenRF(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBGetCatInfo(CInfoPBPtr pb, BOOLEAN async) override;
    virtual OSErr PBSetCatInfo(CInfoPBPtr pb, BOOLEAN async) override;
    virtual OSErr PBCatMove(CMovePBPtr pb, BOOLEAN async) override;
    virtual OSErr PBHGetFInfo(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBOpen(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBOpenRF(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBCreate(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBDelete(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBOpenWD(WDPBPtr pb, BOOLEAN async) override;
    virtual OSErr PBGetFInfo(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBSetFInfo(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHSetFInfo(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBSetFLock(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHSetFLock(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBRstFLock(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHRstFLock(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBSetFVers(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBRename(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBSetVInfo(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBFlushVol(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBUnmountVol(ParmBlkPtr pb) override;
    virtual OSErr PBEject(ParmBlkPtr pb) override;
    virtual OSErr PBOffLine(ParmBlkPtr pb) override;
    virtual OSErr PBRead(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBWrite(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBClose(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBAllocate(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBAllocContig(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBSetEOF(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBLockRange(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBUnlockRange(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBGetFPos(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBSetFPos(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBGetEOF(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBFlushFile(ParmBlkPtr pb, BOOLEAN async) override;
};

class LocalVolume::Item
{
    LocalVolume& volume_;
    fs::path path_;
    bool isDir_;
    long dirID_;
    long parID_;
public:
    Item(LocalVolume& vol, fs::path p);

    operator const fs::path& () const { return path_; }

    const fs::path& path() const { return path_; }

    long dirID() const { return dirID_; }
    long parID() const { return parID_; }
    bool isDirectory() const { return isDir_; }
};

LocalVolume::Item::Item(LocalVolume& vol, fs::path p)
    : volume_(vol), path_(std::move(p))
{
    if(fs::is_directory(path_))
    {
        isDir_ = true;
        dirID_ = volume_.lookupDirID(path_);
        if(dirID_ == 2)
            parID_ = 2;
        else
             parID_ = volume_.lookupDirID(path_.parent_path());
    }
    else
    {
        isDir_ = false;
        dirID_ = parID_ = volume_.lookupDirID(path_.parent_path());
    }
}

using mac_string_view = std::basic_string_view<unsigned char>;
mac_string_view PascalStringView(ConstStringPtr s)
{
    return mac_string_view(s+1, (size_t)s[0]);
}

LocalVolume::LocalVolume(VCB& vcb, fs::path root)
    : Volume(vcb), root(root)
{
    idToPath[2] = root;
    pathToId[root] = 2;
}

long LocalVolume::lookupDirID(const fs::path& path)
{
    auto [it, inserted] = pathToId.emplace(path, nextDirectory);
    if(inserted)
    {
        idToPath.emplace(nextDirectory, path);
        return nextDirectory++;
    }
    else
        return it->second;
}

LocalVolume::Item LocalVolume::resolve(short vRef, long dirID)
{
    if(dirID)
    {
        auto it = idToPath.find(dirID);
        if(it == idToPath.end())
            throw OSErrorException(fnfErr);
        return Item(*this, it->second); // TODO: don't throw away dirID
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
LocalVolume::Item LocalVolume::resolve(const unsigned char* name, short vRef, long dirID)
{
    if(!name)
        return resolve(vRef, dirID);
    // TODO: handle pathnames (should probably be done outside of LocalVolume)
    //       handle encoding
    //       handle case insensitivity?
    
    std::string cname(name + 1, name + 1 + name[0]);
    return Item(*this, resolve(nullptr, vRef, dirID, 0).path() / cname);
}
LocalVolume::Item LocalVolume::resolve(short vRef, long dirID, short index)
{
    for(const auto& e : fs::directory_iterator(resolve(vRef, dirID)))
    {
        if(!--index)
            return Item(*this, e);
    }
    throw OSErrorException(fnfErr);
}
LocalVolume::Item LocalVolume::resolve(const unsigned char* name, short vRef, long dirID, short index)
{
    if(index > 0)
        return resolve(vRef, dirID, index);
    else if(index == 0 && name)
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
OSErr LocalVolume::PBHOpen(HParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBHOpenRF(HParmBlkPtr pb, BOOLEAN async)
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
        Item item = resolve(inputName, 
            CW(pb->hFileInfo.ioVRefNum), CL(pb->hFileInfo.ioDirID), CW(pb->hFileInfo.ioFDirIndex));

        std::cout << item.path() << std::endl;
        if(StringPtr outputName = MR(pb->hFileInfo.ioNamePtr))
        {
            const std::string& str = item.path().filename().string();
            uint8_t n = str.size() > 255 ? 255 : str.size();
            memcpy(outputName+1, str.data(), n);
            outputName[0] = n;
        }
        pb->hFileInfo.ioFlAttrib = item.isDirectory() ? ATTRIB_ISADIR : 0;

        if(item.isDirectory())
        {
            pb->dirInfo.ioDrDirID = CL(item.dirID());
            pb->dirInfo.ioDrParID = CL(item.parID());

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
OSErr LocalVolume::PBHGetFInfo(HParmBlkPtr pb, BOOLEAN async)
{
    std::cout << "hgetFInfo\n" << std::endl;
    return paramErr;
}
OSErr LocalVolume::PBOpen(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
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
    Item item = resolve(MR(pb->ioNamePtr), CW(pb->ioVRefNum), CL(pb->ioWDDirID));
    std::cout << "PBOpenWD: " << item.path() << std::endl;
    return ROMlib_mkwd(pb, &vcb, item.dirID(), Cx(pb->ioWDProcID));
}
OSErr LocalVolume::PBGetFInfo(ParmBlkPtr pb, BOOLEAN async)
{
        std::cout << "PBGetFInfo: " << std::endl;
    return paramErr;
}
OSErr LocalVolume::PBSetFInfo(ParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
}
OSErr LocalVolume::PBHSetFInfo(HParmBlkPtr pb, BOOLEAN async)
{
    return paramErr;
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
    return paramErr;
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
    return paramErr;
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