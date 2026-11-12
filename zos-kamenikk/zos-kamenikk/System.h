#pragma once
#include "Filesystem.h"
#include "CommandHandle.h"

#include <memory>

class System
{
public:
	System();
	~System() = default;

	bool Initialize(const FileSystemType filesystemType, std::string fileDisk);
	void Run();

private:
	void InitializeCommandHandle();

	std::shared_ptr<IFileSystem> m_filesystem = nullptr;
	
	bool m_systemRunning = false;
	std::shared_ptr<CommandHandle> m_commandHandle = nullptr;
};

