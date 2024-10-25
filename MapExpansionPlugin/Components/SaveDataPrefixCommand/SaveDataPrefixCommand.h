#pragma once

#include "../BaseCommand.h"

class SaveDataPrefixCommand : public BaseCommand {
public:
	explicit SaveDataPrefixCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	std::string GetNetcodeIdentifier() override;
};