#include "Filesystem.h"
#include "FilesystemFAT.h"

std::shared_ptr<IFileSystem> CreateFileSystem(const FileSystemType type)
{
    if (type == FILE_SYSTEM_TYPE_FAT) {
        return std::move(std::make_shared<FilesystemFAT>());
    } else {
        return nullptr;
    }
}
