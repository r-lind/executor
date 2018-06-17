#pragma once

#include "macstrings.h"
#include "filesystem.h"
#include <memory>

namespace Executor
{
class LocalVolume;

class Item
{
    LocalVolume& volume_;
    fs::path path_;
    mac_string name_;
    bool isDir_;
    long dirID_;
    long parID_;
public:
    Item(LocalVolume& vol, fs::path p);

    operator const fs::path& () const { return path_; }

    const fs::path& path() const { return path_; }

    long dirID() const { return dirID_; }
    long parID() const { return parID_; }
    bool isDirectory() const { return isDir_; }
    const mac_string& name() const { return name_; }
};

using ItemPtr = std::unique_ptr<Item>;

}