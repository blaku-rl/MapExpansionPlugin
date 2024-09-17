#pragma once

#include "../BaseCommand.h"

class ChangeScoreCommand : public BaseCommand {
public:
	explicit ChangeScoreCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	std::string GetNetcodeIdentifier() override;
};