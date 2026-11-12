#include "FilesystemFAT.h"

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <ranges>
#include <string_view>

#define DISK_IS_FORMATED_CHECK() {                                      \
    if (m_diskIsNotFormated) {                                          \
        std::cout << "ERROR: YOU NEED FORMAT DISK FIRTS!!" << std::endl;\
        return;                                                         \
    }                                                                   \
}

namespace {
    bool CreateFileDisk(const std::string fileDisk) {
        std::ofstream outFile(fileDisk, std::ios::out | std::ios::binary);

        if (!outFile) {
            std::cout << "CANNOT CREATE FILE" << std::endl;
            return false;
        }
        outFile.close();
        return true;
    }

    bool ResizeFileDisk(const std::string fileDisk, const unsigned long long fileSize) {
        std::error_code ec;

        std::filesystem::resize_file(fileDisk, fileSize, ec);

        if (ec) {
            std::cout << "CANNOT CREATE FILE" << std::endl;
            return false;
        }

        return true;
    }

    unsigned long long GetFileSizeInBytes(std::string sizeText) {
        
        const std::string gigabytes[2] = { "GB", "gb" };
        const std::string megabytes[2] = { "MB", "mb" };
        const std::string kilobytes[2] = { "KB", "kb" };

        unsigned long long number = 0;

        try {
            number = static_cast<unsigned long long>(std::stoi(sizeText));
        }
        catch (...) {
            return 0;
        }

        for (size_t i = 0; i < 2; i++)
        {
            if (sizeText.find(gigabytes[i]) != std::string::npos) {
                return number * 1024 * 1024 * 1024;
            }
            if (sizeText.find(megabytes[i]) != std::string::npos) {
                return number * 1024 * 1024;
            }
            if (sizeText.find(kilobytes[i]) != std::string::npos) {
                return number * 1024;
            }
        }

        return 0; //we do not allow lower than kilobytes, so we prevent some padding/allocation and other issues
    }
}

FilesystemFAT::~FilesystemFAT()
{
    if (m_diskIsNotFormated) {
        if (m_fileDisk->is_open()) {
            m_fileDisk->close();
        }

        //removing bc it is not formated
        std::filesystem::remove(m_fileDiskPath);
    }
}

void FilesystemFAT::SetFileDisk(std::string fileDisk)
{
    m_fileDiskPath = fileDisk;

    if (std::filesystem::exists(m_fileDiskPath)) {
        std::uintmax_t size = std::filesystem::file_size(m_fileDiskPath);
        m_numberOfClusters = size / CLUSTER_SIZE;
    } else {
        CreateFileDisk(m_fileDiskPath);
        m_diskIsNotFormated = true;
    }

    m_fileDisk = std::make_unique<std::fstream>(m_fileDiskPath, std::ios::in | std::ios::out | std::ios::binary);
    if (!*m_fileDisk) {
        throw std::exception("UNKNOW ERROR :/");
    }
}

void FilesystemFAT::SetCommandHandle(std::shared_ptr<CommandHandle> commandHandle)
{
    m_commandHandle = commandHandle;
}

void FilesystemFAT::CreateFile(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    //This is for testing.

    if (argc != 1 && argc != 0) {
        std::cout << "NO PATH" << std::endl;
        return;
    }

    const std::string instruction = "pwd\nmkdir lolec\nls\ncd lolec\npwd\nls\nmkdir e\nlol xd\nlol q\nmkdir w\nls";

    const size_t size = argc == 1 ? 1024 : instruction.size() + 1;
    IndexFAT currentCluster = FAT_EOF;

    std::string path = argc == 1 ? args[0] : "./test.xd";

    if (!CreateDirectoryEntry(path, false, size, false, &currentCluster)) {
        return;
    }
    if (currentCluster == FAT_EOF) {
        std::cout << "UNKNOW ERROR" << std::endl;
    }

    size_t numberOfClustersNeeded = (size + CLUSTER_SIZE - 1) / CLUSTER_SIZE;

    std::array<unsigned char, 4> pattern = { '1', '2', '3', '4' };

    size_t count = 0;
    for (size_t x = 0; x < numberOfClustersNeeded; x++)
    {
        for (size_t i = 0; i < CLUSTER_SIZE; ++i) {
            if (argc == 1) {
                WriteClusterByte(currentCluster, i, pattern[i % pattern.size()]);
            } else {
                WriteClusterByte(currentCluster, i, instruction[i]);
            }

            count += 1;
            if (count >= size) {
                break;
            }
        }
        if (count >= size) {
            break;
        }

        currentCluster = ReadFAT(currentCluster);
    }

    std::cout << "OK" << std::endl;
}

void FilesystemFAT::CopyFile(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (argc != 2) {
        std::cout << "NO PATH" << std::endl;
        return;
    }

    if (strcmp(args[0].c_str(), FAT_DELIMETER_STR) == 0) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }
    std::vector<std::string> folders = GetFoldersFromPath(args[0]);
    IndexFAT parentIndex = FAT_EOF;
    IndexFAT index = FindFolderDestinationCluster(folders, &parentIndex);

    if (index == FAT_EOF || parentIndex == FAT_EOF || index == 0) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    size_t size = 0;

    for (int i = 0; i < static_cast<int>(c_maxDirectoryEntries); i++)
    {
        DirectoryEntry result{};
        if (!GetDirectoryEntryFromCluster(parentIndex, i, result)) {
            continue;
        }

        if (result.beginCluster == index) {
            size = result.size;
            break;
        }
    }

    if (size == 0) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    IndexFAT currentCluster = FAT_EOF;

    if (!CreateDirectoryEntry(args[1], false, size, false, &currentCluster)) {
        return;
    }
    if (currentCluster == FAT_EOF) {
        std::cout << "UNKNOW ERROR" << std::endl;
    }

    do {
        for (size_t i = 0; i < CLUSTER_SIZE; i++)
        {
            unsigned char byte = ReadClusterByte(index, i);
            WriteClusterByte(currentCluster, i, byte);
        }
        index = ReadFAT(index);
        currentCluster = ReadFAT(currentCluster);
    } while (index != FAT_EOF);

    std::cout << "OK" << std::endl;
}

void FilesystemFAT::MoveFile(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (argc != 2) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    if (!DestinationPathIsValid(args[1])) {
        return;
    }

    if (IsFolder(args[0])) {
        std::cout << "IS NOT FILE!" << std::endl;
        return;
    }

    DirectoryEntry entry{};
    if (!RemoveDirectoryEntryAndContents(args[0], false, false, &entry)) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    if (!CreateDirectoryEntry(args[1], false, entry.size, true, &entry.beginCluster)) {
        return;
    }

    std::cout << "OK" << std::endl;
}

void FilesystemFAT::RemoveFile(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (argc != 1) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    if (IsFolder(args[0])) {
        std::cout << "IS NOT FILE!" << std::endl;
        return;
    }

    if (!RemoveDirectoryEntryAndContents(args[0], false, true)) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    std::cout << "OK" << std::endl;
}

void FilesystemFAT::CreateFolder(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (argc != 1) {
        std::cout << "PATH NOT FOUND" << std::endl;
        return;
    }

    if (!CreateDirectoryEntry(args[0], true)) {
        return;
    }

    std::cout << "OK" << std::endl;
}

void FilesystemFAT::RemoveFolder(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (argc != 1) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    if (!RemoveDirectoryEntryAndContents(args[0], true, true)) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }
 
    std::cout << "OK" << std::endl;
}

void FilesystemFAT::OutputFolderContents(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (argc != 0 && argc != 1) {
        std::cout << "PATH NOT FOUND" << std::endl;
    }

    if (argc == 1) {
        if (!IsFolder(args[0])) {
            std::cout << "IS NOT DIRECTORY!" << std::endl;
            return;
        }
    }

    std::string currentPath = argc == 0 ? m_currentFolder : args[0];

    IndexFAT index = FAT_EOF;

    if (strcmp(currentPath.c_str(), FAT_DELIMETER_STR) == 0) {
        index = 0;
    } else {
        std::vector<std::string> folders = GetFoldersFromPath(currentPath);
        index = FindFolderDestinationCluster(folders);
    }

    if (index != FAT_EOF) {
        for (int i = 0; i < static_cast<int>(c_maxDirectoryEntries); i++)
        {
            DirectoryEntry result{};
            if (!GetDirectoryEntryFromCluster(index, i, result)) {
                continue;
            }

            if (result.isFolder) {
                std::cout << "Folder: " << result.name << std::endl;
            }
            else {
                std::cout << "File: " << result.name << "; Size: " << result.size << " bytes" << std::endl;
            }
        }
        return;
    }
    std::cout << "PATH NOT FOUND" << std::endl;
}

void FilesystemFAT::OutputFileContents(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (argc != 1) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    if (strcmp(args[0].c_str(), FAT_DELIMETER_STR) == 0) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    if (IsFolder(args[0])) {
        std::cout << "IS NOT FILE!" << std::endl;
        return;
    }

    std::vector<std::string> folders = GetFoldersFromPath(args[0]);
    IndexFAT index = FindFolderDestinationCluster(folders);

    if (index != FAT_EOF && index != 0) {

        do {
            for (auto& item : ReadFullCluster(index)) {
                std::cout << item;
            }
            index = ReadFAT(index);
        } while (index != FAT_EOF);
        
        std::cout << std::endl;
        return;
    }
    std::cout << "FILE NOT FOUND" << std::endl;
}

void FilesystemFAT::ChangeCurrentFolder(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (argc != 1) {
        std::cout << "PATH NOT FOUND" << std::endl;
        return;
    }

    if (strcmp(args[0].c_str(), FAT_DELIMETER_STR) == 0) {
        m_currentFolder = FAT_DELIMETER_STR;
        std::cout << "OK" << std::endl;
        return;
    }

    if (!IsFolder(args[0])) {
        std::cout << "IS NOT DIRECTORY!" << std::endl;
        return;
    }

    std::vector<std::string> folders = GetFoldersFromPath(args[0]);

    IndexFAT index = FindFolderDestinationCluster(folders);
    
    if (index != FAT_EOF) {
        m_currentFolder = FAT_DELIMETER_STR;
        for (auto& folder : folders) {
            m_currentFolder += folder + FAT_DELIMETER_STR;
        }
        std::cout << "OK" << std::endl;
        return;
    }

    std::cout << "PATH NOT FOUND" << std::endl;
}

void FilesystemFAT::OutputCurrentFolder(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    std::cout << m_currentFolder << std::endl;
}

void FilesystemFAT::OutputInfoFileFolder(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (argc != 1 || args[0].empty()) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    bool isFolder = IsFolder(args[0]);

    IndexFAT index = FAT_EOF;
    std::vector<std::string> folders{};

    if (strcmp(args[0].c_str(), FAT_DELIMETER_STR) == 0) {
        index = 0;
    }
    else {
        folders = GetFoldersFromPath(args[0]);
        index = FindFolderDestinationCluster(folders);
    }

    if (index != FAT_EOF) {

        std::string name = index == 0 ? FAT_DELIMETER_STR : folders.back();

        std::cout << name << " - ";
        if (isFolder) {
            std::cout << "dir - cluster - ";
        } else {
            std::cout << "file - size: " << GetSize(args[0]) << " bytes - ";
        }

        std::cout << index;

        while (ReadFAT(index) != FAT_EOF)
        {
            index = ReadFAT(index);
            std::cout << ", " << index;
        }

        std::cout << std::endl;

        return;
    }

    std::cout << "FILE NOT FOUND" << std::endl;
}

void FilesystemFAT::InputCopyFile(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (argc != 2) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    std::ifstream file(args[0], std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    IndexFAT currentCluster = FAT_EOF;

    if (!CreateDirectoryEntry(args[1], false, size, false, &currentCluster)) {
        return;
    }
    if (currentCluster == FAT_EOF) {
        std::cout << "UNKNOW ERROR" << std::endl;
    }

    char ch{};
    do {
        for (size_t i = 0; i < CLUSTER_SIZE; i++)
        {
            if (!file.get(ch)) {
                break;
            }
            WriteClusterByte(currentCluster, i, static_cast<unsigned char>(ch));
        }
        currentCluster = ReadFAT(currentCluster);
    } while (currentCluster != FAT_EOF);

    file.close();

    std::cout << "OK" << std::endl;
}

void FilesystemFAT::OutputCopyFile(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (argc != 2) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    if (strcmp(args[0].c_str(), FAT_DELIMETER_STR) == 0) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }
    std::vector<std::string> folders = GetFoldersFromPath(args[0]);
    IndexFAT index = FindFolderDestinationCluster(folders);

    if (index == FAT_EOF || index == 0) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    if (IsFolder(args[0])) {
        std::cout << "IS NOT FILE!" << std::endl;
        return;
    }

    std::ofstream outFile(args[1], std::ios::binary);

    if (!outFile) {
        std::cout << "PATH NOT FOUND" << std::endl;
        return;
    }

    do {
        for (size_t i = 0; i < CLUSTER_SIZE; i++)
        {
            unsigned char data = ReadClusterByte(index, i);
            outFile.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }
        index = ReadFAT(index);
    } while (index != FAT_EOF);

    outFile.close();
    std::cout << "OK" << std::endl;
}

void FilesystemFAT::LoadFile(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (m_commandHandle == nullptr) {
        throw std::exception("COMMAND HANDLE IS NULL!!!!!!!!!");
    }

    if (argc != 1) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    if (strcmp(args[0].c_str(), FAT_DELIMETER_STR) == 0) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    if (IsFolder(args[0])) {
        std::cout << "IS NOT FILE!" << std::endl;
        return;
    }

    std::vector<std::string> folders = GetFoldersFromPath(args[0]);
    IndexFAT index = FindFolderDestinationCluster(folders);

    std::string instruction{};

    if (index != FAT_EOF && index != 0) {

        do {
            for (auto& item : ReadFullCluster(index)) {
                if (item == '\n') {
                    std::string command{};
                    int numberOfArguments = 0;
                    std::vector<std::string> parameters{};
                    m_commandHandle->GetCommandString(instruction, command, numberOfArguments, parameters);
                    m_commandHandle->Execute(command, numberOfArguments, parameters.data());
                    instruction.clear();
                    continue;
                }

                if (item == '\r' || item == '\0') {
                    continue;
                }
                    
                instruction += item;
            }
            index = ReadFAT(index);
        } while (index != FAT_EOF);

        if (!instruction.empty()) {
            std::string command{};
            int numberOfArguments = 0;
            std::vector<std::string> parameters{};
            m_commandHandle->GetCommandString(instruction, command, numberOfArguments, parameters);
            m_commandHandle->Execute(command, numberOfArguments, parameters.data());
        }

        std::cout << "OK" << std::endl;
        return;
    }
    std::cout << "FILE NOT FOUND" << std::endl;
}

void FilesystemFAT::FormatFile(int argc, std::string* args)
{
	if (m_fileDiskPath.empty()) {
		throw std::exception("Missing file disk");
	}

    const unsigned long long fileSize = GetFileSizeInBytes(args[0]);
    
    if (fileSize == 0) {
        std::cout << "CANNOT CREATE FILE" << std::endl;
        return;
    }

    bool reopenDisk = false;
    if (m_fileDisk != nullptr) {
        m_fileDisk->close();
        m_fileDisk = nullptr;
        reopenDisk = true;
    }

    if (!CreateFileDisk(m_fileDiskPath)) {
        std::cout << "CANNOT CREATE FILE" << std::endl;
        return;
    }

    if (!ResizeFileDisk(m_fileDiskPath, fileSize)) {
        std::cout << "CANNOT CREATE FILE" << std::endl;
        return;
    }

    if (reopenDisk) {
        m_fileDisk = std::make_unique<std::fstream>(m_fileDiskPath, std::ios::in | std::ios::out | std::ios::binary);
        if (!*m_fileDisk) {
            throw std::exception("UNKNOW ERROR :/");
        }
    }

    m_numberOfClusters = fileSize / CLUSTER_SIZE;
    

    WriteFAT(0, FAT_EOF); //root reserve
    m_currentFolder = FAT_DELIMETER_STR; //set it to root folder

    m_diskIsNotFormated = false;
    std::cout << "OK" << std::endl;
}

void FilesystemFAT::CheckIntegrity(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (!CheckValidFATNumbers()) {
        std::cout << "WRONG CLUSTER NUMBER" << std::endl;
        return;
    }

    if (!CheckNonCyclicFATLink()) {
        std::cout << "CYCLIC CLUSTER DETECTED" << std::endl;
        return;
    }

    std::cout << "OK" << std::endl;
}

void FilesystemFAT::BugIntegrity(int argc, std::string* args)
{
    DISK_IS_FORMATED_CHECK();

    if (argc != 1) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }

    if (IsFolder(args[0])) {
        std::cout << "IS NOT FILE!" << std::endl;
        return;
    }

    size_t size = GetSize(args[0]);

    if (size > CLUSTER_SIZE) {
        BugCyclicCluster(args[0]);
    } else {
        BugFATNumber(args[0]);
    }

    std::cout << "OK" << std::endl;
}

bool FilesystemFAT::DestinationPathIsValid(const std::string path, std::string* outName, IndexFAT* outIndex)
{
    std::vector<std::string> folders = GetFoldersFromPath(path);

    if (folders.size() == 0) {
        std::cout << "PATH NOT FOUND" << std::endl;
        return false;
    }

    IndexFAT index = FindFolderDestinationCluster(folders);

    if (index != FAT_EOF) {
        std::cout << "EXISTS" << std::endl;
        return false;
    }

    std::string name = folders.back();
    folders.pop_back();

    index = FindFolderDestinationCluster(folders);

    if (index == FAT_EOF) {
        std::cout << "PATH NOT FOUND" << std::endl;
        return false;
    }

    if (strcmp(name.c_str(), ERROR_STRING) == 0) {
        std::cout << "PATH NOT FOUND" << std::endl;
        return false;
    }

    if (outIndex != nullptr) {
        *outIndex = index;
    }
    if (outName != nullptr) {
        *outName = name;
    }

    return true;
}

bool FilesystemFAT::CreateDirectoryEntry(std::string path, bool isFolder, size_t fileSize, bool writeBeginCluster, IndexFAT* outBeginCluster)
{
    if (writeBeginCluster && outBeginCluster == nullptr) {
        throw std::exception("In write mode you must provide BeginCluster index!!!");
    }

    if (strcmp(path.c_str(), FAT_DELIMETER_STR) == 0) {
        std::cout << "EXIST" << std::endl;
        return false;
    }

    IndexFAT freeIndex = GetFreeIndexSlot();
    if (freeIndex == FAT_EOF) {
        std::cout << "NO MEMORY IN DISK" << std::endl;
        return false;
    }

    std::string name{};
    IndexFAT index = FAT_EOF;
    if (!DestinationPathIsValid(path, &name, &index)) {
        return false;
    }

    DirectoryEntry* entry = new DirectoryEntry();
    name.copy(entry->name, MAX_NAME_LENGHT - 1);
    entry->name[MAX_NAME_LENGHT - 1] = '\0';
    entry->isFolder = isFolder;
    entry->beginCluster = (writeBeginCluster ? *outBeginCluster : freeIndex);
    entry->size = fileSize;
    if (!WriteDirectoryEntry(index, entry, !writeBeginCluster)) {
        std::cout << "CLUSTER IS FULL" << std::endl;
        return false;
    }

    if (!writeBeginCluster && outBeginCluster != nullptr) {
        *outBeginCluster = entry->beginCluster;
    }

    delete entry;
    return true;
}

bool FilesystemFAT::RemoveDirectoryEntryAndContents(std::string path, bool isFolder, bool removeContents, DirectoryEntry* outEntry)
{
    if (strcmp(path.c_str(), FAT_DELIMETER_STR) == 0) {
        std::cout << "FILE NOT FOUND - CANNOT REMOVE ROOT!" << std::endl;
        return false;
    }

    std::vector<std::string> folders = GetFoldersFromPath(path);
    IndexFAT parentIndex = FAT_EOF;
    IndexFAT index = FindFolderDestinationCluster(folders, &parentIndex);

    if (index == 0) {
        std::cout << "FILE NOT FOUND - CANNOT REMOVE ROOT!" << std::endl;
        return false;
    }

    if (index != FAT_EOF && parentIndex != FAT_EOF) {
        
        if (isFolder) {
            for (int i = 0; i < static_cast<int>(c_maxDirectoryEntries); i++)
            {
                DirectoryEntry result{};
                if (!GetDirectoryEntryFromCluster(index, i, result)) {
                    continue;
                }

                std::cout << "NOT EMPTY" << std::endl;
                return false;
            }
        }

        for (int i = 0; i < static_cast<int>(c_maxDirectoryEntries); i++)
        {
            DirectoryEntry result{};
            if (!GetDirectoryEntryFromCluster(parentIndex, i, result)) {
                continue;
            }

            if (result.beginCluster != index) {
                continue;
            }
            
            ClearClusterEntry(parentIndex, i * sizeof(DirectoryEntry));

            if (outEntry != nullptr) {
                *outEntry = result;
            }

            if (removeContents) {
                do {
                    ClearWholeCluster(index);
                    IndexFAT tmp = index;
                    index = ReadFAT(index);
                    WriteFAT(tmp, FAT_FREE);
                } while (index != FAT_EOF);
            }

            return true;
        }
    }
    return false;
}

bool FilesystemFAT::WriteDirectoryEntry(const IndexFAT clusterIndex, DirectoryEntry* entry, bool writeFATIndexes)
{
    size_t freeOffset = FAT_EOF;
    for (size_t i = 0; i < c_maxDirectoryEntries; i++)
    {
        size_t offset = i * sizeof(DirectoryEntry);

        DirectoryEntry entryRes{};
        ReadClusterEntryData(clusterIndex, offset, &entryRes);

        if (IsClusterEntryEmpty(clusterIndex, offset)) {
            freeOffset = offset;
            break;
        }
    }

    if (freeOffset == FAT_EOF) {
        return false; //if cluster is already full
    }

    WriteClusterEntryData(clusterIndex, freeOffset, entry);

    if (!writeFATIndexes) {
        return true;
    }

    if (entry->isFolder) {
        WriteFAT(entry->beginCluster, FAT_EOF); //for folders
    } else {
        size_t numberOfClustersNeeded = (entry->size + CLUSTER_SIZE - 1) / CLUSTER_SIZE;
        
        IndexFAT currentFATIndex = entry->beginCluster;

        for (size_t i = 1; i < numberOfClustersNeeded; i++)
        {
            WriteFAT(currentFATIndex, FAT_EOF); //we need flag this out bc our func will return same index
            WriteFAT(currentFATIndex, GetFreeIndexSlot());
            currentFATIndex = ReadFAT(currentFATIndex);
        }
        WriteFAT(currentFATIndex, FAT_EOF); //end of the files
    }
    
    return true;
}

IndexFAT FilesystemFAT::FindFolderDestinationCluster(std::vector<std::string> folders, IndexFAT* outParentIndex) const
{
    if (folders.size() == 0) {
        return 0; //root or undefined behaviour
    }

    IndexFAT currentCluster = 0; //root
    bool found = false;
    for (auto& folder : folders) {
        found = false;
        for (int i = 0; i < static_cast<int>(c_maxDirectoryEntries); i++)
        {
            DirectoryEntry result{};
            if (!GetDirectoryEntryFromCluster(currentCluster, i, result)) {
                continue;
            }

            if (strcmp(folder.c_str(), result.name) == 0) {
                
                if (outParentIndex != nullptr) {
                    *outParentIndex = currentCluster;
                }

                found = true;
                currentCluster = result.beginCluster;
                break;
            }
        }
        if (!found) {
            break;
        }
    }

    if (found) {
        return currentCluster;
    }

    return FAT_EOF;
}

bool FilesystemFAT::GetDirectoryEntryFromCluster(const IndexFAT cluster, const int offsetIndex, DirectoryEntry& outEntry) const
{
    if (offsetIndex < 0 || offsetIndex >= c_maxDirectoryEntries) {
        return false;
    }

    size_t offset = offsetIndex * sizeof(DirectoryEntry);

    if (IsClusterEntryEmpty(cluster, offset)) {
        return false;
    }

    ReadClusterEntryData(cluster, offset, &outEntry);
    return true;
}

std::vector<std::string> FilesystemFAT::GetFoldersFromPath(std::string path)
{
    if (path.empty()) {
        return { ERROR_STRING };
    }

    if (strcmp(path.c_str(), ".") == 0) {
        return GetFoldersFromPath(m_currentFolder);
    }

    if (strcmp(path.c_str(), "..") == 0) {
        std::vector<std::string> result = GetFoldersFromPath(m_currentFolder);
        if (result.size() > 0) {
            result.erase(result.end() - 1);
        }
        return result;
    }

    if (path.find(FAT_DELIMETER) == std::string::npos) {
        return GetFoldersFromPath(m_currentFolder + path);
    }

    if (path[0] == '.' && path[1] == FAT_DELIMETER) {
        return GetFoldersFromPath(m_currentFolder + path.erase(0, 2));
    }

    if (path[0] == '.' && path[1] == '.' && path[2] == FAT_DELIMETER) {
        std::vector<std::string> result = GetFoldersFromPath(m_currentFolder);
        if (result.size() > 0) {
            result.erase(result.end() - 1);
        }
        std::string newPath = FAT_DELIMETER_STR;
        for (auto& folder : result) {
            newPath += folder + FAT_DELIMETER_STR;
        }
        return GetFoldersFromPath(newPath + path.erase(0, 3));
    }

    auto split_view = path | std::views::split(FAT_DELIMETER);

    std::vector<std::string> folders{};

    for (auto&& word : split_view) {
        folders.emplace_back(word.begin(), word.end());
    }
    if (path[0] == FAT_DELIMETER) {
        folders.erase(folders.begin()); //for absolute /.... path we remove first entry, bc it is empty
    }
    if (path[path.size() - 1] == FAT_DELIMETER) {
        folders.erase(folders.end() - 1); //erase also last empty bc user put / in the end
    }

    return folders;
}

IndexFAT FilesystemFAT::GetFreeIndexSlot() const
{
    for (IndexFAT i = 0; i < static_cast<IndexFAT>(m_numberOfClusters); i++)
    {
        if (ReadFAT(static_cast<IndexFAT>(i)) == FAT_FREE) {
            return i;
        }
    }

    return FAT_EOF;
}

void FilesystemFAT::WriteFAT(IndexFAT index, IndexFAT data)
{
    if (m_fileDiskPath.empty()) {
        throw std::exception("NO FILE DISKKK!!!");
    }

    size_t size = sizeof(IndexFAT);

    std::streampos position = index * size;
    
    m_fileDisk->seekp(position);
    m_fileDisk->write(reinterpret_cast<const char*>(&data), sizeof(data));

    if (!m_fileDisk->good()) {
        throw std::exception("UNKNOW ERROR :/");
    }
}

IndexFAT FilesystemFAT::ReadFAT(IndexFAT index) const
{
    if (m_fileDiskPath.empty()) {
        throw std::exception("NO FILE DISKKK!!!");
    }

    size_t size = sizeof(IndexFAT);

    std::streampos position = index * size;
    m_fileDisk->seekg(position);

    if (!*m_fileDisk) {
        throw std::exception("UNKNOW ERROR :/");
    }

    IndexFAT data;
    m_fileDisk->read(reinterpret_cast<char*>(&data), size);

    return data;
}

void FilesystemFAT::WriteClusterEntryData(IndexFAT clusterIndex, size_t dataIndex, DirectoryEntry* entry)
{
    if (m_fileDiskPath.empty()) {
        throw std::exception("NO FILE DISKKK!!!");
    }

    std::streampos position = (m_numberOfClusters * sizeof(IndexFAT)) + (clusterIndex * sizeof(Cluster)) + dataIndex;
    m_fileDisk->seekp(position);

    m_fileDisk->write(reinterpret_cast<const char*>(entry), sizeof(DirectoryEntry));

    if (!m_fileDisk->good()) {
        throw std::exception("UNKNOW ERROR :/");
    }
}

void FilesystemFAT::ReadClusterEntryData(IndexFAT clusterIndex, size_t dataIndex, DirectoryEntry* entry) const
{
    if (m_fileDiskPath.empty()) {
        throw std::exception("NO FILE DISKKK!!!");
    }

    if (entry == nullptr) {
        throw std::exception("EMPTY ENTRY!");
    }

    std::streampos position = (m_numberOfClusters * sizeof(IndexFAT)) + (clusterIndex * sizeof(Cluster)) + dataIndex;
    m_fileDisk->seekg(position);

    if (!*m_fileDisk) {
        throw std::exception("UNKNOW ERROR :/");
    }

    m_fileDisk->read(reinterpret_cast<char*>(entry), sizeof(DirectoryEntry));
}

void FilesystemFAT::WriteClusterByte(IndexFAT clusterIndex, size_t dataIndex, unsigned char byte)
{
    if (m_fileDiskPath.empty()) {
        throw std::exception("NO FILE DISKKK!!!");
    }

    std::streampos position = (m_numberOfClusters * sizeof(IndexFAT)) + (clusterIndex * sizeof(Cluster)) + dataIndex;
    char byteToWrite = static_cast<char>(byte);

    m_fileDisk->seekp(position);
    m_fileDisk->write(&byteToWrite, sizeof(byteToWrite));

    if (!m_fileDisk->good()) {
        throw std::exception("UNKNOW ERROR :/");
    }
}

unsigned char FilesystemFAT::ReadClusterByte(IndexFAT clusterIndex, size_t dataIndex)
{
    if (m_fileDiskPath.empty()) {
        throw std::exception("NO FILE DISKKK!!!");
    }

    std::streampos position = (m_numberOfClusters * sizeof(IndexFAT)) + (clusterIndex * sizeof(Cluster)) + dataIndex;
    m_fileDisk->seekg(position);

    if (!*m_fileDisk) {
        throw std::exception("UNKNOW ERROR :/");
    }

    char byte;
    m_fileDisk->read(&byte, 1);

    return static_cast<unsigned char>(byte);
}

Cluster FilesystemFAT::ReadFullCluster(IndexFAT clusterIndex)
{
    if (m_fileDiskPath.empty()) {
        throw std::exception("NO FILE DISKKK!!!");
    }

    std::streampos position = (m_numberOfClusters * sizeof(IndexFAT)) + (clusterIndex * sizeof(Cluster));
    m_fileDisk->seekg(position);

    if (!*m_fileDisk) {
        throw std::exception("UNKNOW ERROR :/");
    }

    Cluster result{};
    m_fileDisk->read(reinterpret_cast<char*>(result.data()), result.size());

    return result;
}

void FilesystemFAT::ClearWholeCluster(IndexFAT clusterIndex)
{
    if (m_fileDiskPath.empty()) {
        throw std::exception("NO FILE DISKKK!!!");
    }

    std::streampos position = (m_numberOfClusters * sizeof(IndexFAT)) + (clusterIndex * sizeof(Cluster));
    m_fileDisk->seekp(position);

    std::vector<char> zeros(CLUSTER_SIZE, FAT_FREE);
    m_fileDisk->write(zeros.data(), zeros.size());

    if (!m_fileDisk->good()) {
        throw std::exception("UNKNOW ERROR :/");
    }
}

void FilesystemFAT::ClearClusterEntry(IndexFAT clusterIndex, size_t dataIndex)
{
    if (m_fileDiskPath.empty()) {
        throw std::exception("NO FILE DISKKK!!!");
    }

    std::streampos position = (m_numberOfClusters * sizeof(IndexFAT)) + (clusterIndex * sizeof(Cluster)) + dataIndex;
    m_fileDisk->seekp(position);

    std::vector<char> zeros(sizeof(DirectoryEntry), FAT_FREE);
    m_fileDisk->write(zeros.data(), zeros.size());

    if (!m_fileDisk->good()) {
        throw std::exception("UNKNOW ERROR :/");
    }
}

bool FilesystemFAT::IsClusterEntryEmpty(IndexFAT clusterIndex, size_t dataIndex) const
{
    if (m_fileDiskPath.empty()) {
        throw std::exception("NO FILE DISKKK!!!");
    }

    std::streampos position = (m_numberOfClusters * sizeof(IndexFAT)) + (clusterIndex * sizeof(Cluster)) + dataIndex;
    m_fileDisk->seekg(position);

    std::vector<char> buffer(sizeof(DirectoryEntry));
    m_fileDisk->read(buffer.data(), sizeof(DirectoryEntry));

    if (!*m_fileDisk) {
        throw std::exception("UNKNOW ERROR :/");
    }

    return std::all_of(buffer.begin(), buffer.end(), [](char byte) {
        return byte == 0;
        });
}

bool FilesystemFAT::CheckValidFATNumbers()
{
    for (IndexFAT i = 0; i < m_numberOfClusters; i++)
    {
        IndexFAT index = ReadFAT(i);

        if (index == FAT_EOF) {
            continue;
        }
        if (index < m_numberOfClusters) {
            continue;
        }

        std::cout << "FAT[" << i << "] has incorrect cluster number: " << index << std::endl;
        return false;
    }

    return true;
}

bool FilesystemFAT::CheckNonCyclicFATLink()
{
    for (IndexFAT i = 0; i < m_numberOfClusters; i++)
    {
        std::vector<IndexFAT> alreadyVisitedIndexes{};
        if (CheckNonCyclicFATLink(alreadyVisitedIndexes, i)) {
            continue;
        }
        std::cout << "ERROR: FAT[" << i << "] has cyclic clusters" << std::endl;
        return false;
    }

    return true;
}

bool FilesystemFAT::CheckNonCyclicFATLink(std::vector<IndexFAT>& alreadyVisited, IndexFAT index)
{
    IndexFAT newIndex = ReadFAT(index);

    if (newIndex == FAT_EOF) {
        return true;
    }

    for (auto& i : alreadyVisited) {
        if (i == newIndex) {
            return false;
        }
    }
    alreadyVisited.push_back(newIndex);
    return CheckNonCyclicFATLink(alreadyVisited, newIndex);
}

void FilesystemFAT::BugFATNumber(std::string path)
{
    std::vector<std::string> folders = GetFoldersFromPath(path);
    IndexFAT index = FindFolderDestinationCluster(folders);

    const IndexFAT bugIndex = static_cast<IndexFAT>(m_numberOfClusters) + 67;

    WriteFAT(index, bugIndex);
}

void FilesystemFAT::BugCyclicCluster(std::string path)
{
    std::vector<std::string> folders = GetFoldersFromPath(path);
    IndexFAT lastIndex = FindFolderDestinationCluster(folders);
    IndexFAT currentIndex = ReadFAT(lastIndex);

    WriteFAT(currentIndex, lastIndex);
}

bool FilesystemFAT::IsFolder(std::string path)
{
    std::vector<std::string> folders = GetFoldersFromPath(path);
    IndexFAT parentIndex = FAT_EOF;
    IndexFAT index = FindFolderDestinationCluster(folders, &parentIndex);

    if (parentIndex == FAT_EOF) {
        return true;//it is root or undefined behavior
    }

    for (int i = 0; i < static_cast<int>(c_maxDirectoryEntries); i++)
    {
        DirectoryEntry result{};
        if (!GetDirectoryEntryFromCluster(parentIndex, i, result)) {
            continue;
        }

        if (result.beginCluster == index) {
            return result.isFolder;
        }
    }

    return false; //Undefined behavior
}

unsigned long long FilesystemFAT::GetSize(std::string path)
{
    std::vector<std::string> folders = GetFoldersFromPath(path);
    IndexFAT parentIndex = FAT_EOF;
    IndexFAT index = FindFolderDestinationCluster(folders, &parentIndex);

    if (parentIndex == FAT_EOF) {
        return 0;//undefined behavior
    }

    for (int i = 0; i < static_cast<int>(c_maxDirectoryEntries); i++)
    {
        DirectoryEntry result{};
        if (!GetDirectoryEntryFromCluster(parentIndex, i, result)) {
            continue;
        }

        if (result.beginCluster == index) {
            return result.size;
        }
    }

    return 0; //Undefined behavior
}
