#include "pch.h"
#include "Command.h"
#include "../Utility/Utils.h"

Command::Command(std::string_view command)
{
	fullCommand = command;
}

std::string_view Command::GetFullCommand() const
{
	return fullCommand;
}

std::string_view Command::GetCommandName() const
{
	return fullCommand.substr(0, GetCommandSplitPosition());
}

std::string_view Command::GetCommandParams() const
{
	auto splitPos = GetCommandSplitPosition();
	if (splitPos < fullCommand.size())
		return fullCommand.substr(splitPos + 1);

	//TODO: What todo when no params are available
	return fullCommand.substr(fullCommand.size());
}

std::vector<std::string_view> Command::GetSplitCommandParams() const
{
	return Utils::SplitStrViewByChar(GetCommandParams());
}

std::string Command::GetFullCommandStr() const
{
	return std::string(GetFullCommand());
}

std::string Command::GetCommandNameStr() const
{
	return std::string(GetCommandName());
}

std::string Command::GetCommandParamsStr() const
{
	return std::string(GetCommandParams());
}

std::vector<std::string> Command::GetSplitCommandParamsStr() const
{
	return Utils::SplitStringByChar(std::string(GetCommandParams()));
}

size_t Command::GetCommandSplitPosition() const
{
	return fullCommand.find_first_of(' ');
}
