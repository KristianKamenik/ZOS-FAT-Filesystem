#pragma once
#include "CommandHandle.h"

#include <memory>
#include <string>

enum FileSystemType {
	FILE_SYSTEM_TYPE_NONE = 0,
	FILE_SYSTEM_TYPE_FAT = 1,
	FILE_SYSTEM_TYPE_COUNT = 2
};

class IFileSystem;

std::shared_ptr<IFileSystem> CreateFileSystem(const FileSystemType type);

class IFileSystem {
public:
	virtual void SetFileDisk(std::string fileDisk) = 0;
	virtual void SetCommandHandle(std::shared_ptr<CommandHandle> commandHandle) = 0;

	virtual void CreateFile(int argc, std::string* args) = 0;

	virtual void CopyFile(int argc, std::string* args) = 0;
	virtual void MoveFile(int argc, std::string* args) = 0;
	virtual void RemoveFile(int argc, std::string* args) = 0;
	virtual void CreateFolder(int argc, std::string* args) = 0;
	virtual void RemoveFolder(int argc, std::string* args) = 0;
	virtual void OutputFolderContents(int argc, std::string* args) = 0;
	virtual void OutputFileContents(int argc, std::string* args) = 0;
	virtual void ChangeCurrentFolder(int argc, std::string* args) = 0;
	virtual void OutputCurrentFolder(int argc, std::string* args) = 0;
	virtual void OutputInfoFileFolder(int argc, std::string* args) = 0;
	virtual void InputCopyFile(int argc, std::string* args) = 0;
	virtual void OutputCopyFile(int argc, std::string* args) = 0;
	virtual void LoadFile(int argc, std::string* args) = 0;
	virtual void FormatFile(int argc, std::string* args) = 0;
	virtual void CheckIntegrity(int argc, std::string* args) = 0;
	virtual void BugIntegrity(int argc, std::string* args) = 0;
};