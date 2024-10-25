#include "pch.h"
#include "SaveDataCommand.h"
#include "../../Utility/Utils.h"
#include "../../Utility/Constants.h"
#include "../../Utility/KismetIncludes.h"
#include <fstream>

SaveDataCommand::SaveDataCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
}

void SaveDataCommand::CommandFunction(const std::vector<std::string>& params)
{
	if (params.size() != 2) {
		LOG(
			L"savedata command expects 2 parameters. The first is a comma separated list(without spaces) of kismet variables to save. "
			"Currently only bools, floats, ints, strings, and vectors are supported for saving. The variable must be defined on the main sequence to be found. "
			"The second is the name of the file that you would like to save too. The file name parameter only accepts alpha-numeric characters for a valid file name.");
		return;
	}

	const std::string& fileName = params[1];
	if (!Utils::IsAlphaNumeric(fileName)) {
		LOG("Invalid File Name: {}. Only alpha-numeric characters are allowed for a file name.", fileName);
		return;
	}

	auto kismetVars = Utils::SplitStringByChar(params[0], ',');
	auto& mapVars = plugin->GetMapVariables();

	std::ofstream dataFile(plugin->GetExpansionFolder() / (fileName + ".data"));
	if (dataFile.is_open()) {
		for (auto& curVar : kismetVars) {
			auto mapSeqVar = mapVars.find(curVar);
			if (mapSeqVar == mapVars.end()) {
				LOG("Kismet Variable {} was not found in the main sequence. Variable will not be saved", curVar);
				continue;
			}
			dataFile << "{\n";
			dataFile << "    " << CONSTANTS::nameTag << curVar << "\n";
			auto& mapVar = mapVars.find(curVar)->second;
			if (mapVar.IsBool()) {
				dataFile << "    " << CONSTANTS::typeTag << "Bool\n";
				dataFile << "    " << CONSTANTS::valueTag << (mapVar.GetBool() ? "1" : "0") << "\n";
			}
			else if (mapVar.IsFloat()) {
				dataFile << "    " << CONSTANTS::typeTag << "Float\n";
				dataFile << "    " << CONSTANTS::valueTag << mapVar.GetFloat() << "\n";
			}
			else if (mapVar.IsInt()) {
				dataFile << "    " << CONSTANTS::typeTag << "Int\n";
				dataFile << "    " << CONSTANTS::valueTag << mapVar.GetInt() << "\n";
			}
			else if (mapVar.IsString()) {
				dataFile << "    " << CONSTANTS::typeTag << "String\n";
				dataFile << "    " << CONSTANTS::valueTag << mapVar.GetString() << "\n";
			}
			else if (mapVar.IsVector()) {
				dataFile << "    " << CONSTANTS::typeTag << "Vector\n";
				Vector vec = mapVar.GetVector();
				dataFile << "    " << CONSTANTS::valueTag << vec.X << "," << vec.Y << "," << vec.Z << "\n";
			}
			dataFile << "}\n";
		}
	}
	dataFile.close();

	LOG("File " + params[1] + ".data has been saved");
	plugin->ActivateRemoteEvent("MEPDataSaved");
}

void SaveDataCommand::NetcodeHandler(const std::vector<std::string>& params)
{
}

std::string SaveDataCommand::GetNetcodeIdentifier()
{
	return "SD";
}
