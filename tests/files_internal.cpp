#include "gtest/gtest.h"

#include <file/file.h>
#include <rsys/macstrings.h>

using namespace Executor;

TEST(FilesInternal, pathToFSSpec)
{
    auto path = fs::current_path();

    std::string macpath = "vol" + path.string();
    for(char& c : macpath)
    {
        if(c == ':')
            c = '/';
        else if(c == '/')
            c = ':';
    }

    auto native = nativePathToFSSpec(path);
    auto mac = cmdlinePathToFSSpec(macpath);

    ASSERT_EQ(true, native.has_value());
    ASSERT_EQ(true, mac.has_value());

    EXPECT_EQ(mac->vRefNum, native->vRefNum);
    EXPECT_EQ(mac->parID, native->parID);
    EXPECT_EQ(toUnicode(mac->name), toUnicode(native->name));
}
