#include "pch.h"
#include "SaveDataPrefixCommand.h"
#include "../../Utility/Utils.h"
#include "../../Utility/Constants.h"
#include "../../Utility/KismetIncludes.h"
#include <fstream>
#include <ranges>

SaveDataPrefixCommand::SaveDataPrefixCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
}

void SaveDataPrefixCommand::CommandFunction(const std::vector<std::string>& params)
{
	if (params.size() != 2) {
		LOG(
			L"savedataprefix command expects 2 parameters. The first is a comma separated list(without spaces) of prefixes that your variables have. "
			"Currently only bools, floats, ints, strings, and vectors are supported for saving. The variable must be defined on the main sequence to be found. "
			"The second is the name of the file that you would like to save too. The file name parameter only accepts alpha-numeric characters for a valid file name.");
		return;
	}

	const std::string& fileName = params[1];
	if (!Utils::IsAlphaNumeric(fileName)) {
		LOG("Invalid File Name: {}. Only alpha-numeric characters are allowed for a file name.", fileName);
		return;
	}

	auto kismetVarPrefixes = Utils::SplitStringByChar(params[0], ',');

	std::ofstream dataFile(plugin->GetExpansionFolder() / (fileName + ".data"));
	if (dataFile.is_open()) {
		for (auto& [varName, varObj] : plugin->GetMapVariables()) {
			auto containsPrefix = std::ranges::fold_left(kismetVarPrefixes, false, [varName](bool value, std::string prefix) 
				{
					return value or varName.starts_with(prefix); 
				});
			if (!containsPrefix) {
				continue;
			}
			if (varObj.IsNull() or varObj.IsActor() or varObj.IsObjectList()) {
				LOG("{} is either null, an object, or object list. Not saved to file.", varName);
				continue;
			}
			dataFile << "{\n";
			dataFile << "    " << Constants::SaveData::nameTag << varName << "\n";
			if (varObj.IsBool()) {
				dataFile << "    " << Constants::SaveData::typeTag << "Bool\n";
				dataFile << "    " << Constants::SaveData::valueTag << (varObj.GetBool() ? "1" : "0") << "\n";
			}
			else if (varObj.IsFloat()) {
				dataFile << "    " << Constants::SaveData::typeTag << "Float\n";
				dataFile << "    " << Constants::SaveData::valueTag << varObj.GetFloat() << "\n";
			}
			else if (varObj.IsInt()) {
				dataFile << "    " << Constants::SaveData::typeTag << "Int\n";
				dataFile << "    " << Constants::SaveData::valueTag << varObj.GetInt() << "\n";
			}
			else if (varObj.IsString()) {
				dataFile << "    " << Constants::SaveData::typeTag << "String\n";
				dataFile << "    " << Constants::SaveData::valueTag << varObj.GetString() << "\n";
			}
			else if (varObj.IsVector()) {
				dataFile << "    " << Constants::SaveData::typeTag << "Vector\n";
				Vector vec = varObj.GetVector();
				dataFile << "    " << Constants::SaveData::valueTag << vec.X << "," << vec.Y << "," << vec.Z << "\n";
			}
			dataFile << "}\n";
		}
	}
	dataFile.close();

	LOG("File {}.data has been saved", params[1]);
	plugin->ActivateRemoteEvent("MEPDataSaved");
}

void SaveDataPrefixCommand::NetcodeHandler(const std::vector<std::string>& params)
{
}

std::string SaveDataPrefixCommand::GetNetcodeIdentifier()
{
	return "SP";
}
