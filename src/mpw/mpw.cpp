#define INSTANTIATE_TRAPS_mpw

#include <mpw/mpw.h>
#include <ExMacTypes.h>
#include <MPW.h>

#include <prefs/prefs.h>

#include <QuickDraw.h>
#include <WindowMgr.h>
#include <FontMgr.h>
#include <MenuMgr.h>
#include <DialogMgr.h>

using namespace Executor;
using namespace Executor::mpw;

namespace
{
PgmInfo2 pgm2;
PgmInfo1 pgm1;

MPWFile files[3] = {};

devtable devTable;

struct MPWFDEntry
{
    int refCount = 1;
    
    FILE *stdfile = nullptr;
    short macRefNum = 0;

    MPWFDEntry(FILE* f)
        : stdfile(f)
    {
    }



    ~MPWFDEntry()
    {
        if (refCount <= 1 && stdfile)
            fclose(stdfile);
    }
};

std::vector<std::unique_ptr<MPWFDEntry>> fdEntries;
}

static MPWFDEntry* getEntry(int fd)
{
    if(fd >= 0 && fd < fdEntries.size() && fdEntries[fd])
        return fdEntries[fd].get();
    else
        return nullptr;
}

static void C_mpwQuit()
{
    printf("quitting something\n");
    fflush(stdout);
    exit(0);
}

static int C_mpwAccess(char *name, int op, uint32_t param)
{
    printf("mpwAccess(%s, %x, %x)\n", name, op, param);
    fflush(stdout);
    return kEINVAL;
}

static int C_mpwClose(MPWFile* file)
{
    printf("close(cookie=%d)\n", (int)file->cookie);
    fflush(stdout);

    if (auto *e = getEntry(file->cookie))
    {
        if (--e->refCount <= 0)
            fdEntries[file->cookie] = {};
        return 0;
    }
    return kEINVAL;
}

static int C_mpwRead(MPWFile* file)
{
    printf("mpwRead(cookie=%d, %d)\n", (int) file->cookie, (int) file->count);
    fflush(stdout);

    ssize_t size = 0;

    if(auto *e = getEntry(file->cookie))
    {
        if (e->stdfile)
            size = fread(file->buffer, file->count, 1, e->stdfile);
    }
    
    printf("--> %d\n", (int) size);
    fflush(stdout);

    if (size < 0)
    {
        file->err = -99;
        return kEIO;
    }
    file->count -= size;
    file->err = 0;

    return 0;
}

static int C_mpwWrite(MPWFile* file)
{
    printf("writing something\n");
    fflush(stdout);

    printf("writing %d bytes from %p\n", (int) file->count, (void*) file->buffer);

    ssize_t size = 0;

    if(auto *e = getEntry(file->cookie))
    {
        if (e->stdfile)
            size = fwrite(file->buffer, file->count, 1, e->stdfile);
    }
    fflush(stdout);

    if (size < 0)
    {
        file->err = -99;
        return kEIO;
    }
    file->count -= size;
    file->err = 0;

    printf("\n\nwriting %d bytes from %p done\n", (int) file->count, (void*) file->buffer);

    return 0;
}

static int C_mpwIOCtl(MPWFile *file, int cmd, uint32_t param)
{
    printf("mpwIOCtl(cookie = %d, %x, %x)\n", (int)file->cookie, (int)cmd, (int)param);
    fflush(stdout);

    switch(cmd)
    {
        //case k
        case kFIODUPFD:
            if (auto *e = getEntry(file->cookie))
            {
                ++e->refCount;
                file->err = 0;
                return 0;
            }
            break;
/*
        case kFIOINTERACTIVE:
            if (auto *e = getEntry(file->cookie))
            {
                
            }
            break;
*/

    }

    return mpw::kEINVAL;
}

CCALL_FUNCTION_PTR(mpwQuit);
CCALL_FUNCTION_PTR(mpwAccess);
CCALL_FUNCTION_PTR(mpwClose);
CCALL_FUNCTION_PTR(mpwRead);
CCALL_FUNCTION_PTR(mpwWrite);
CCALL_FUNCTION_PTR(mpwIOCtl);

QDGlobals mpwQD;

void mpw::SetupTool()
{
    ROMlib_nowarn32 = true;
    InitGraf(&mpwQD.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(nullptr);
    
    LM(MacPgm) = &pgm1;
    pgm1.magic = "MPGM"_4;
    pgm1.pgmInfo2 = &pgm2;

    pgm2.magic2 = 0x5348;
    pgm2.argc = 0;
    pgm2.argv = nullptr;
    pgm2.envp = nullptr;
    pgm2.exitCode = 0;
    pgm2.tableSize = 3;

    fdEntries.emplace_back(std::make_unique<MPWFDEntry>(stdin));
    fdEntries.emplace_back(std::make_unique<MPWFDEntry>(stdout));
    fdEntries.emplace_back(std::make_unique<MPWFDEntry>(stderr));
    pgm2.ioptr = files;
    for (int i = 0; i < 3; i++)
    {
        files[i].cookie = i;
        files[i].functions = &devTable.table;
    }
    files[0].flags = 1;
    files[1].flags = 2;
    files[1].flags = 2;
    pgm2.devptr = &devTable;

    devTable.magic = "FSYS"_4;
    devTable.table.quit = &mpwQuit;
    devTable.table.access = &mpwAccess;
    devTable.table.close = &mpwClose;
    devTable.table.read = &mpwRead;
    devTable.table.write = &mpwWrite;
    devTable.table.ioctl = &mpwIOCtl;
}
