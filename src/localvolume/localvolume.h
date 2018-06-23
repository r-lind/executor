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

using DirID = long;

class LocalVolume : public Volume
{
    fs::path root;
    long nextDirectory = 3;
    std::map<fs::path, DirID> pathToId;

    std::unordered_map<DirID, std::shared_ptr<DirectoryItem>> directories_; 

    std::shared_ptr<DirectoryItem> resolve(short vRef, long dirID);
    ItemPtr resolve(mac_string_view name, short vRef, long dirID);
    ItemPtr resolve(short vRef, long dirID, short index);
    ItemPtr resolve(mac_string_view name, short vRef, long dirID, short index);

    struct FCBExtension;
    std::vector<FCBExtension> fcbExtensions;

    FCBExtension& getFCBX(short refNum);
    FCBExtension& openFCBX();
public:
    std::shared_ptr<DirectoryItem> lookupDirectory(const DirectoryItem& parent, const fs::path& path);

    long lookupDirID(const fs::path& path);
    std::vector<std::unique_ptr<MetaDataHandler>> handlers;


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
    virtual void PBHOpen(HParmBlkPtr pb) override;
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
    fs::fstream stream;
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


} /* namespace Executor */