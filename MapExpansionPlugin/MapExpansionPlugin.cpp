#include "pch.h"
#include "MapExpansionPlugin.h"
#include "Components/AllCommandsInclude.h"
#include "Utility/Utils.h"
#include <bakkesmod/wrappers/kismet/SequenceWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceVariableWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceOpWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceObjectWrapper.h>

BAKKESMOD_PLUGIN(MapExpansionPlugin, "Map Expansion Plugin", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void MapExpansionPlugin::onLoad()
{
	using namespace std::placeholders;
	_globalCvarManager = cvarManager;

	customCommands.emplace_back("input", std::make_unique<InputCommand>(this));
	customCommands.emplace_back("keylisten", std::make_unique<KeyListenCommand>(this));
	customCommands.emplace_back("savedata", std::make_unique<SaveDataCommand>(this));
	customCommands.emplace_back("loaddata", std::make_unique<LoadDataCommand>(this));
	customCommands.emplace_back("remoteevent", std::make_unique<RemoteEventCommand>(this));
	customCommands.emplace_back("changescore", std::make_unique<ChangeScoreCommand>(this));
	customCommands.emplace_back("changestats", std::make_unique<ChangeStatsCommand>(this));
	customCommands.emplace_back("gamestate", std::make_unique<GameStateCommand>(this));
	customCommands.emplace_back("savedataprefix", std::make_unique<SaveDataPrefixCommand>(this));
	customCommands.emplace_back("speedrun", std::make_unique<SRCCommand>(this));

	setupThread = std::thread(&MapExpansionPlugin::SetUpKeysMap, this);
	gameWrapper->HookEventPost("Function Engine.GameViewportClient.Tick", std::bind(&MapExpansionPlugin::CheckForSetupThreadComplete, this, _1));

	if (!std::filesystem::exists(GetExpansionFolder()))
		std::filesystem::create_directory(GetExpansionFolder());
	
	SetDefaultHooks();

	//Ensure netcode is installed
	Netcode = std::make_shared<NetcodeManager>(cvarManager, gameWrapper, exports,
		std::bind(&MapExpansionPlugin::OnMessageRecieved, this, _1, _2));
}

void MapExpansionPlugin::onUnload()
{
	if (setupThread.joinable()) {
		setupThread.join();
	}
}

void MapExpansionPlugin::SetUpKeysMap()
{
	for (auto& keyName : keyNames) {
		int fnameIndex = gameWrapper->GetFNameIndexByString(keyName);
		keysPressed[fnameIndex] = false;
		keyNameToIndex[keyName] = fnameIndex;
	}
	isSetupComplete = true;
}

void MapExpansionPlugin::CheckForSetupThreadComplete(std::string eventName)
{
	if (isSetupComplete && setupThread.joinable()) {
		setupThread.join();
		gameWrapper->UnhookEventPost("Function Engine.GameViewportClient.Tick");
		LOG("Map Expansion setup complete");

		//Check for var incase plugin is loaded while in a map
		MapPluginVarCheck("");
	}
}

void MapExpansionPlugin::OnPhysicsTick(CarWrapper cw, void* params, std::string eventName)
{
	if (!isInMap) return;
	if (!gameWrapper->GetGameEventAsServer() or !isSetupComplete) { return; }

	//maybe remove for multiplayer input controls
	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;

	//Check for bm console command
	auto command = mapVariables.find("bmcommand");
	if (command != mapVariables.end() and command->second.IsString() and command->second.GetString() != "") {
		LOG("Map {} is running '{}'", gameWrapper->GetCurrentMap(), command->second.GetString());
		cvarManager->executeCommand(command->second.GetString());
		command->second.SetString("");
	}

	//Check for bm logging
	auto bmlog = mapVariables.find("bmlog");
	if (bmlog != mapVariables.end() and bmlog->second.IsString() and bmlog->second.GetString() != "") {
		LOG("Map {} says '{}'", gameWrapper->GetCurrentMap(), bmlog->second.GetString());
		bmlog->second.SetString("");
	}

	//Check for custom commands
	auto custCommand = mapVariables.find("mepcommand");
	if (custCommand != mapVariables.end() and custCommand->second.IsString() and custCommand->second.GetString() != "") {
		auto custCommandValue = custCommand->second.GetString();
		ParseCommands(custCommandValue);
		custCommand->second.SetString("");
	}

	if (inputBlocked) {
		ControllerInput* ci = (ControllerInput*)params;
		*ci = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 0, 0 };
	}
}

void MapExpansionPlugin::ParseCommands(const std::string& commands)
{
	LOG("Commands to exectue: '{}'", commands);
	auto commandList = Utils::SplitStringByChar(commands, ';');
	for (auto& command : commandList) {
		auto splitCommand = Utils::SplitStringByChar(command, ' ');
		if (splitCommand.size() == 0) continue;
		std::string commandName = splitCommand[0];
		splitCommand.erase(splitCommand.begin());

		bool commandFound = false;
		for (auto& [id, command] : customCommands) {
			if (id == commandName) {
				command->CommandFunction(splitCommand);
				commandFound = true;
				break;
			}
		}

		if (!commandFound)
			LOG("Command '{}' was not found.", commandName);
	}
}

void MapExpansionPlugin::BlockInput(const bool& isBlocked)
{
	inputBlocked = isBlocked;
}

bool MapExpansionPlugin::DoesKeyExist(const std::string& keyName) const
{
	return keyNameToIndex.find(keyName) != keyNameToIndex.end();
}

int MapExpansionPlugin::GetIndexFromKey(const std::string& keyName) const
{
	return keyNameToIndex.at(keyName);
}

void MapExpansionPlugin::AddKeyBind(const MapBind& bind)
{
	mapBinds.push_back(bind);
}

void MapExpansionPlugin::SendInfoToMap(const std::string& str)
{
	if (str.size() > 100)
		LOG("mepoutput is {} characters, showing first 100 here {}", std::to_string(str.size()), str.substr(0, 100));
	else
		LOG("Setting mepoutput to {}", str);

	auto outVar = mapVariables.find("mepoutput");
	if (outVar != mapVariables.end() and outVar->second.IsString())
		outVar->second.SetString(str);
	ActivateRemoteEvent("MEPOutputEvent");
}

std::filesystem::path MapExpansionPlugin::GetExpansionFolder() const
{
	return gameWrapper->GetDataFolder() / "expansion";
}

std::map<std::string, SequenceVariableWrapper>& MapExpansionPlugin::GetMapVariables()
{
	return mapVariables;
}

void MapExpansionPlugin::ActivateRemoteEvent(const std::string& eventName) const
{
	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;
	LOG("Activating remote event '{}'", eventName);
	sequence.ActivateRemoteEvents(eventName);
}

void MapExpansionPlugin::OnKeyPressed(ActorWrapper aw, void* params, std::string eventName)
{
	KeyPressParams* keyPressData = (KeyPressParams*)params;
	if (keysPressed.find(keyPressData->Key.Index) != keysPressed.end()) {
		keysPressed[keyPressData->Key.Index] = keyPressData->EventType != EInputEvent::Released;
	}
	else {
		return;
	}

	CheckForSatisfiedBinds();
}

void MapExpansionPlugin::CheckForSatisfiedBinds()
{
	for (auto& binding : mapBinds) {
		bool keysPressedForBinding = CheckForAllKeysPressed(binding.keyListFnameIndex);
		if (keysPressedForBinding and !binding.allKeysPressed) {
			binding.allKeysPressed = true;
			ActivateRemoteEvent(binding.remoteEvent);
		}
		else if (!keysPressedForBinding and binding.allKeysPressed) {
			binding.allKeysPressed = false;
		}
	}
}

bool MapExpansionPlugin::CheckForAllKeysPressed(const std::vector<int>& keys)
{
	if (keys.size() == 0) { return false; }
	for (auto& fnameIndex : keys) {
		if (!keysPressed[fnameIndex])
			return false;
	}
	return true;
}

void MapExpansionPlugin::SetDefaultHooks()
{
	using namespace std::placeholders;
	gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.SetVehicleInput", std::bind(&MapExpansionPlugin::OnPhysicsTick, this, _1, _2, _3));
	gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GameViewportClient_TA.HandleKeyPress", std::bind(&MapExpansionPlugin::OnKeyPressed, this, _1, _2, _3));
	gameWrapper->HookEventPost("Function Engine.HUD.SetShowScores", std::bind(&MapExpansionPlugin::MapPluginVarCheck, this, _1));
	gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.Destroyed", std::bind(&MapExpansionPlugin::MapUnload, this, _1));
}

void MapExpansionPlugin::MapPluginVarCheck(std::string eventName)
{
	if (isInMap) return;
	if (!gameWrapper->GetGameEventAsServer()) { return; }
	auto sequence = gameWrapper->GetMainSequence();
	mapVariables = sequence.GetAllSequenceVariables(false);
	ActivateRemoteEvent("MEPLoaded");
	LOG("Map Expansion Plugin Loaded");
	isInMap = true;
}

void MapExpansionPlugin::MapUnload(std::string eventName)
{
	inputBlocked = false;
	isInMap = false;
	mapBinds.clear();
}

void MapExpansionPlugin::OnMessageRecieved(const std::string& Message, PriWrapper Sender)
{
	if (!Sender or Message.empty()) return;
	auto parsedMessage = Utils::SplitStringByChar(Message, ' ');
	if (parsedMessage.size() == 0) return;
	std::string commandId = parsedMessage[0];
	parsedMessage.erase(parsedMessage.begin());


	for (auto& [id, command] : customCommands) {
		if (command->GetNetcodeIdentifier() == commandId) {
			command->NetcodeHandler(parsedMessage);
			return;
		}
	}
}

void MapExpansionPlugin::NotifyPlayers(const std::string& message) 
{
	Netcode->SendNewMessage(message);
}

void MapExpansionPlugin::RenderSettings()
{
	if (!isSetupComplete) {
		ImGui::TextUnformatted("Map Expansion Plugin is still loading");
	}
	else {
		ImGui::TextUnformatted("The Map Expansion Plugin is designed for map makers to leverage bakkesmod with their maps");
		ImGui::TextUnformatted("Usage Details can be found here: https://github.com/blaku-rl/MapExpansionPlugin");
	}
}

std::string MapExpansionPlugin::GetPluginName()
{
	return "Map Expansion Plugin";
}

// Don't call this yourself, BM will call this function with a pointer to the current ImGui context
void MapExpansionPlugin::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}
