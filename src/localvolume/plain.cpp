#include "plain.h"
#include "host-os-config.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <rsys/macros.h>

using namespace Executor;

#if 1
PlainDataFork::PlainDataFork(fs::path path)
{
    fd = open(path.string().c_str(), O_RDWR | O_BINARY, 0644);
    //std::cout << "ACCESSING FILE: " << fd << " = " << path << std::endl;
}

PlainDataFork::PlainDataFork(fs::path path, create_t)
{
    fd = open(path.string().c_str(), O_RDWR | O_CREAT | O_BINARY, 0644);
    //std::cout << "CREATING FILE: " << fd << " = " << path << std::endl;
}

PlainDataFork::~PlainDataFork()
{
    close(fd);
    //std::cout << "CLOSING FILE: " << fd << std::endl;
}

size_t PlainDataFork::getEOF()
{
    return lseek(fd, 0,SEEK_END);
}
void PlainDataFork::setEOF(size_t sz)
{
    ftruncate(fd, sz);
}

size_t PlainDataFork::read(size_t offset, void *p, size_t n)
{
    lseek(fd, offset, SEEK_SET);
    ssize_t done;
    
    done = ::read(fd, p, n);
    
    return done;
}
size_t PlainDataFork::write(size_t offset, void *p, size_t n)
{
    lseek(fd, offset, SEEK_SET);
    ssize_t done;
    
    done = ::write(fd, p, n);
    
    if(done != n)
    {
        std::cout << "short write: " << done << " of " << n << " ; errno = " << errno << std::endl;
    }
    return done;
}
#else
PlainDataFork::PlainDataFork(fs::path path)
    : stream(path, std::ios::binary)
{
    std::cout << "ACCESSING FILE: " << path << std::endl;
}
PlainDataFork::~PlainDataFork()
{
}

size_t PlainDataFork::getEOF()
{
    stream.seekg(0, std::ios::end);
    return stream.tellg();
}
void PlainDataFork::setEOF(size_t sz)
{
    
}
size_t PlainDataFork::read(size_t offset, void *p, size_t n)
{
    stream.seekg(offset);
    stream.read((char*)p, n);
    stream.clear(); // ?
    return stream.gcount();
}
size_t PlainDataFork::write(size_t offset, void *p, size_t n)
{
    stream.seekp(offset);
    stream.write((char*)p, n);
    return n;
}
#endif


std::unique_ptr<OpenFile> PlainFileItem::open()
{
    return std::make_unique<PlainDataFork>(path_);
}
std::unique_ptr<OpenFile> PlainFileItem::openRF()
{
    return std::make_unique<EmptyFork>();
}

ItemInfo PlainFileItem::getInfo()
{
    ItemInfo info = {0};
    info.file.info = {
        TICKX("TEXT"),
        TICKX("ttxt"),
        CWC(0), // fdFlags
        { CWC(0), CWC(0) }, // fdLocation
        CWC(0) // fdFldr
    };
    return info;
}

void PlainFileItem::setInfo(ItemInfo info)
{
    if(info.file.info.fdType != TICKX("TEXT"))
        throw UpgradeRequiredException();
}
