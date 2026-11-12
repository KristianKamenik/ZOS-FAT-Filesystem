#pragma once
#include <string>
#include <functional>
#include <unordered_map>

using CommandAction = std::function<void(int, std::string*)>;

class CommandHandle
{
public:
	CommandHandle() = default;
	~CommandHandle() = default;

	void GetCommandString(std::string input, std::string& command, int& numberOfArg, std::vector<std::string>& args);
	void AddCommand(std::string command, std::vector<int> numberOfArguments, CommandAction action);
	void Execute(std::string command, int numberOfArguments, std::string* arguments);
private:
	bool CheckCommand(std::string command, int numberOfArguments);

	using Command = struct {
		std::string command;
		std::vector<int> numberOfArguments;
		CommandAction action;
	};

	std::unordered_map<std::string ,Command> m_commands{};
};

