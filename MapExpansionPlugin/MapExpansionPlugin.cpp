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

	setupThread = std::thread(&MapExpansionPlugin::SetUpKeysMap, this);

	//Hooks
	gameWrapper->HookEventPost("Function Engine.GameViewportClient.Tick", std::bind(&MapExpansionPlugin::CheckForSetupThreadComplete, this, std::placeholders::_1));
	gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.SetVehicleInput", std::bind(&MapExpansionPlugin::OnPhysicsTick, this, _1, _2, _3));
	gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GameViewportClient_TA.HandleKeyPress", std::bind(&MapExpansionPlugin::OnKeyPressed, this, _1, _2, _3));
	gameWrapper->HookEventPost("Function ProjectX.EngineShare_X.EventPostLoadMap", std::bind(&MapExpansionPlugin::MapPluginVarCheck, this, _1));
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed", std::bind(&MapExpansionPlugin::MapUnload, this, _1));

	//Ensure netcode is installed
	Netcode = std::make_shared<NetcodeManager>(cvarManager, gameWrapper, exports,
		std::bind(&MapExpansionPlugin::OnMessageRecieved, this, _1, _2));
}

void MapExpansionPlugin::onUnload()
{
	//No crashes on updates...maybe
	if (setupThread.joinable()) {
		setupThread.join();
	}
}

void MapExpansionPlugin::SetUpKeysMap()
{
	for (int i = 1; i < keyNames.size(); i++) {
		int fnameIndex = gameWrapper->GetFNameIndexByString(keyNames[i]);
		keysPressed.insert(std::pair<int, bool>(fnameIndex, false ));
		validKeys.insert(std::pair<std::string, bool>(keyNames[i], false));
	}
	isSetupComplete = true;
}

void MapExpansionPlugin::CheckForSetupThreadComplete(std::string eventName)
{
	if (isSetupComplete && setupThread.joinable()) {
		setupThread.join();
		gameWrapper->UnhookEventPost("Function Engine.GameViewportClient.Tick");
		cvarManager->log("Map Expansion setup complete");

		//Check for var incase plugin is loaded while in a map
		MapPluginVarCheck("");
	}
}

void MapExpansionPlugin::OnPhysicsTick(CarWrapper cw, void* params, std::string eventName)
{
	if (!gameWrapper->GetGameEventAsServer() || !isSetupComplete) { return; }
	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;

	auto allVars = sequence.GetAllSequenceVariables(false);

	//Check for bm console command
	auto command = allVars.find("bmcommand");
	if (command != allVars.end() && command->second.IsString() && command->second.GetString() != "") {
		cvarManager->log("Map " + gameWrapper->GetCurrentMap() + " is running '" + command->second.GetString() + "'");
		cvarManager->executeCommand(command->second.GetString());
		command->second.SetString("");
	}

	//Check for bm logging
	auto bmlog = allVars.find("bmlog");
	if (bmlog != allVars.end() && bmlog->second.IsString() && bmlog->second.GetString() != "") {
		cvarManager->log("Map " + gameWrapper->GetCurrentMap() + " says '" + bmlog->second.GetString() + "'");
		bmlog->second.SetString("");
	}

	//Check for custom commands
	auto custCommand = allVars.find("customcommand");
	if (custCommand != allVars.end() && custCommand->second.IsString() && custCommand->second.GetString() != "") {
		auto custCommandValue = custCommand->second.GetString();
		ParseCommands(custCommandValue);
		custCommand->second.SetString("");
	}

	if (inputBlocked) {
		ControllerInput* ci = (ControllerInput*)params;
		*ci = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0};
	}
}

void MapExpansionPlugin::ParseCommands(std::string commands)
{
	std::stringstream commandsStream(commands);
	std::string commandValue;
	while (std::getline(commandsStream, commandValue, ';')) {
		cvarManager->log("Map " + gameWrapper->GetCurrentMap() + " is running command '" + commandValue + "'");

		std::stringstream commandStream(commandValue);
		std::string commandName;
		std::getline(commandStream, commandName, ' ');

		std::vector<std::string> commandArgs;
		std::string segment;
		while (std::getline(commandStream, segment, ' ')) {
			commandArgs.push_back(segment);
		}

		if (custCommands.find(commandName) == custCommands.end()) {
			cvarManager->log("Command '" + commandName + "' was not found.");
		}
		else {
			custCommands[commandName](commandArgs);
		}
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
			L"keylisten command expects 2 parameters separated by spaces. First is a comma separated list of keys that are required."
			"The second is the name of the remote event that will be ran once the required keys are pressed");
		return;
	}

	MapBind curBind;
	std::stringstream keysStream(params[0]);
	std::string key;
	while (std::getline(keysStream, key, ',')) {
		if (validKeys.find(key) == validKeys.end()) {
			cvarManager->log("Key '" + key + "' is not a supported key. View documentation for supported key names. keylisten command will not be added");
			return;
		}
		//Might cause a little lag until new update with cached values
		auto keyIndex = gameWrapper->GetFNameIndexByString(key);
		curBind.keyListFnameIndex.push_back(keyIndex);
	}
	curBind.remoteEvent = params[1];
	curBind.allKeysPressed = false;

	mapBinds.push_back(std::make_shared<MapBind>(curBind));
	cvarManager->log("Making bind with keys '" + params[0] + "' and remote event '" + params[1] + "'");
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

			cvarManager->log("Activating remote event '" + binding->remoteEvent + "'");
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
	cvarManager->log("Setting 'mappluginenabled' var to true");
}



void MapExpansionPlugin::MapUnload(std::string eventName)
{
	inputBlocked = false;
	mapBinds.clear();
}

void MapExpansionPlugin::OnMessageRecieved(const std::string& Message, PriWrapper Sender)
{
}
