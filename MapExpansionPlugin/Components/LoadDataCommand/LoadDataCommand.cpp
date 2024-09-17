#include "pch.h"
#include "LoadDataCommand.h"
#include "../../Utility/Utils.h"
#include "../../Utility/Constants.h"
#include "../../Utility/KismetIncludes.h"
#include <fstream>
#include <sstream>

LoadDataCommand::LoadDataCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
}

void LoadDataCommand::CommandFunction(const std::vector<std::string>& params)
{
	if (params.size() != 1) {
		LOG(L"loaddata command expects 1 parameter. It should be the name of the file that you have saved data to. "
			"The file name parameter only accepts alpha-numeric characters for a valid file name.");
		return;
	}

	const std::string& fileName = params[0];
	if (!Utils::IsAlphaNumeric(fileName)) {
		LOG("Invalid File Name: \"" + fileName + "\". Only alpha-numeric characters are allowed for a file name.");
		return;
	}

	auto saveDataPath = plugin->GetExpansionFolder() / (fileName + ".data");
	if (!std::filesystem::exists(saveDataPath)) {
		LOG("File " + fileName + ".data doesn't exist. Ignore if this is first time loading the map");
	}

	auto sequence = plugin->gameWrapper->GetMainSequence();
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
			if (line.find(CONSTANTS::nameTag) != std::string::npos) {
				varName = line.substr(line.find(CONSTANTS::nameTag) + CONSTANTS::nameTag.size(), std::string::npos);
			}

			std::getline(dataFile, line);
			if (line.find(CONSTANTS::typeTag) != std::string::npos) {
				varType = line.substr(line.find(CONSTANTS::typeTag) + CONSTANTS::typeTag.size(), std::string::npos);
			}

			std::getline(dataFile, line);
			if (line.find(CONSTANTS::valueTag) != std::string::npos) {
				varValue = line.substr(line.find(CONSTANTS::valueTag) + CONSTANTS::valueTag.size(), std::string::npos);
			}

			auto varMap = allVars.find(varName);
			if (varMap == allVars.end()) {
				LOG("Variable " + varName + " was not found in the map.");
				std::getline(dataFile, line);
				continue;
			}
			auto& var = varMap->second;

			if (varType == "Bool") {
				if (!var.IsBool()) {
					LOG("Variable " + varName + " is not a bool.");
					std::getline(dataFile, line);
					continue;
				}

				var.SetBool(varValue == "0" ? false : true);
			}
			else if (varType == "Float") {
				if (!var.IsFloat()) {
					LOG("Variable " + varName + " is not a float.");
					std::getline(dataFile, line);
					continue;
				}

				var.SetFloat(std::stof(varValue));
			}
			else if (varType == "Int") {
				if (!var.IsInt()) {
					LOG("Variable " + varName + " is not an int.");
					std::getline(dataFile, line);
					continue;
				}

				var.SetInt(std::stoi(varValue));
			}
			else if (varType == "String") {
				if (!var.IsString()) {
					LOG("Variable " + varName + " is not a string.");
					std::getline(dataFile, line);
					continue;
				}

				var.SetString(varValue);
			}
			else if (varType == "Vector") {
				if (!var.IsVector()) {
					LOG("Variable " + varName + " is not a vector.");
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
					LOG("Value for variable " + varName + " does not have 3 coordiantes. Value: " + varValue);
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

	LOG("File " + params[0] + ".data has been loaded");
	sequence.ActivateRemoteEvents("MEPDataLoaded");
}

void LoadDataCommand::NetcodeHandler(const std::vector<std::string>& params)
{
}

std::string LoadDataCommand::GetNetcodeIdentifier()
{
	return "LD";
}
