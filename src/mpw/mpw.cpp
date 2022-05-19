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

RAW_68K_IMPLEMENTATION(stdioQuit)
{
    printf("quitting something\n");
    fflush(stdout);
    return POPADDR();
}
RAW_68K_IMPLEMENTATION(stdioAccess)
{
    printf("accessig something\n");
    fflush(stdout);
    return POPADDR();
}
RAW_68K_IMPLEMENTATION(stdioClose)
{
    printf("closing something\n");    
    fflush(stdout);
    return POPADDR();
}
RAW_68K_IMPLEMENTATION(stdioRead)
{
    printf("reading something\n");
    fflush(stdout);
    return POPADDR();
}

RAW_68K_IMPLEMENTATION(stdioWrite)
{
    printf("writing something\n");
    fflush(stdout);

    MPWFile *file = *ptr_from_longint<GUEST<GUEST<MPWFile*>*>>(EM_A7 + 4);

    printf("writing %d bytes from %p\n", (int) file->count, (void*) file->buffer);
    fwrite(file->buffer, file->count, 1, stdout);
    fflush(stdout);

    printf("\n\nwriting %d bytes from %p done\n", (int) file->count, (void*) file->buffer);
    return POPADDR();
}

RAW_68K_IMPLEMENTATION(stdioIoctl)
{
    printf("controlling something\n");
    fflush(stdout);
    return POPADDR();
}

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
        files[i].cookie = 42;
        files[i].functions = &devTable.table;
    }
    pgm2.devptr = &devTable;

//    stdioFunctions.write = ProcPtr(&stub_stdioWrite);

    devTable.magic = "FSYS"_4;
    devTable.table.quit = ProcPtr(&stub_stdioQuit);
    devTable.table.access = ProcPtr(&stub_stdioAccess);
    devTable.table.close = ProcPtr(&stub_stdioClose);
    devTable.table.read = ProcPtr(&stub_stdioRead);
    devTable.table.write = ProcPtr(&stub_stdioWrite);
    devTable.table.ioctl = ProcPtr(&stub_stdioIoctl);

}