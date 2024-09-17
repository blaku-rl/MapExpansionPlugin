#pragma once

#include "../BaseCommand.h"

class InputCommand : public BaseCommand {
public:
	explicit InputCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	std::string GetNetcodeIdentifier() override;
};