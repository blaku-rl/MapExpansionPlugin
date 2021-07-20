#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "NetcodeManager/NetcodeManager.h"
#include "Key.h"
#include <functional>

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class MapExpansionPlugin: public BakkesMod::Plugin::BakkesModPlugin/*, public BakkesMod::Plugin::PluginWindow*/
{
	virtual void onLoad();
	virtual void onUnload();
	void SetUpKeysMap();

	//Command Handling
	void OnPhysicsTick(CarWrapper cw, void* params, std::string eventName);
	void InputBlockCommand(std::vector<std::string> params);
	void KeyListenCommand(std::vector<std::string> params);

	// Key press and map binds
	void OnKeyPressed(ActorWrapper aw, void* params, std::string eventName);
	void CheckForSatisfiedBinds();
	bool CheckForAllKeysPressed(std::vector<int>& keys);

	//Management
	void MapPluginVarCheck(std::string eventName);
	void MapUnload(std::string eventName);

	//Required
	void OnMessageRecieved(const std::string& Message, PriWrapper Sender);
	std::shared_ptr<NetcodeManager> Netcode;

	//Custom command implementation
	std::map<std::string, std::function<void(std::vector<std::string> params)>> custCommands;

	//Key Data
	std::vector<std::shared_ptr<MapBind>> mapBinds;
	std::map<int, bool> keysPressed = {};
	std::map<std::string, bool> validKeys = {};

	bool inputBlocked;
	std::string loadedMap = "";
};

