#pragma once

#include "../BaseCommand.h"

class LoadDataCommand : public BaseCommand {
public:
	explicit LoadDataCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	std::string GetNetcodeIdentifier() override;
};