#include <rsys/builtinlibs.h>
#include <rsys/cfm.h>
#include <rsys/pef.h>
#include <unordered_map>
#include <vector>
#include <string>

using namespace Executor;

namespace
{
    std::unordered_map<std::string, std::vector<map_entry_t>> builtins;
}

void builtinlibs::addPPCEntrypoint(const char *library, const char *function, void *ppc_code)
{
    builtins[library].push_back({function, ppc_code});
}

ConnectionID builtinlibs::getBuiltinLib(Str255 libname)
{
    std::string name(libname + 1, libname + 1 + libname[0]);
    auto it = builtins.find(name);
    if(it == builtins.end())
        return nullptr;
    else
    {
        ConnectionID cid = ROMlib_new_connection(1);
        cid->lihp = RM(ROMlib_build_pef_hash(it->second.data(), it->second.size()));
        cid->ref_count = CL(1);
        return cid;
    }
}
