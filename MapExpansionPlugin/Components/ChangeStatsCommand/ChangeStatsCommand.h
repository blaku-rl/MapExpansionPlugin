#pragma once

#include "../BaseCommand.h"

class ChangeStatsCommand : public BaseCommand {
public:
	explicit ChangeStatsCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	std::string GetNetcodeIdentifier() override;
};