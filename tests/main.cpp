#include "gtest/gtest.h"

extern "C"
int mkdir(const char*, mode_t)
{
    return -1;
}

extern "C"
char *getcwd(char *buf, size_t size)
{
    buf[0] = '/';
    buf[1] = 0;
    return buf;
} 

int main(int argc, char **argv)
{
    freopen("out", "w", stdout);
    testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    //fgetc(stdin);
    return result;
}
