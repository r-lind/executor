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

class OSErrorException : public std::runtime_error
{
public:
    OSErr code;

    OSErrorException(OSErr err) : std::runtime_error("oserror"), code(err) {}
};

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

    virtual OSErr PBGetCatInfo(CInfoPBPtr pb, BOOLEAN async) override;
    virtual OSErr PBGetFInfo(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHGetFInfo(HParmBlkPtr pb, BOOLEAN async) override;

    virtual OSErr PBSetCatInfo(CInfoPBPtr pb, BOOLEAN async) override;
    virtual OSErr PBSetFInfo(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHSetFInfo(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBSetVInfo(HParmBlkPtr pb, BOOLEAN async) override;

    virtual OSErr PBDirCreate(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBCreate(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHCreate(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBDelete(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHDelete(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBRename(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHRename(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBCatMove(CMovePBPtr pb, BOOLEAN async) override;

    virtual OSErr PBOpenWD(WDPBPtr pb, BOOLEAN async) override;
    virtual OSErr PBOpen(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBOpenRF(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHOpen(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHOpenRF(HParmBlkPtr pb, BOOLEAN async) override;

    virtual OSErr PBSetFLock(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHSetFLock(HParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBRstFLock(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBHRstFLock(HParmBlkPtr pb, BOOLEAN async) override;

    virtual OSErr PBClose(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBRead(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBWrite(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBGetFPos(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBSetFPos(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBGetEOF(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBFlushFile(ParmBlkPtr pb, BOOLEAN async) override;

    virtual OSErr PBFlushVol(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBUnmountVol(ParmBlkPtr pb) override;
    virtual OSErr PBEject(ParmBlkPtr pb) override;
    virtual OSErr PBOffLine(ParmBlkPtr pb) override;

    virtual OSErr PBAllocate(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBAllocContig(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBSetEOF(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBLockRange(ParmBlkPtr pb, BOOLEAN async) override;
    virtual OSErr PBUnlockRange(ParmBlkPtr pb, BOOLEAN async) override;

    virtual OSErr PBSetFVers(ParmBlkPtr pb, BOOLEAN async) override;
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


class OpenFile
{
public:
    virtual ~OpenFile() = default;

    virtual size_t getEOF() = 0;
    virtual void setEOF(size_t sz) = 0;
    virtual size_t read(size_t offset, void *p, size_t n) = 0;
    virtual size_t write(size_t offset, void *p, size_t n) = 0;
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

} /* namespace Executor */