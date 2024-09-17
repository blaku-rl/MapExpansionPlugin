#pragma once

#include "BasePlugin.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "Components/BaseCommand.h"
#include <future>
#include <bakkesmod/wrappers/kismet/SequenceVariableWrapper.h>
#include "Utility/Key.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class MapExpansionPlugin: public BasePlugin, public BakkesMod::Plugin::PluginSettingsWindow
{
	virtual void onLoad();
	virtual void onUnload();

	bool isInMap = false;
	bool inputBlocked = false;
	std::vector<std::pair<std::string, std::unique_ptr<BaseCommand>>> customCommands;
	std::map<std::string, SequenceVariableWrapper> mapVariables;

	//Setup Functions
	void SetUpKeysMap();
	void CheckForSetupThreadComplete(std::string eventName);

	//Command Handling
	void OnPhysicsTick(CarWrapper cw, void* params, std::string eventName);
	void ParseCommands(const std::string& commands);

	//Exposure to commands
	void BlockInput(const bool& isBlocked) override;
	bool DoesKeyExist(const std::string& keyName) override;
	int GetIndexFromKey(const std::string& keyName) override;
	void AddKeyBind(const MapBind& bind) override;
	std::filesystem::path GetExpansionFolder() override;

	// Key press and map binds
	void OnKeyPressed(ActorWrapper aw, void* params, std::string eventName);
	void CheckForSatisfiedBinds();
	bool CheckForAllKeysPressed(const std::vector<int>& keys);

	//Management
	void SetDefaultHooks();
	void MapPluginVarCheck(std::string eventName);
	void MapUnload(std::string eventName);

	//Netcode Requirement
	void OnMessageRecieved(const std::string& Message, PriWrapper Sender);
	std::shared_ptr<NetcodeManager> Netcode;
	void NotifyPlayers(const std::string& message) override;

	//Key Press Data
	std::vector<MapBind> mapBinds = {};
	std::map<int, bool> keysPressed = {};
	std::map<std::string, int> keyNameToIndex = {};

	//Setup Thread Stuff
	std::thread setupThread;
	bool isSetupComplete = false;

	//Settings Window
	virtual void RenderSettings() override;
	virtual std::string GetPluginName() override;
	virtual void SetImGuiContext(uintptr_t ctx) override;
};

