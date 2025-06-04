#pragma once

#include "../BasePlugin.h"
#include<string>
#include<vector>

class BaseCommand {
protected:
	BasePlugin *plugin;
public:
	explicit BaseCommand(BasePlugin* plugin);
	virtual void CommandFunction(const std::vector<std::string>& params) = 0;
	virtual void NetcodeHandler(const std::vector<std::string>& params) = 0;
	virtual void OnMapExit() {};
	virtual void OnPluginUnload() {};
	virtual std::string GetNetcodeIdentifier() = 0;
};