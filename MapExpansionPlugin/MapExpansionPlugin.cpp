#include "pch.h"
#include "MapExpansionPlugin.h"
#include <bakkesmod/wrappers/kismet/SequenceWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceVariableWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceOpWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceObjectWrapper.h>
#include <sstream>

BAKKESMOD_PLUGIN(MapExpansionPlugin, "Plugin for expanding map creators capabilities", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void MapExpansionPlugin::onLoad()
{
	using namespace std::placeholders;
	_globalCvarManager = cvarManager;

	custCommands.insert({ "input", std::bind(&MapExpansionPlugin::InputBlockCommand, this, _1) });
	custCommands.insert({ "keylisten", std::bind(&MapExpansionPlugin::KeyListenCommand, this, _1) });
	mapBinds = {};
	SetUpKeysMap();

	//Hooks
	gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.SetVehicleInput", std::bind(&MapExpansionPlugin::OnPhysicsTick, this, _1, _2, _3));
	gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GameViewportClient_TA.HandleKeyPress", std::bind(&MapExpansionPlugin::OnKeyPressed, this, _1, _2, _3));
	gameWrapper->HookEventPost("Function ProjectX.EngineShare_X.EventPostLoadMap", std::bind(&MapExpansionPlugin::MapPluginVarCheck, this, _1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", std::bind(&MapExpansionPlugin::MapUnload, this, _1));

	//Ensure netcode is installed
	Netcode = std::make_shared<NetcodeManager>(cvarManager, gameWrapper, exports,
		std::bind(&MapExpansionPlugin::OnMessageRecieved, this, _1, _2));

	//Initialize
	inputBlocked = false;

	//Check for var incase plugin is loaded while in a map
	MapPluginVarCheck("");
}

void MapExpansionPlugin::onUnload()
{
}

void MapExpansionPlugin::SetUpKeysMap()
{
	for (int i = 1; i < keyNames.size(); i++) {
		int fnameIndex = gameWrapper->GetFNameIndexByString(keyNames[i]);
		keysPressed.insert(std::pair<int, bool>(fnameIndex, false ));
	}
}

void MapExpansionPlugin::OnPhysicsTick(CarWrapper cw, void* params, std::string eventName)
{
	if (!gameWrapper->GetGameEventAsServer()) { return; }
	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;

	auto allVars = sequence.GetAllSequenceVariables(false);

	//Check for bm console command
	auto command = allVars.find("bmcommand");
	if (command != allVars.end() && command->second.IsString() && command->second.GetString() != "") {
		cvarManager->log("Map " + gameWrapper->GetCurrentMap() + " is running: " + command->second.GetString());
		cvarManager->executeCommand(command->second.GetString());
		command->second.SetString("");
	}

	//Check for bm logging
	auto bmlog = allVars.find("bmlog");
	if (bmlog != allVars.end() && bmlog->second.IsString() && bmlog->second.GetString() != "") {
		cvarManager->log("Map " + gameWrapper->GetCurrentMap() + " says: " + bmlog->second.GetString());
		bmlog->second.SetString("");
	}

	//Check for custom commands
	auto custCommand = allVars.find("customcommand");
	if (custCommand != allVars.end() && custCommand->second.IsString() && custCommand->second.GetString() != "") {
		auto custCommandValue = custCommand->second.GetString();
		cvarManager->log("Map " + gameWrapper->GetCurrentMap() + " is running command " + custCommandValue);

		std::stringstream commandStream(custCommandValue);
		std::string commandName;
		std::getline(commandStream, commandName, ' ');

		std::vector<std::string> commandArgs;
		std::string segment;
		while (std::getline(commandStream, segment, ' ')) {
			commandArgs.push_back(segment);
		}

		if (custCommands.find(commandName) == custCommands.end()) {
			cvarManager->log("Command " + commandName + " was not found.");
		}
		else {
			custCommands[commandName](commandArgs);
		}
		custCommand->second.SetString("");
	}

	if (inputBlocked) {
		ControllerInput* ci = (ControllerInput*)params;
		*ci = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0};
	}
}

void MapExpansionPlugin::InputBlockCommand(std::vector<std::string> params)
{
	if (params.size() != 1) {
		cvarManager->log("Input command expects 1 parameter. Either block or allow");
		return;
	}

	if (params[0] == "block") {
		cvarManager->log("Map " + gameWrapper->GetCurrentMap() + " is blocking input");
		inputBlocked = true;
	}
	else if (params[0] == "allow") {
		cvarManager->log("Map " + gameWrapper->GetCurrentMap() + " is allowing input");
		inputBlocked = false;
	}
	else {
		cvarManager->log("Param " + params[0] + " is not a valid paramater for input command. Use block or allow");
	}
}

void MapExpansionPlugin::KeyListenCommand(std::vector<std::string> params)
{
	if (params.size() != 2) {
		cvarManager->log(
			L"keylisten command expects 2 parameters. First is a comma separated list of keys that are required."
			"The second is the name of the remote event that will be ran once the required keys are pressed");
		return;
	}

	MapBind curBind;
	std::stringstream keysStream(params[0]);
	std::string key;
	while (std::getline(keysStream, key, ',')) {
		auto keyIndex = gameWrapper->GetFNameIndexByString(key);
		curBind.keyListFnameIndex.push_back(keyIndex);
	}
	curBind.remoteEvent = params[1];
	curBind.allKeysPressed = false;

	mapBinds.push_back(std::make_shared<MapBind>(curBind));
	cvarManager->log("Making bind with keys: " + params[0] + " and remote event: " + params[1]);
}

void MapExpansionPlugin::OnKeyPressed(ActorWrapper aw, void* params, std::string eventName)
{
	KeyPressParams* keyPressData = (KeyPressParams*)params;
	if (keysPressed.find(keyPressData->Key.Index) != keysPressed.end()) {
		keysPressed[keyPressData->Key.Index] = keyPressData->EventType != 1;
	}
	else {
		cvarManager->log("Key with Index " + std::to_string(keyPressData->Key.Index) + " not found");
		return;
	}

	CheckForSatisfiedBinds();
}

void MapExpansionPlugin::CheckForSatisfiedBinds()
{
	for (auto& binding : mapBinds) {
		bool keysPressedForBinding = CheckForAllKeysPressed(binding->keyListFnameIndex);
		if (keysPressedForBinding && !binding->allKeysPressed) {
			binding->allKeysPressed = true;
			auto sequence = gameWrapper->GetMainSequence();
			if (sequence.memory_address == NULL) return;

			cvarManager->log("Activating remote event " + binding->remoteEvent);
			sequence.ActivateRemoteEvents(binding->remoteEvent);
		}
		else if (!keysPressedForBinding && binding->allKeysPressed) {
			binding->allKeysPressed = false;
		}
	}
}

bool MapExpansionPlugin::CheckForAllKeysPressed(std::vector<int>& keys)
{
	if (keys.size() == 0) { return false; }
	for (auto& fnameIndex : keys) {
		if (!keysPressed[fnameIndex])
			return false;
	}
	return true;
}

void MapExpansionPlugin::MapPluginVarCheck(std::string eventName)
{
	if (!gameWrapper->GetGameEventAsServer()) { return; }
	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) { return; }

	auto allVars = sequence.GetAllSequenceVariables(false);
	auto pluginCheck = allVars.find("mappluginenabled");
	if (pluginCheck == allVars.end() || !pluginCheck->second.IsBool()) { return; }
	pluginCheck->second.SetBool(true);
}

void MapExpansionPlugin::MapUnload(std::string eventName)
{
	mapBinds = {};
}

void MapExpansionPlugin::OnMessageRecieved(const std::string& Message, PriWrapper Sender)
{
}
