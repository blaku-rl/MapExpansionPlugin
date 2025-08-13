#pragma once

#include "../BaseCommand.h"

class GameTimeCommand : public BaseCommand {
public:
	explicit GameTimeCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	std::string GetNetcodeIdentifier() override;
};