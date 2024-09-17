#pragma once

#include "../BaseCommand.h"

class KeyListenCommand : public BaseCommand {
public:
	explicit KeyListenCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	std::string GetNetcodeIdentifier() override;
};