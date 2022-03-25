/* Copyright 1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#include <base/common.h>
#include <Gestalt.h>
#include <commandline/parseopt.h>
#include <commandline/flags.h>
#include <prefs/prefs.h>
#include <rsys/version.h>
#include <rsys/paths.h>

#include <ctype.h>
#include <string.h>

using namespace Executor;
using namespace std;

/* Parse version e.g. "executor -system 7.0.2".  Omitted
 * digits will be zero, so "executor -system 7" is equivalent to
 * "executor -system 7.0.0".  Returns true on success, else false.
 */

bool Executor::parse_version(string vers, uint32_t *version_out)
{
    bool success_p;
    int major_version, minor_version, teeny_version;
    char *major_str, *minor_str, *teeny_str;
    char *temp_str, *system_str;
    char zero_str[] = "0";

    /* Copy the version to a temp string we can manipulate. */
    system_str = (char *)alloca(vers.length() + 1);
    strcpy(system_str, vers.c_str());

    major_str = system_str;

    temp_str = strchr(major_str, '.');
    if(temp_str)
    {
        *temp_str = 0;
        minor_str = &temp_str[1];
    }
    else
        minor_str = zero_str;

    temp_str = strchr(minor_str, '.');
    if(temp_str)
    {
        *temp_str = 0;
        teeny_str = &temp_str[1];
    }
    else
        teeny_str = zero_str;

    major_version = atoi(major_str);
    minor_version = atoi(minor_str);
    teeny_version = atoi(teeny_str);

    if(major_version <= 0 || major_version > 0xF
       || minor_version < 0 || minor_version > 0xF
       || teeny_version < 0 || teeny_version > 0xF)
        success_p = false;
    else
    {
        *version_out = CREATE_SYSTEM_VERSION(major_version, minor_version,
                                             teeny_version);
        success_p = true;
    }

    return success_p;
}
