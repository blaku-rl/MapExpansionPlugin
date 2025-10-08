#pragma once
#include <vector>
#include <string>
#include <string_view>

class Command {
	std::string_view fullCommand;

public:
	Command(std::string_view command);

	std::string_view GetFullCommand() const;
	std::string_view GetCommandName() const;
	std::string_view GetCommandParams() const;
	std::vector<std::string_view> GetSplitCommandParams() const;
	
	std::string GetFullCommandStr() const;
	std::string GetCommandNameStr() const;
	std::string GetCommandParamsStr() const;
	std::vector<std::string> GetSplitCommandParamsStr() const;
private:
	size_t GetCommandSplitPosition() const;
};