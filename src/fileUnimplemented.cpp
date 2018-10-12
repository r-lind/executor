#include <FileMgr.h>
#include <rsys/error.h>
#include <rsys/file.h>

using namespace Executor;

OSErr Executor::PBHGetLogInInfo(HParmBlkPtr pb, BOOLEAN a)
{
    OSErr retval;

    warning_unimplemented(NULL_STRING);
    retval = paramErr;
    return retval;
}

OSErr Executor::PBHGetDirAccess(HParmBlkPtr pb, BOOLEAN a)
{
    OSErr retval;

    warning_unimplemented(NULL_STRING);
    retval = paramErr;
    return retval;
}

OSErr Executor::PBHCopyFile(HParmBlkPtr pb, BOOLEAN a)
{
    OSErr retval;

    warning_unimplemented(NULL_STRING);
    retval = paramErr;
    return retval;
}

OSErr Executor::PBHMapName(HParmBlkPtr pb, BOOLEAN a)
{
    OSErr retval;

    warning_unimplemented(NULL_STRING);
    retval = paramErr;
    return retval;
}

OSErr Executor::PBHMapID(HParmBlkPtr pb, BOOLEAN a)
{
    OSErr retval;

    warning_unimplemented(NULL_STRING);
    retval = paramErr;
    return retval;
}

OSErr Executor::PBHSetDirAccess(HParmBlkPtr pb, BOOLEAN a)
{
    OSErr retval;

    warning_unimplemented(NULL_STRING);
    retval = paramErr;
    return retval;
}

OSErr Executor::PBHMoveRename(HParmBlkPtr pb, BOOLEAN a)
{
    OSErr retval;

    warning_unimplemented(NULL_STRING);
    retval = paramErr;
    return retval;
}

OSErr Executor::PBExchangeFiles(ParmBlkPtr pb, BOOLEAN async)
{
    OSErr retval;

    warning_unimplemented(NULL_STRING);
    retval = paramErr;
    FAKEASYNC(pb, async, retval);
}

OSErr Executor::PBCatSearch(ParmBlkPtr pb, BOOLEAN async)
{
    OSErr retval;

    warning_unimplemented(NULL_STRING);
    retval = paramErr;
    FAKEASYNC(pb, async, retval);
}

OSErr Executor::PBCreateFileIDRef(ParmBlkPtr pb, BOOLEAN async)
{
    OSErr retval;

    warning_unimplemented(NULL_STRING);
    retval = paramErr;
    FAKEASYNC(pb, async, retval);
}

OSErr Executor::PBDeleteFileIDRef(ParmBlkPtr pb, BOOLEAN async)
{
    OSErr retval;

    warning_unimplemented(NULL_STRING);
    retval = paramErr;
    FAKEASYNC(pb, async, retval);
}

OSErr Executor::PBResolveFileIDRef(ParmBlkPtr pb, BOOLEAN async)
{
    OSErr retval;

    warning_unimplemented(NULL_STRING);
    retval = paramErr;
    FAKEASYNC(pb, async, retval);
}
