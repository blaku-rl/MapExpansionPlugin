#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "NetcodeManager/NetcodeManager.h"
#include "Utility/InputControls.h"
#include "Utility/Key.h"
#include <memory>
#include <filesystem>

class BasePlugin : public BakkesMod::Plugin::BakkesModPlugin
{
public:
	virtual void NotifyPlayers(const std::string& message) = 0;
	virtual void BlockInput(const bool& isBlocked) = 0;
	virtual bool DoesKeyExist(const std::string& keyName) = 0;
	virtual int GetIndexFromKey(const std::string& keyName) = 0;
	virtual void AddKeyBind(const MapBind& bind) = 0;
	virtual std::filesystem::path GetExpansionFolder() = 0;
};
