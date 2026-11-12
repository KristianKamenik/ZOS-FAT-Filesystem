#include <iostream>
#include <string>
#include <filesystem>

#include "System.h"

#define CURRENT_FILE_SYSTEM FILE_SYSTEM_TYPE_FAT

enum ErrorReturnCodes {
	CODE_SUCCESS = 0,
	CODE_GENERAL_ERROR = -1,
	CODE_INITIALIZATION_ERROR = -2,
	CODE_FORMAT_ERROR = -3
};

int main(int argc, char* argv[])
{
	if (argc == 2) {
		System system;
		if (!system.Initialize(CURRENT_FILE_SYSTEM, argv[1]))
			return CODE_INITIALIZATION_ERROR;

		system.Run();
		return 0;
	}

	std::cout << "Missing or incorrect arguments, call name of your desired filesystem" << std::endl;
	std::cout << argv[0] << " [Filesystem name]" << std::endl;

	return CODE_GENERAL_ERROR;
}