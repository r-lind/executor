#include "basilisk.h"
#include "plain.h"

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

void BasiliskHandler::createFile(const fs::path& parentPath, mac_string_view name)
{
    fs::path fn = toUnicodeFilename(name);

    fs::ofstream(parentPath / fn);

    fs::create_directory(parentPath / ".rsrc");
    fs::create_directory(parentPath / ".finf");

    fs::ofstream(parentPath / ".rsrc" / fn, std::ios::binary);
    FInfo info = {0};
    fs::ofstream(parentPath / ".finf" / fn, std::ios::binary).write((char*)&info, sizeof(info));
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
    fs::ifstream(finf, std::ios::binary).read((char*)&info, sizeof(info));

    return info;
}

void BasiliskFileItem::setFInfo(FInfo info)
{
    fs::path finf = path().parent_path() / ".finf" / path().filename();

    fs::ofstream(finf, std::ios::binary).write((char*)&info, sizeof(info));
}

void BasiliskFileItem::deleteItem()
{
    fs::remove(path());
    fs::path rsrc = path().parent_path() / ".rsrc" / path().filename();
    fs::path finf = path().parent_path() / ".finf" / path().filename();
    boost::system::error_code ec;
    fs::remove(rsrc, ec);
    fs::remove(finf, ec);
}

void BasiliskFileItem::renameItem(mac_string_view newName)
{
    fs::path newFN = toUnicodeFilename(newName);
    fs::path newPath = path().parent_path() / newFN;

    fs::path rsrcOld = path().parent_path() / ".rsrc" / path().filename();
    fs::path finfOld = path().parent_path() / ".finf" / path().filename();
    fs::path rsrcNew = path().parent_path() / ".rsrc" / newFN;
    fs::path finfNew = path().parent_path() / ".finf" / newFN;

    fs::rename(path(), newPath);

    boost::system::error_code ec;
    fs::rename(rsrcOld, rsrcNew, ec);
    fs::rename(finfOld, finfNew, ec);

    path_ = std::move(newPath);
    name_ = newName;
}

void BasiliskFileItem::moveItem(const fs::path& newParent)
{
    fs::path newPath = newParent / path().filename();

    fs::path rsrcOld = path().parent_path() / ".rsrc" / path().filename();
    fs::path finfOld = path().parent_path() / ".finf" / path().filename();
    fs::path rsrcNew = newParent / ".rsrc" / path().filename();
    fs::path finfNew = newParent / ".finf" / path().filename();

    fs::create_directory(newParent / ".rsrc");
    fs::create_directory(newParent / ".finf");

    fs::rename(path(), newPath);

    boost::system::error_code ec;
    fs::rename(rsrcOld, rsrcNew, ec);
    fs::rename(finfOld, finfNew, ec);

    path_ = std::move(newPath);

}
