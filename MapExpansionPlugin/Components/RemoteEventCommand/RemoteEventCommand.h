#pragma once

#include "../BaseCommand.h"

class RemoteEventCommand : public BaseCommand {
public:
	explicit RemoteEventCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	std::string GetNetcodeIdentifier() override;
};