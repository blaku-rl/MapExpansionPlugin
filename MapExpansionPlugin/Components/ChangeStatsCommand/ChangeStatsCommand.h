#pragma once

#include "../BaseCommand.h"
#include <set>

class ChangeStatsCommand : public BaseCommand {
	std::set<std::string> validStats = { "assists", "goals", "saves", "score", "shots" };
	std::set<std::string> validOpps = { "add", "sub" };
public:
	explicit ChangeStatsCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	std::string GetNetcodeIdentifier() override;
};