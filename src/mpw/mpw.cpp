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
#include <MemoryMgr.h>

#include <unistd.h>

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
    
    int nativeFd = -1;
    short macRefNum = 0;

    MPWFDEntry(int fd)
        : nativeFd(fd)
    {
    }



    ~MPWFDEntry()
    {
        if (refCount <= 1 && nativeFd != -1)
            close(nativeFd);
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
    //printf("quitting something\n");
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
    //printf("close(cookie=%d)\n", (int)file->cookie);
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
    //printf("mpwRead(cookie=%d, %d)\n", (int) file->cookie, (int) file->count);
    //fflush(stdout);

    ssize_t size = 0;

    if(auto *e = getEntry(file->cookie))
    {
        //if (e->stdfile)
        //    size = fread(file->buffer, 1, file->count, e->stdfile);

        if (e->nativeFd != -1)
        {
            do
            {
                errno = 0;
                size = read(e->nativeFd ,file->buffer, file->count);
            } while (errno == EINTR);
        }
    }
    
    //printf("--> %d (errno == %d)\n", (int) size, (int) errno);
    //fflush(stdout);

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
    //printf("writing something\n");
    //fflush(stdout);

    //printf("writing %d bytes from %p\n", (int) file->count, (void*) file->buffer);

    ssize_t size = 0;

    if(auto *e = getEntry(file->cookie))
    {
        if (e->nativeFd != -1)
        {
            const char *buf = (const char*)file->buffer;

            std::string temp(buf, buf + file->count);
            for(char& c : temp)
                if (c == '\r')
                    c = '\n';

            //fprintf(e->stdfile,"{");
            //fflush(e->stdfile);
            //size = fwrite(temp.data(), 1, file->count, e->stdfile);
            size = write(e->nativeFd, temp.data(), file->count);
            //for (int i = 0; i < file->count; i++)
            //    fputc(buf[i], e->stdfile);
            //fprintf(e->stdfile,"}");
            //fflush(e->stdfile);
            //printf("written: %ld\n", size);
        }
        else
            printf("no stdfile.\n");
    }
    else
        printf("no entry\n");
    fflush(stdout);

    if (size < 0)
    {
        file->err = -99;
        return kEIO;
    }
    file->count = file->count - size;
    file->err = 0;

    //printf("\n\nwriting %d bytes from %p done\n", (int) file->count, (void*) file->buffer);

    return 0;
}

static int C_mpwIOCtl(MPWFile *file, int cmd, uint32_t param)
{
    //printf("mpwIOCtl(cookie = %d, %x, %x)\n", (int)file->cookie, (int)cmd, (int)param);
    //fflush(stdout);

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

        case kFIOINTERACTIVE:
            if (auto *e = getEntry(file->cookie))
            {
                file->err = 0;
                return 0;
            }
            break;


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

std::vector<std::string> argsHack;
extern char **environ;

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
    pgm2.argc = argsHack.size();

    GUEST<char*>* mpwArgs = (GUEST<char*>*) NewPtr((argsHack.size() + 1) * 4);
    pgm2.argv = mpwArgs;
    
    for(const std::string& arg : argsHack)
    {
        Ptr p = NewPtr(arg.size() + 1);
        *mpwArgs++ = p;
        memcpy(p, arg.c_str(), arg.size() + 1);
        std::cout << "argument: " << arg << std::endl;
    }
    *mpwArgs++ = nullptr;

    int nEnv = 0;
    for (char** e = environ; *e; ++e)
        nEnv++;

    GUEST<char*>* mpwEnv = (GUEST<char*>*) NewPtr((nEnv + 1) * 4);
    pgm2.envp = mpwEnv;

    for (char** e = environ; *e; ++e)
    {
        int len = strlen(*e);
        Ptr p = NewPtr(len + 1);
        *mpwEnv++ = p;
        memcpy(p, *e, len + 1);
        if (char *q = strchr(p, '='))
            *q = '\0';
        //std::cout << "environment: " << *e << std::endl;
    }
    *mpwEnv = nullptr;


    pgm2.exitCode = 0;
    pgm2.tableSize = 3;

    fdEntries.emplace_back(std::make_unique<MPWFDEntry>(0));
    fdEntries.emplace_back(std::make_unique<MPWFDEntry>(1));
    fdEntries.emplace_back(std::make_unique<MPWFDEntry>(2));
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
