#pragma once
#include "Filesystem.h"
#include "CommandHandle.h"

#include <array>
#include <vector>
#include <memory>
#include <fstream>

constexpr char FAT_DELIMETER = '/';
constexpr const char* FAT_DELIMETER_STR = "/";

using IndexFAT = unsigned int;

constexpr IndexFAT FAT_FREE= 0;
constexpr IndexFAT FAT_EOF= 0xFFFFFFFF;
constexpr size_t CLUSTER_SIZE = 256;
constexpr size_t MAX_NAME_LENGHT = 8 + 3 + 1;
constexpr char ERROR_STRING[16] = "___FAT_ERROR___";

using Cluster = std::array<unsigned char, CLUSTER_SIZE>;

class FilesystemFAT : public IFileSystem
{
public:
	FilesystemFAT() = default;
	~FilesystemFAT();

	void SetFileDisk(std::string fileDisk) override;
	void SetCommandHandle(std::shared_ptr<CommandHandle> commandHandle) override;

	void CreateFile(int argc, std::string* args) override;

	void CopyFile(int argc, std::string* args) override;
	void MoveFile(int argc, std::string* args) override;
	void RemoveFile(int argc, std::string* args) override;
	void CreateFolder(int argc, std::string* args) override;
	void RemoveFolder(int argc, std::string* args) override;
	void OutputFolderContents(int argc, std::string* args) override;
	void OutputFileContents(int argc, std::string* args) override;
	void ChangeCurrentFolder(int argc, std::string* args) override;
	void OutputCurrentFolder(int argc, std::string* args) override;
	void OutputInfoFileFolder(int argc, std::string* args) override;
	void InputCopyFile(int argc, std::string* args) override;
	void OutputCopyFile(int argc, std::string* args) override;
	void LoadFile(int argc, std::string* args) override;
	void FormatFile(int argc, std::string* args) override;
	void CheckIntegrity(int argc, std::string* args) override;
	void BugIntegrity(int argc, std::string* args) override;

private:
#pragma pack(push, 1)
	struct DirectoryEntry {
		char name[MAX_NAME_LENGHT] = { 0 };
		bool isFolder = false;
		unsigned long long size = 0;
		IndexFAT beginCluster = FAT_EOF;
	};
#pragma pack(pop)

	const size_t c_maxDirectoryEntries = CLUSTER_SIZE / sizeof(DirectoryEntry);

	bool DestinationPathIsValid(const std::string path, std::string* outName = nullptr, IndexFAT* outIndex = nullptr);
	bool CreateDirectoryEntry(std::string path, bool isFolder, size_t fileSize = 0, bool writeBeginCluster = false, IndexFAT* outBeginCluster = nullptr);
	bool RemoveDirectoryEntryAndContents(std::string path, bool isFolder, bool removeContents, DirectoryEntry* outEntry = nullptr);
	bool WriteDirectoryEntry(const IndexFAT clusterIndex, DirectoryEntry* entry, bool writeFATIndexes = true);
	IndexFAT FindFolderDestinationCluster(std::vector<std::string> folders, IndexFAT* outParentIndex = nullptr) const;
	bool GetDirectoryEntryFromCluster(const IndexFAT cluster, const int offsetIndex, DirectoryEntry& outEntry) const;
	std::vector<std::string> GetFoldersFromPath(std::string path);
	IndexFAT GetFreeIndexSlot() const;

	void WriteFAT(IndexFAT index, IndexFAT data);
	IndexFAT ReadFAT(IndexFAT index) const;
	void WriteClusterEntryData(IndexFAT clusterIndex, size_t dataIndex, DirectoryEntry* entry);
	void ReadClusterEntryData(IndexFAT clusterIndex, size_t dataIndex, DirectoryEntry* entry) const;
	void WriteClusterByte(IndexFAT clusterIndex, size_t dataIndex, unsigned char byte);
	unsigned char ReadClusterByte(IndexFAT clusterIndex, size_t dataIndex);
	Cluster ReadFullCluster(IndexFAT clusterIndex);

	void ClearWholeCluster(IndexFAT clusterIndex);
	void ClearClusterEntry(IndexFAT clusterIndex, size_t dataIndex);

	bool IsClusterEntryEmpty(IndexFAT clusterIndex, size_t dataIndex) const;

	bool CheckValidFATNumbers();
	bool CheckNonCyclicFATLink();
	bool CheckNonCyclicFATLink(std::vector<IndexFAT>& alreadyVisited, IndexFAT index);

	void BugFATNumber(std::string path);
	void BugCyclicCluster(std::string path);

	bool IsFolder(std::string path);
	unsigned long long GetSize(std::string path);

	std::shared_ptr<CommandHandle> m_commandHandle = nullptr;

	std::unique_ptr<std::fstream> m_fileDisk = nullptr;

	bool m_diskIsNotFormated = false;

	unsigned long long m_numberOfClusters{};
	std::string m_fileDiskPath{};
	std::string m_currentFolder = FAT_DELIMETER_STR;
};

