#include "pch.h"
#include "MapExpansionPlugin.h"
#include <bakkesmod/wrappers/kismet/SequenceWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceVariableWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceOpWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceObjectWrapper.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

BAKKESMOD_PLUGIN(MapExpansionPlugin, "Plugin for expanding map creators capabilities", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void MapExpansionPlugin::onLoad()
{
	using namespace std::placeholders;
	_globalCvarManager = cvarManager;

	custCommands["input"] = std::bind(&MapExpansionPlugin::InputBlockCommand, this, _1);
	custCommands["keylisten"] = std::bind(&MapExpansionPlugin::KeyListenCommand, this, _1);
	custCommands["savedata"] = std::bind(&MapExpansionPlugin::SaveDataCommand, this, _1);
	custCommands["loaddata"] = std::bind(&MapExpansionPlugin::LoadDataCommand, this, _1);
	custCommands["remoteevent"] = std::bind(&MapExpansionPlugin::RemoteEventCommand, this, _1);
	custCommands["changescore"] = std::bind(&MapExpansionPlugin::ChangeScoreCommand, this, _1);
	custCommands["changestats"] = std::bind(&MapExpansionPlugin::ChangeStatsCommand, this, _1);
	custCommands["gamestate"] = std::bind(&MapExpansionPlugin::ChangeGameState, this, _1);
	custCommands["playsound"] = std::bind(&MapExpansionPlugin::PlaySoundHandler, this, _1);
	custCommands["stopsound"] = std::bind(&MapExpansionPlugin::StopSoundHandler, this, _1);

	setupThread = std::thread(&MapExpansionPlugin::SetUpKeysMap, this);
	if (!std::filesystem::exists(expansionFolder))
		std::filesystem::create_directory(expansionFolder);

	//Hooks
	gameWrapper->HookEventPost("Function Engine.GameViewportClient.Tick", std::bind(&MapExpansionPlugin::CheckForSetupThreadComplete, this, std::placeholders::_1));
	gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.SetVehicleInput", std::bind(&MapExpansionPlugin::OnPhysicsTick, this, _1, _2, _3));
	gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.GameViewportClient_TA.HandleKeyPress", std::bind(&MapExpansionPlugin::OnKeyPressed, this, _1, _2, _3));
	gameWrapper->HookEventPost("Function Engine.HUD.SetShowScores", std::bind(&MapExpansionPlugin::MapPluginVarCheck, this, _1));
	gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.Destroyed", std::bind(&MapExpansionPlugin::MapUnload, this, _1));

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
		cvarManager->log("Map Expansion setup complete");

		//Check for var incase plugin is loaded while in a map
		MapPluginVarCheck("");
	}
}

void MapExpansionPlugin::OnPhysicsTick(CarWrapper cw, void* params, std::string eventName)
{
	if (!isInMap) return;
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
	auto custCommand = allVars.find("mepcommand");
	if (custCommand != allVars.end() && custCommand->second.IsString() && custCommand->second.GetString() != "") {
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
	cvarManager->log("Commands to exectue: '" + commands + "'");
	auto commandList = SplitStringByChar(commands, ';');
	for (auto& command : commandList) {
		auto splitCommand = SplitStringByChar(command, ' ');
		if (splitCommand.size() == 0) continue;
		std::string commandName = splitCommand[0];
		splitCommand.erase(splitCommand.begin());

		if (custCommands.find(commandName) == custCommands.end()) {
			cvarManager->log("Command '" + commandName + "' was not found.");
		}
		else {
			custCommands[commandName](splitCommand);
		}
	}
}

void MapExpansionPlugin::InputBlockCommand(const std::vector<std::string>& params)
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

void MapExpansionPlugin::KeyListenCommand(const std::vector<std::string>& params)
{
	if (params.size() != 2) {
		cvarManager->log(
			L"keylisten command expects 2 parameters separated by spaces. First is a comma separated list(without spaces) of keys that are required."
			"The second is the name of the remote event that will be called once the required keys are pressed");
		return;
	}

	MapBind curBind;
	auto keyList = SplitStringByChar(params[0], ',');
	for (auto& key : keyList) {
		if (keyNameToIndex.find(key) == keyNameToIndex.end()) {
			cvarManager->log("Key '" + key + "' is not a supported key. View documentation for supported key names. keylisten command will not be added");
			return;
		}
		curBind.keyListFnameIndex.push_back(keyNameToIndex[key]);
	}

	curBind.remoteEvent = params[1];
	curBind.allKeysPressed = false;

	mapBinds.push_back(curBind);
	cvarManager->log("Making bind with keys '" + params[0] + "' and remote event '" + params[1] + "'");
}

void MapExpansionPlugin::SaveDataCommand(const std::vector<std::string>& params)
{
	if (params.size() != 2) {
		cvarManager->log(
			L"savedata command expects 2 parameters. The first is a comma separated list(without spaces) of kismet variables to save. "
			"Currently only bools, floats, ints, strings, and vectors are supported for saving. The variable must be defined on the main sequence to be found. "
			"The second is the name of the file that you would like to save too. The file name parameter only accepts alpha-numeric characters for a valid file name.");
		return;
	}

	const std::string& fileName = params[1];
	if (!IsAlphaNumeric(fileName)) {
		cvarManager->log("Invalid File Name: \"" + fileName + "\". Only alpha-numeric characters are allowed for a file name.");
		return;
	}

	auto kismetVars = SplitStringByChar(params[0], ',');

	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;
	auto allVars = sequence.GetAllSequenceVariables(false);

	std::ofstream dataFile(expansionFolder / (fileName + ".data"));
	if (dataFile.is_open()) {
		for (auto& curVar : kismetVars) {
			auto mapSeqVar = allVars.find(curVar);
			if (mapSeqVar == allVars.end()) { 
				cvarManager->log("Kismet Variable " + curVar + "was not found in the main sequence. Variable will not be saved");
				continue; 
			}
			dataFile << "{\n";
			dataFile << "    " << nameTag << curVar << "\n";
			auto mapVar = allVars.find(curVar)->second;
			if (mapVar.IsBool()) {
				dataFile << "    " << typeTag << "Bool\n";
				dataFile << "    " << valueTag << (mapVar.GetBool() ? "1" : "0") << "\n";
			}
			else if (mapVar.IsFloat()) {
				dataFile << "    " << typeTag << "Float\n";
				dataFile << "    " << valueTag << mapVar.GetFloat() << "\n";
			}
			else if (mapVar.IsInt()) {
				dataFile << "    " << typeTag << "Int\n";
				dataFile << "    " << valueTag << mapVar.GetInt() << "\n";
			}
			else if (mapVar.IsString()) {
				dataFile << "    " << typeTag << "String\n";
				dataFile << "    " << valueTag << mapVar.GetString() << "\n";
			}
			else if (mapVar.IsVector()) {
				dataFile << "    " << typeTag << "Vector\n";
				Vector vec = mapVar.GetVector();
				dataFile << "    " << valueTag << vec.X << "," << vec.Y << "," << vec.Z << "\n";
			}
			dataFile << "}\n";
		}
	}
	dataFile.close();

	cvarManager->log("File " + params[1] + ".data has been saved");
	sequence.ActivateRemoteEvents("MEPDataSaved");
}

void MapExpansionPlugin::LoadDataCommand(const std::vector<std::string>& params)
{
	if (params.size() != 1) {
		cvarManager->log(L"loaddata command expects 1 parameter. It should be the name of the file that you have saved data to. "
			"The file name parameter only accepts alpha-numeric characters for a valid file name.");
		return;
	}

	const std::string& fileName = params[0];
	if (!IsAlphaNumeric(fileName)) {
		cvarManager->log("Invalid File Name: \"" + fileName + "\". Only alpha-numeric characters are allowed for a file name.");
		return;
	}

	auto saveDataPath = expansionFolder / (fileName + ".data");
	if (!std::filesystem::exists(saveDataPath)) {
		cvarManager->log("File " + fileName + ".data doesn't exist. Ignore if this is first time loading the map");
	}

	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;
	auto allVars = sequence.GetAllSequenceVariables(false);

	std::ifstream dataFile(saveDataPath);
	if (dataFile.is_open()) {
		std::string line;
		while (std::getline(dataFile, line)) {
			if (line != "{") {
				break;
			}
			std::string varName = "";
			std::string varType = "";
			std::string varValue = "";

			std::getline(dataFile, line);
			if (line.find(nameTag) != std::string::npos) {
				varName = line.substr(line.find(nameTag) + nameTag.size(), std::string::npos);
			}

			std::getline(dataFile, line);
			if (line.find(typeTag) != std::string::npos) {
				varType = line.substr(line.find(typeTag) + typeTag.size(), std::string::npos);
			}

			std::getline(dataFile, line);
			if (line.find(valueTag) != std::string::npos) {
				varValue = line.substr(line.find(valueTag) + valueTag.size(), std::string::npos);
			}

			auto varMap = allVars.find(varName);
			if (varMap == allVars.end()) {
				cvarManager->log("Variable " + varName + " was not found in the map.");
				std::getline(dataFile, line);
				continue;
			}
			auto& var = varMap->second;

			if (varType == "Bool") {
				if (!var.IsBool()) {
					cvarManager->log("Variable " + varName + " is not a bool.");
					std::getline(dataFile, line);
					continue;
				}

				var.SetBool(varValue == "0" ? false : true);
			}
			else if (varType == "Float") {
				if (!var.IsFloat()) {
					cvarManager->log("Variable " + varName + " is not a float.");
					std::getline(dataFile, line);
					continue;
				}

				var.SetFloat(std::stof(varValue));
			}
			else if (varType == "Int") {
				if (!var.IsInt()) {
					cvarManager->log("Variable " + varName + " is not an int.");
					std::getline(dataFile, line);
					continue;
				}

				var.SetInt(std::stoi(varValue));
			}
			else if (varType == "String") {
				if (!var.IsString()) {
					cvarManager->log("Variable " + varName + " is not a string.");
					std::getline(dataFile, line);
					continue;
				}

				var.SetString(varValue);
			}
			else if (varType == "Vector") {
				if (!var.IsVector()) {
					cvarManager->log("Variable " + varName + " is not a vector.");
					std::getline(dataFile, line);
					continue;
				}

				std::stringstream vecStream(varValue);
				std::string vecValue;
				std::vector<std::string> coordinates;
				Vector vec;
				while (std::getline(vecStream, vecValue, ',')) {
					coordinates.push_back(vecValue);
				}

				if (coordinates.size() != 3) {
					cvarManager->log("Value for variable " + varName + " does not have 3 coordiantes. Value: " + varValue);
					std::getline(dataFile, line);
					continue;
				}

				vec.X = std::stof(coordinates[0]);
				vec.Y = std::stof(coordinates[1]);
				vec.Z = std::stof(coordinates[2]);

				var.SetVector(vec);
			}

			std::getline(dataFile, line);
		}
	}
	dataFile.close();

	cvarManager->log("File " + params[0] + ".data has been loaded");
	sequence.ActivateRemoteEvents("MEPDataLoaded");
}

void MapExpansionPlugin::RemoteEventCommand(const std::vector<std::string>& params)
{
	if (params.size() != 1) {
		cvarManager->log("The remote event command expects 1 parameter and that's the name of the remote event to be activated");
		return;
	}

	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;
	cvarManager->log("Map " + gameWrapper->GetCurrentMap() + " is activating remote event " + params[0]);
	sequence.ActivateRemoteEvents(params[0]);
}

void MapExpansionPlugin::ChangeScoreCommand(const std::vector<std::string>& params)
{
	if (params.size() != 3) {
		cvarManager->log(L"The change score command expects 3 parameters. The first parameter is the team color either blue or orange. "
			"The second parameter is the operation either add or sub. The third parameter is the amount of goals to add or subtract.");
		return;
	}

	if (gameWrapper->IsInOnlineGame()) return;
	auto server = gameWrapper->GetCurrentGameState();
	if (!server) return;
	auto teams = server.GetTeams();
	if (teams.IsNull()) return;

	if (params[0] != "blue" && params[0] != "orange") {
		cvarManager->log(L"The team color must be either blue or orange.");
		return;
	}

	int teamNum = params[0] == "blue" ? 0 : 1;

	if (params[1] != "add" && params[1] != "sub") {
		cvarManager->log(L"The operation must be either add or sub.");
		return;
	}

	int mult = params[1] == "add" ? 1 : -1;

	if (!IsNumeric(params[2])) {
		cvarManager->log(L"The number of goals must be a number, no letters or symbols allowed.");
		return;
	}

	int goals = std::stoi(params[2]) * mult;

	for (int i = 0; i < teams.Count(); ++i) {
		auto team = teams.Get(i);
		if (!team.IsNull() && team.GetTeamNum2() == teamNum)
			team.ScorePoint(goals);
	}

	cvarManager->log("Changing score of " + params[0] + " by " + std::to_string(goals));
}

void MapExpansionPlugin::ChangeStatsCommand(const std::vector<std::string>& params)
{
	if (params.size() != 4) {
		cvarManager->log(L"The change stats command expects 4 parameters. The first is the stat to change. The avaiable stats to change are: "
			              "'assists goals saves score shots'. The second parameter is is the operation either add or sub. The third parameter is the amount you would "
		                  "like to increment or decrement the stat by. The fourth parameter is the playerid to change stats for.");
		return;
	}

	if (gameWrapper->IsInOnlineGame()) return;
	auto server = gameWrapper->GetCurrentGameState();
	if (!server) return;

	if (params[0] != "assists" && params[0] != "goals" && params[0] != "saves" && params[0] != "score" && params[0] != "shots") {
		cvarManager->log("The first parameter must be on of the following: 'assists goals saves score shots'. " + params[0] + " is not recognized.");
		return;
	}

	if (params[1] != "add" && params[1] != "sub") {
		cvarManager->log("The second parameter must be either add or sub.");
		return;
	}

	if (!IsNumeric(params[2])) {
		cvarManager->log("The third parameter is the amount to change the stat by, and must be a number. No letters or symbols are allowed.");
		return;
	}

	if (!IsNumeric(params[3])) {
		cvarManager->log("The fourth parameter must be a number that is the playerid. You can access the playerid through the object property chain of Player > PRI > PlayerID");
		return;
	}

	Netcode->SendNewMessage("cs " + params[0] + " " + params[1] + " " + params[2] + " " + params[3]);
}

void MapExpansionPlugin::UpdatePlayerStats(const int& playerId, const int& modifiedAmount, const std::string& statType)
{
	auto server = gameWrapper->GetCurrentGameState();
	if (!server) return;
	auto cars = server.GetCars();
	if (cars.IsNull()) return;
	for (int i = 0; i < cars.Count(); ++i) {
		auto car = cars.Get(i);
		if (!car) continue;
		auto pri = car.GetPRI();
		if (!pri) continue;
		if (pri.GetPlayerID() != playerId) continue;

		if (statType == "assists") {
			pri.SetMatchAssists(pri.GetMatchAssists() + modifiedAmount);
		}
		else if (statType == "goals") {
			pri.SetMatchGoals(pri.GetMatchGoals() + modifiedAmount);
		}
		else if (statType == "saves") {
			pri.SetMatchSaves(pri.GetMatchSaves() + modifiedAmount);
		}
		else if (statType == "score") {
			pri.SetMatchScore(pri.GetMatchScore() + modifiedAmount);
		}
		else if (statType == "shots") {
			pri.SetMatchShots(pri.GetMatchShots() + modifiedAmount);
		}

		break;
	}
}

void MapExpansionPlugin::ChangeGameState(const std::vector<std::string>& params)
{
	if (params.size() != 1) {
		cvarManager->log("The gamestate command expects 1 parameter. Right now you can only end the game with end.");
		return;
	}

	if (gameWrapper->IsInOnlineGame()) return;
	auto server = gameWrapper->GetCurrentGameState();
	if (!server) return;
	
	if (params[0] == "end") {
		server.EndGame();
		cvarManager->log("Ending match.");
	}
}

void MapExpansionPlugin::PlaySoundHandler(const std::vector<std::string>& params)
{
	if (params.size() != 2) {
		cvarManager->log(L"The playsound command expects two parameters. The first is the name of the file which is only allowed to contain alpha-numeric "
			"characters. This command can only play .wav files. Make sure the file is in the '{bakkesmodfolder}/data/expansion' folder. The second is either "
		    "a comma seperated list of player ids(no spaces) who will hear the sound, or the word 'all' to have every player hear the sound.");
		return;
	}

	if (!IsAlphaNumeric(params[0])) {
		cvarManager->log("Invalid File Name: \"" + params[0] + "\". Only alpha-numeric characters are allowed for a file name.");
		return;
	}

	if (params[1] == "all") {
		Netcode->SendNewMessage("ps " + params[0] + " all");
		cvarManager->log("Sending playsound command for file " + params[0] + " to all players.");
	}
	else {
		auto playerIds = SplitStringByChar(params[1], ',');
		for (const auto& playerId : playerIds) {
			Netcode->SendNewMessage("ps " + params[0] + " " + playerId);
			cvarManager->log("Sending playsound command for file " + params[0] + " to id : " + playerId);
		}
	}
}

void MapExpansionPlugin::PlaySoundFromFile(const std::string& wavFile)
{
	std::string fileName = wavFile + ".wav";
	auto soundPath = expansionFolder / (fileName);
	if (!std::filesystem::exists(soundPath)) {
		cvarManager->log("Unable to locate \"" + fileName + "\". Make sure the file is named correctly, is a .wav file, and inside the '{bakkesmodfolder}/data/expansion' folder.");
		return;
	}

	cvarManager->log("Attempting to play sound from " + fileName);
	bool result = PlaySound(soundPath.wstring().c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
	if (result) {
		cvarManager->log("Sound successfully played.");
	}
	else {
		cvarManager->log("Error playing sound");
	}
}

void MapExpansionPlugin::StopSoundHandler(const std::vector<std::string>& params)
{
	if (params.size() != 1) {
		cvarManager->log(L"The stopsound command expects one parameter. It's either a comma seperated list of player ids(no spaces), or the word 'all' to have the sound stop for everyone.");
		return;
	}

	if (params[0] == "all") {
		Netcode->SendNewMessage("ss all");
		cvarManager->log("Sending stopsound command to all players.");
	}
	else {
		auto playerIds = SplitStringByChar(params[0], ',');
		for (const auto& playerId : playerIds) {
			Netcode->SendNewMessage("ss " + playerId);
			cvarManager->log("Sending stopsound command to id: " + playerId);
		}
	}
}

void MapExpansionPlugin::StopSound()
{
	PlaySound(NULL, 0, 0);
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
		if (keysPressedForBinding && !binding.allKeysPressed) {
			binding.allKeysPressed = true;
			auto sequence = gameWrapper->GetMainSequence();
			if (sequence.memory_address == NULL) return;

			cvarManager->log("Activating remote event '" + binding.remoteEvent + "'");
			sequence.ActivateRemoteEvents(binding.remoteEvent);
		}
		else if (!keysPressedForBinding && binding.allKeysPressed) {
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

void MapExpansionPlugin::MapPluginVarCheck(std::string eventName)
{
	if (isInMap) return;
	if (!gameWrapper->GetGameEventAsServer()) { return; }
	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) { return; }
	auto allVars = sequence.GetAllSequenceVariables(false);
	sequence.ActivateRemoteEvents("MEPLoaded");
	cvarManager->log("Map Expansion Plugin Loaded");
	isInMap = true;
}

void MapExpansionPlugin::MapUnload(std::string eventName)
{
	inputBlocked = false;
	isInMap = false;
	mapBinds.clear();
	StopSound();
}

bool MapExpansionPlugin::IsNumeric(const std::string& str)
{
	return str.find_first_not_of("0123456789") == std::string::npos;
}

bool MapExpansionPlugin::IsAlphaNumeric(const std::string& str)
{
	return std::find_if(str.begin(), str.end(), [](char c) { return !isalnum(c); }) == str.end();
}

std::vector<std::string> MapExpansionPlugin::SplitStringByChar(const std::string& str, const char& sep)
{
	std::vector<std::string> splitString;
	std::stringstream splitStringStream(str);
	std::string stringSegment;
	while (std::getline(splitStringStream, stringSegment, sep)) {
		splitString.push_back(stringSegment);
	}

	return splitString;
}

void MapExpansionPlugin::OnMessageRecieved(const std::string& Message, PriWrapper Sender)
{
	if (!Sender || Message.empty()) return;
	auto parsedMessage = SplitStringByChar(Message, ' ');
	if (parsedMessage.size() == 0) return;

	if (parsedMessage[0] == "ps" && parsedMessage.size() == 3) {
		if (parsedMessage[2] == "all") {
			PlaySoundFromFile(parsedMessage[1]);
			return;
		}

		if (!IsMyId(parsedMessage[2])) return;
		PlaySoundFromFile(parsedMessage[1]);
	}
	else if (parsedMessage[0] == "ss" && parsedMessage.size() == 2) {
		if (parsedMessage[1] == "all") {
			StopSound();
			return;
		}

		if (!IsMyId(parsedMessage[1])) return;
		StopSound();
	}
	else if (parsedMessage[0] == "cs" && parsedMessage.size() == 5) {
		int modAmount = std::stoi(parsedMessage[3]);
		modAmount = modAmount * (parsedMessage[2] == "add" ? 1 : -1);
		UpdatePlayerStats(std::stoi(parsedMessage[4]), modAmount, parsedMessage[1]);
	}
}

bool MapExpansionPlugin::IsMyId(const std::string& idString)
{
	if (!IsNumeric(idString)) return false;
	auto curPC = gameWrapper->GetPlayerController();
	if (!curPC) return false;
	auto pri = curPC.GetPRI();
	if (!pri) return false;
	return pri.GetPlayerID() == std::stoi(idString);
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
