#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "NetcodeManager/NetcodeManager.h"
#include <future>
#include "Key.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class MapExpansionPlugin: public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow
{
	virtual void onLoad();
	virtual void onUnload();

	bool isInMap = false;

	//Setup Functions
	void SetUpKeysMap();
	void CheckForSetupThreadComplete(std::string eventName);

	//Command Handling
	void OnPhysicsTick(CarWrapper cw, void* params, std::string eventName);
	void ParseCommands(const std::string& commands);
	void InputBlockCommand(const std::vector<std::string>& params);
	void KeyListenCommand(const std::vector<std::string>& params);
	void SaveDataCommand(const std::vector<std::string>& params);
	void LoadDataCommand(const std::vector<std::string>& params);
	void RemoteEventCommand(const std::vector<std::string>& params);
	void ChangeScoreCommand(const std::vector<std::string>& params);
	void ChangeStatsCommand(const std::vector<std::string>& params);
	void UpdatePlayerStats(const int& playerId, const int& modifiedAmount, const std::string& statType);
	void ChangeGameState(const std::vector<std::string>& params);
	void PlaySoundHandler(const std::vector<std::string>& params);
	void PlaySoundFromFile(const std::string& wavFile);
	void StopSoundHandler(const std::vector<std::string>& params);
	void StopSound();

	// Key press and map binds
	void OnKeyPressed(ActorWrapper aw, void* params, std::string eventName);
	void CheckForSatisfiedBinds();
	bool CheckForAllKeysPressed(const std::vector<int>& keys);

	//Management
	void MapPluginVarCheck(std::string eventName);
	void MapUnload(std::string eventName);
	bool IsNumeric(const std::string& str);
	bool IsAlphaNumeric(const std::string& str);
	std::vector<std::string> SplitStringByChar(const std::string& str, const char& sep);
	std::filesystem::path expansionFolder = gameWrapper->GetDataFolder() / "expansion";

	//Netcode Requirement
	void OnMessageRecieved(const std::string& Message, PriWrapper Sender);
	bool IsMyId(const std::string& idString);
	std::shared_ptr<NetcodeManager> Netcode;

	//Custom command implementation
	std::map<std::string, std::function<void(std::vector<std::string> params)>> custCommands;

	//Key Press Data
	std::vector<MapBind> mapBinds = {};
	std::map<int, bool> keysPressed = {};
	std::map<std::string, int> keyNameToIndex = {};

	//Setup Thread Stuff
	std::thread setupThread;
	bool isSetupComplete = false;

	//Command vars
	bool inputBlocked = false;

	std::string nameTag = "KismetVar:";
	std::string typeTag = "Type:";
	std::string valueTag = "Value:";

	//Settings Window
	virtual void RenderSettings() override;
	virtual std::string GetPluginName() override;
	virtual void SetImGuiContext(uintptr_t ctx) override;
};

