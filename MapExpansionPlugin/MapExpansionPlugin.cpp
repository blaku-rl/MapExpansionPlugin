#include "pch.h"
#include "MapExpansionPlugin.h"
#include <bakkesmod/wrappers/kismet/SequenceWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceVariableWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceOpWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceObjectWrapper.h>
#include <fstream>
#include <sstream>

BAKKESMOD_PLUGIN(MapExpansionPlugin, "Plugin for expanding map creators capabilities", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void MapExpansionPlugin::onLoad()
{
	using namespace std::placeholders;
	_globalCvarManager = cvarManager;

	custCommands.insert({ "input", std::bind(&MapExpansionPlugin::InputBlockCommand, this, _1) });
	custCommands.insert({ "keylisten", std::bind(&MapExpansionPlugin::KeyListenCommand, this, _1) });
	custCommands.insert({ "savedata", std::bind(&MapExpansionPlugin::SaveDataCommand, this, _1) });
	custCommands.insert({ "loaddata", std::bind(&MapExpansionPlugin::LoadDataCommand, this, _1) });

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
	//No crashes on updates...maybe
	if (setupThread.joinable()) {
		setupThread.join();
	}
}

void MapExpansionPlugin::SetUpKeysMap()
{
	for (auto& keyName : keyNames) {
		int fnameIndex = gameWrapper->GetFNameIndexByString(keyName);
		keysPressed.insert(std::pair<int, bool>(fnameIndex, false));
		keyNameToIndex.insert(std::pair<std::string, int>(keyName, fnameIndex));
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
	auto custCommand = allVars.find("mepcommand");
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
			L"keylisten command expects 2 parameters separated by spaces. First is a comma separated list(without spaces) of keys that are required."
			"The second is the name of the remote event that will be called once the required keys are pressed");
		return;
	}

	MapBind curBind;
	std::stringstream keysStream(params[0]);
	std::string key;
	while (std::getline(keysStream, key, ',')) {
		if (keyNameToIndex.find(key) == keyNameToIndex.end()) {
			cvarManager->log("Key '" + key + "' is not a supported key. View documentation for supported key names. keylisten command will not be added");
			return;
		}

		curBind.keyListFnameIndex.push_back(keyNameToIndex[key]);
	}
	curBind.remoteEvent = params[1];
	curBind.allKeysPressed = false;

	mapBinds.push_back(std::make_shared<MapBind>(curBind));
	cvarManager->log("Making bind with keys '" + params[0] + "' and remote event '" + params[1] + "'");
}

void MapExpansionPlugin::SaveDataCommand(std::vector<std::string> params)
{
	if (params.size() != 2) {
		cvarManager->log(
			L"savedata command expects 2 parameters. The first is a comma separated list(without spaces) of kismet variables to save. "
			"Currently only bools, floats, ints, strings, and vectors are supported for saving. The variable must be defined on the main sequence to be found. "
			"The second is the name of the file that you would like to save too.(no file extenstion) Make this unique to your map so as not to conflict with other maps.");
		return;
	}

	std::vector<std::string> kismetVars;
	std::stringstream kismetVarsStream(params[0]);
	std::string kismetVar;
	while (std::getline(kismetVarsStream, kismetVar, ',')) {
		kismetVars.push_back(kismetVar);
	}

	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;
	auto allVars = sequence.GetAllSequenceVariables(false);

	std::ofstream dataFile(expansionFolder / (params[1] + ".data"));
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

void MapExpansionPlugin::LoadDataCommand(std::vector<std::string> params)
{
	if (params.size() != 1) {
		cvarManager->log("loaddata command expects 1 parameter. It should be the name of the file that you have saved to without a fileextension.");
		return;
	}

	auto saveDataPath = expansionFolder / (params[0] + ".data");
	if (!std::filesystem::exists(saveDataPath)) {
		cvarManager->log("File " + params[0] + ".data doesn't exist. Ignore if this is first time loading the map");
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
	sequence.ActivateRemoteEvents("MEPLoaded");
}

void MapExpansionPlugin::MapUnload(std::string eventName)
{
	inputBlocked = false;
	mapBinds.clear();
}

void MapExpansionPlugin::OnMessageRecieved(const std::string& Message, PriWrapper Sender)
{
}
