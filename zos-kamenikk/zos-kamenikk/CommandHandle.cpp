#include "CommandHandle.h"
#include <string>
#include <ranges>
#include <string_view>

void CommandHandle::GetCommandString(std::string input, std::string& command, int& numberOfArg, std::vector<std::string>& args)
{
	auto split_view = input | std::views::split(' ');

	for (auto&& word : split_view) {
		args.emplace_back(word.begin(), word.end());
		args.back() += "\0";
	}

	command = args[0];
	args.erase(args.begin());

	numberOfArg = static_cast<int>(args.size());
}

void CommandHandle::AddCommand(std::string command, std::vector<int> numberOfArguments, CommandAction action)
{
	m_commands.insert(std::make_pair(command, Command{ command, numberOfArguments, action }));
}

bool CommandHandle::CheckCommand(std::string command, int numberOfArguments)
{
	if (m_commands.contains(command)) {
		for (auto& argc : m_commands[command].numberOfArguments) {
			if (argc == numberOfArguments) {
				return true;
			}
		}
	}
	return false;;
}

void CommandHandle::Execute(std::string command, int numberOfArguments, std::string* arguments)
{
	if (CheckCommand(command, numberOfArguments)) {
		m_commands[command].action(numberOfArguments, arguments);
	}
}
