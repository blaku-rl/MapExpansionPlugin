#pragma once

#include "../BaseCommand.h"

class GameStateCommand : public BaseCommand {
public:
	explicit GameStateCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	std::string GetNetcodeIdentifier() override;
};