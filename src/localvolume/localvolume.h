#pragma once

#include <rsys/volume.h>

#include <FileMgr.h>
#include <MemoryMgr.h>
#include "macstrings.h"
#include "item.h"

#include "filesystem.h"

#include <map>
#include <unordered_map>
#include <vector>

namespace Executor
{


class OpenFile;
class MetaDataHandler;

enum class Fork
{
    data,
    resource
};

class LocalVolume : public Volume
{
    fs::path root;
    long nextCNID = 3;
    std::map<fs::path, CNID> pathToId;
    std::unordered_map<CNID, ItemPtr> items; 
    std::vector<std::unique_ptr<MetaDataHandler>> handlers;

    std::shared_ptr<DirectoryItem> resolve(short vRef, long dirID);
    ItemPtr resolve(mac_string_view name, short vRef, long dirID);
    ItemPtr resolveForInfo(mac_string_view name, short vRef, long dirID, short index, bool includeDirectories);
    ItemPtr resolveRelative(const std::shared_ptr<DirectoryItem>& base, mac_string_view name);

    struct FCBExtension;
    std::vector<FCBExtension> fcbExtensions;

    FCBExtension& getFCBX(short refNum);
    FCBExtension& openFCBX();

    struct NonexistentFile
    {
        std::shared_ptr<DirectoryItem> parent;
        mac_string_view name;
    };

    NonexistentFile resolveForCreate(mac_string_view name, short vRefNum, CNID dirID);

    void createCommon(NonexistentFile file);
    void setFInfoCommon(Item& item, ParmBlkPtr pb);
    void openCommon(GUEST<short>& refNum, ItemPtr item, Fork fork, int8_t permission);
    void deleteCommon(ItemPtr item);
    void renameCommon(ItemPtr item, mac_string_view newName);
    void setFPosCommon(ParmBlkPtr pb, bool checkEOF);

    enum class InfoKind
    {
        FInfo,
        HFInfo,
        CatInfo
    };

    void getInfoCommon(CInfoPBPtr pb, InfoKind infoKind);

public:
    ItemPtr getItemForDirEntry(const DirectoryItem& parent, const fs::directory_entry& path);
    CNID newCNID();


    mac_string getVolumeName() const;

    LocalVolume(VCB& vcb, fs::path root);

    virtual void PBGetCatInfo(CInfoPBPtr pb) override;
    virtual void PBGetFInfo(ParmBlkPtr pb) override;
    virtual void PBHGetFInfo(HParmBlkPtr pb) override;

    virtual void PBSetCatInfo(CInfoPBPtr pb) override;
    virtual void PBSetFInfo(ParmBlkPtr pb) override;
    virtual void PBHSetFInfo(HParmBlkPtr pb) override;
    virtual void PBSetVInfo(HParmBlkPtr pb) override;

    virtual void PBDirCreate(HParmBlkPtr pb) override;
    virtual void PBCreate(ParmBlkPtr pb) override;
    virtual void PBHCreate(HParmBlkPtr pb) override;
    virtual void PBDelete(ParmBlkPtr pb) override;
    virtual void PBHDelete(HParmBlkPtr pb) override;
    virtual void PBRename(ParmBlkPtr pb) override;
    virtual void PBHRename(HParmBlkPtr pb) override;
    virtual void PBCatMove(CMovePBPtr pb) override;

    virtual void PBOpenWD(WDPBPtr pb) override;
    virtual void PBOpenDF(ParmBlkPtr pb) override;
    virtual void PBOpenRF(ParmBlkPtr pb) override;
    virtual void PBHOpenDF(HParmBlkPtr pb) override;
    virtual void PBHOpenRF(HParmBlkPtr pb) override;

    virtual void PBSetFLock(ParmBlkPtr pb) override;
    virtual void PBHSetFLock(HParmBlkPtr pb) override;
    virtual void PBRstFLock(ParmBlkPtr pb) override;
    virtual void PBHRstFLock(HParmBlkPtr pb) override;

    virtual void PBClose(ParmBlkPtr pb) override;
    virtual void PBRead(ParmBlkPtr pb) override;
    virtual void PBWrite(ParmBlkPtr pb) override;
    virtual void PBGetFPos(ParmBlkPtr pb) override;
    virtual void PBSetFPos(ParmBlkPtr pb) override;
    virtual void PBGetEOF(ParmBlkPtr pb) override;
    virtual void PBFlushFile(ParmBlkPtr pb) override;

    virtual void PBFlushVol(ParmBlkPtr pb) override;
    virtual void PBUnmountVol(ParmBlkPtr pb) override;
    virtual void PBEject(ParmBlkPtr pb) override;
    virtual void PBOffLine(ParmBlkPtr pb) override;

    virtual void PBAllocate(ParmBlkPtr pb) override;
    virtual void PBAllocContig(ParmBlkPtr pb) override;
    virtual void PBSetEOF(ParmBlkPtr pb) override;
    virtual void PBLockRange(ParmBlkPtr pb) override;
    virtual void PBUnlockRange(ParmBlkPtr pb) override;

    virtual void PBSetFVers(ParmBlkPtr pb) override;
};

class MetaDataHandler
{
public:
    virtual ~MetaDataHandler() = default;

    virtual bool isHidden(const fs::directory_entry& e) { return false; }
    virtual ItemPtr handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e) = 0;
};

class DirectoryHandler : public MetaDataHandler
{
    LocalVolume& volume;
public:
    DirectoryHandler(LocalVolume& vol) : volume(vol) {}
    virtual ItemPtr handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e);
};

class ExtensionHandler : public MetaDataHandler
{
    LocalVolume& volume;
public:
    ExtensionHandler(LocalVolume& vol) : volume(vol) {}
    virtual ItemPtr handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e);
};

class BasiliskHandler : public MetaDataHandler
{
    LocalVolume& volume;
public:
    BasiliskHandler(LocalVolume& vol) : volume(vol) {}
    virtual bool isHidden(const fs::directory_entry& e);
    virtual ItemPtr handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e);
};

class AppleDoubleHandler : public MetaDataHandler
{
    LocalVolume& volume;
public:
    AppleDoubleHandler(LocalVolume& vol) : volume(vol) {}
    virtual bool isHidden(const fs::directory_entry& e);
    virtual ItemPtr handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e);
};

class MacHandler : public MetaDataHandler
{
    virtual ItemPtr handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e);
};

class OpenFile
{
public:
    virtual ~OpenFile() = default;

    virtual size_t getEOF() = 0;
    virtual void setEOF(size_t sz) { throw OSErrorException(wrPermErr); }
    virtual size_t read(size_t offset, void *p, size_t n) = 0;
    virtual size_t write(size_t offset, void *p, size_t n) { throw OSErrorException(wrPermErr); }
};

class PlainDataFork : public OpenFile
{
    //fs::fstream stream;
    int fd;
    fs::path path_;
public:
    PlainDataFork(fs::path path);
    ~PlainDataFork();

    virtual size_t getEOF() override;
    virtual void setEOF(size_t sz) override;
    virtual size_t read(size_t offset, void *p, size_t n) override;
    virtual size_t write(size_t offset, void *p, size_t n) override;
};

class EmptyFork : public OpenFile
{
public:
    virtual size_t getEOF() override { return 0; }
    virtual size_t read(size_t offset, void *p, size_t n) override { return 0; }
};

class AppleSingleDoubleFork : public OpenFile
{
    struct EntryDescriptor
    {
        GUEST_STRUCT;
        GUEST<uint32_t> entryId;
        GUEST<uint32_t> offset;
        GUEST<uint32_t> length;
    };
    std::unique_ptr<OpenFile> file;
    EntryDescriptor desc;
    uint32_t descOffset;
public:
    AppleSingleDoubleFork(std::unique_ptr<OpenFile> file, int entry);
    ~AppleSingleDoubleFork();

    virtual size_t getEOF() override;
    virtual void setEOF(size_t sz) override;
    virtual size_t read(size_t offset, void *p, size_t n) override;
    virtual size_t write(size_t offset, void *p, size_t n) override;
};


} /* namespace Executor */
