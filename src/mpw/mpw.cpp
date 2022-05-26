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

namespace Executor::mpw
{
PgmInfo2 pgm2;
PgmInfo1 pgm1;

MPWFile files[3] = {};

devtable devTable;
}

static void C_stdioQuit()
{
    printf("quitting something\n");
    fflush(stdout);
    exit(0);
}

static int C_stdioAccess(char *name, int op, uint32_t param)
{
    printf("accessing something\n");
    fflush(stdout);
    return -1;
}

static int C_stdioClose(MPWFile* file)
{
    printf("closing something\n");
    fflush(stdout);
    return -1;
}

static int C_stdioRead(MPWFile* file)
{
    printf("reading something\n");
    fflush(stdout);
    return -1;
}

static int C_stdioWrite(MPWFile* file)
{
    printf("writing something\n");
    fflush(stdout);

    printf("writing %d bytes from %p\n", (int) file->count, (void*) file->buffer);
    fwrite(file->buffer, file->count, 1, stdout);
    fflush(stdout);

    printf("\n\nwriting %d bytes from %p done\n", (int) file->count, (void*) file->buffer);

    return 0;
}

static int C_stdioIOCtl(int fd, int cmd, uint32_t param)
{
    printf("controlling something\n");
    fflush(stdout);
    return -1;
}

CCALL_FUNCTION_PTR(stdioQuit);
CCALL_FUNCTION_PTR(stdioAccess);
CCALL_FUNCTION_PTR(stdioClose);
CCALL_FUNCTION_PTR(stdioRead);
CCALL_FUNCTION_PTR(stdioWrite);
CCALL_FUNCTION_PTR(stdioIOCtl);

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
    devTable.table.quit = &stdioQuit;
    devTable.table.access = &stdioAccess;
    devTable.table.close = &stdioClose;
    devTable.table.read = &stdioRead;
    devTable.table.write = &stdioWrite;
    devTable.table.ioctl = &stdioIOCtl;
}
