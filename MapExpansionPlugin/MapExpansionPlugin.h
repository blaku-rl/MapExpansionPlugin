#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "NetcodeManager/NetcodeManager.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


class MapExpansionPlugin: public BakkesMod::Plugin::BakkesModPlugin/*, public BakkesMod::Plugin::PluginWindow*/
{
	virtual void onLoad();
	virtual void onUnload();
	void OnPhysicsTick(CarWrapper cw, void* params, std::string eventName);
	void OnMessageRecieved(const std::string& Message, PriWrapper Sender);

	std::shared_ptr<NetcodeManager> Netcode;
	bool inputBlocked;
};

