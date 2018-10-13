#include <string>
#include "rsys/filesystem.h"

namespace Executor
{
    extern char ROMlib_startdir[MAXPATHLEN];
    #if defined(WIN32)
    extern char ROMlib_start_drive;
    #endif
    extern std::string ROMlib_ConfigurationFolder;
    extern fs::path ROMlib_DirectoryMap;
    extern std::string ROMlib_ScreenDumpFile;
    extern std::string ROMlib_appname;
}