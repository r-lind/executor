#include "localvolume.h"
#include "item.h"

using namespace Executor;

bool BasiliskHandler::isHidden(const fs::directory_entry& e)
{
    if(e.path().filename().string() == ".rsrc")
        return true;
    else if(e.path().filename().string() == ".finf")
        return true;
    return false;
}


ItemPtr BasiliskHandler::handleDirEntry(const DirectoryItem& parent, const fs::directory_entry& e)
{
    if(fs::is_regular_file(e.path()))
    {
        fs::path rsrc = e.path().parent_path() / ".rsrc" / e.path().filename();
        fs::path finf = e.path().parent_path() / ".finf" / e.path().filename();
        
        if(fs::is_regular_file(rsrc) || fs::is_regular_file(finf))
            return std::make_shared<BasiliskFileItem>(parent, e.path());
    }
    return nullptr;
}

std::unique_ptr<OpenFile> BasiliskFileItem::open()
{
    return std::make_unique<PlainDataFork>(path_);
}
std::unique_ptr<OpenFile> BasiliskFileItem::openRF()
{
    fs::path rsrc = path().parent_path() / ".rsrc" / path().filename();
    return std::make_unique<PlainDataFork>(rsrc);
}

FInfo BasiliskFileItem::getFInfo()
{
    fs::path finf = path().parent_path() / ".finf" / path().filename();

    FInfo info = {0};
    fs::ifstream(finf).read((char*)&info, sizeof(info));

    return info;
}

void BasiliskFileItem::setFInfo(FInfo info)
{
    fs::path finf = path().parent_path() / ".finf" / path().filename();

    fs::ofstream(finf).write((char*)&info, sizeof(info));
}

void BasiliskFileItem::deleteFile()
{
    fs::remove(path());
    fs::path rsrc = path().parent_path() / ".rsrc" / path().filename();
    fs::path finf = path().parent_path() / ".finf" / path().filename();
    fs::remove(rsrc);
    fs::remove(finf);
}
