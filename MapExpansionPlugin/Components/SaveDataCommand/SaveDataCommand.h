#pragma once

#include "../BaseCommand.h"

class SaveDataCommand : public BaseCommand {
public:
	explicit SaveDataCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	std::string GetNetcodeIdentifier() override;
};