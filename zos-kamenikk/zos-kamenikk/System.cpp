#include "System.h"
#include <iostream>
#include <string>
#include <ranges>
#include <vector>

System::System()
{
	m_commandHandle = std::make_shared<CommandHandle>();
}

bool System::Initialize(const FileSystemType filesystemType, std::string fileDisk)
{
	m_filesystem = CreateFileSystem(filesystemType);

	if (!m_filesystem) {
		return false;
	}

	m_filesystem->SetFileDisk(fileDisk);
	m_filesystem->SetCommandHandle(m_commandHandle);

	InitializeCommandHandle();

	return true;
}

void System::Run()
{
	m_systemRunning = true;
	do {
		std::cout << ":> ";

		std::string arg{};
		std::getline(std::cin, arg);

		if (arg.empty()) {
			continue;
		}

		std::string command{};
		int numberOfArg{};
		std::vector<std::string> args{};
		
		m_commandHandle->GetCommandString(arg, command, numberOfArg, args);
		m_commandHandle->Execute(command, numberOfArg, args.data());

	} while (m_systemRunning);
}

void System::InitializeCommandHandle()
{
	m_commandHandle->AddCommand("exit", { 0 }, [&](int, std::string*) { m_systemRunning = false; });
	m_commandHandle->AddCommand("clear", { 0 }, [&](int, std::string*) { std::cout << "\033[2J\033[H"; });
	m_commandHandle->AddCommand("cp", { 2 }, [&](int argc, std::string* argv) { m_filesystem->CopyFile(argc, argv); });
	m_commandHandle->AddCommand("mv", { 2 }, [&](int argc, std::string* argv) { m_filesystem->MoveFile(argc, argv); });
	m_commandHandle->AddCommand("rm", { 1 }, [&](int argc, std::string* argv) { m_filesystem->RemoveFile(argc, argv); });
	m_commandHandle->AddCommand("mkdir", { 1 }, [&](int argc, std::string* argv) { m_filesystem->CreateFolder(argc, argv); });
	m_commandHandle->AddCommand("rmdir", { 1 }, [&](int argc, std::string* argv) { m_filesystem->RemoveFolder(argc, argv); });
	m_commandHandle->AddCommand("ls", { 0, 1 }, [&](int argc, std::string* argv) { m_filesystem->OutputFolderContents(argc, argv); });
	m_commandHandle->AddCommand("cat", { 1 }, [&](int argc, std::string* argv) { m_filesystem->OutputFileContents(argc, argv); });
	m_commandHandle->AddCommand("cd", { 1 }, [&](int argc, std::string* argv) { m_filesystem->ChangeCurrentFolder(argc, argv); });
	m_commandHandle->AddCommand("pwd", { 0 }, [&](int argc, std::string* argv) { m_filesystem->OutputCurrentFolder(argc, argv); });
	m_commandHandle->AddCommand("info", { 1 }, [&](int argc, std::string* argv) { m_filesystem->OutputInfoFileFolder(argc, argv); });
	m_commandHandle->AddCommand("incp", { 2 }, [&](int argc, std::string* argv) { m_filesystem->InputCopyFile(argc, argv); });
	m_commandHandle->AddCommand("outcp", { 2 }, [&](int argc, std::string* argv) { m_filesystem->OutputCopyFile(argc, argv); });
	m_commandHandle->AddCommand("load", { 1 }, [&](int argc, std::string* argv) { m_filesystem->LoadFile(argc, argv); });
	m_commandHandle->AddCommand("format", { 1 }, [&](int argc, std::string* argv) { m_filesystem->FormatFile(argc, argv); });
	
	//bonus
	m_commandHandle->AddCommand("check", { 0 }, [&](int argc, std::string* argv) { m_filesystem->CheckIntegrity(argc, argv); });
	m_commandHandle->AddCommand("bug", { 1 }, [&](int argc, std::string* argv) { m_filesystem->BugIntegrity(argc, argv); });

	//for testing
	m_commandHandle->AddCommand("lol", { 0, 1 }, [&](int argc, std::string* argv) { m_filesystem->CreateFile(argc, argv); });
}
