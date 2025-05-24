#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "NetcodeManager/NetcodeManager.h"
#include "Utility/InputControls.h"
#include "Utility/Key.h"
#include <memory>
#include <filesystem>
#include <bakkesmod/wrappers/kismet/SequenceVariableWrapper.h>

class BasePlugin : public BakkesMod::Plugin::BakkesModPlugin
{
public:
	virtual void NotifyPlayers(const std::string& message) = 0;
	virtual void BlockInput(const bool& isBlocked) = 0;
	virtual bool DoesKeyExist(const std::string& keyName) const = 0;
	virtual int GetIndexFromKey(const std::string& keyName) const = 0;
	virtual void AddKeyBind(const MapBind& bind) = 0;
	virtual void SendInfoToMap(const std::string& str) = 0;
	virtual std::filesystem::path GetExpansionFolder() const = 0;
	virtual std::map<std::string, SequenceVariableWrapper>& GetMapVariables() = 0;
	virtual void ActivateRemoteEvent(const std::string& eventName) const = 0;
	virtual constexpr const char* GetPluginVersion() const = 0;
};
