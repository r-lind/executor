#pragma once

#define MODULE_NAME mpw
#include <base/api-module.h>

namespace Executor
{
    RAW_68K_FUNCTION(stdioQuit);
    RAW_68K_FUNCTION(stdioAccess);
    RAW_68K_FUNCTION(stdioClose);
    RAW_68K_FUNCTION(stdioRead);
    RAW_68K_FUNCTION(stdioWrite);
    RAW_68K_FUNCTION(stdioIoctl);
}

namespace Executor::mpw
{
    
    void SetupTool();
}
