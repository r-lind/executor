#include <string>
#include <rsys/filesystem.h>

namespace Executor
{
    extern char ROMlib_startdir[MAXPATHLEN];
    extern std::string ROMlib_ConfigurationFolder;
    extern fs::path ROMlib_DirectoryMap;
    extern std::string ROMlib_ScreenDumpFile;
    extern std::string ROMlib_appname;
}