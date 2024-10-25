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

	auto sequence = plugin->gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;
	auto allVars = sequence.GetAllSequenceVariables(false);

	std::ofstream dataFile(plugin->GetExpansionFolder() / (fileName + ".data"));
	if (dataFile.is_open()) {
		for (auto& [varName, varObj] : allVars) {
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
			dataFile << "    " << CONSTANTS::nameTag << varName << "\n";
			if (varObj.IsBool()) {
				dataFile << "    " << CONSTANTS::typeTag << "Bool\n";
				dataFile << "    " << CONSTANTS::valueTag << (varObj.GetBool() ? "1" : "0") << "\n";
			}
			else if (varObj.IsFloat()) {
				dataFile << "    " << CONSTANTS::typeTag << "Float\n";
				dataFile << "    " << CONSTANTS::valueTag << varObj.GetFloat() << "\n";
			}
			else if (varObj.IsInt()) {
				dataFile << "    " << CONSTANTS::typeTag << "Int\n";
				dataFile << "    " << CONSTANTS::valueTag << varObj.GetInt() << "\n";
			}
			else if (varObj.IsString()) {
				dataFile << "    " << CONSTANTS::typeTag << "String\n";
				dataFile << "    " << CONSTANTS::valueTag << varObj.GetString() << "\n";
			}
			else if (varObj.IsVector()) {
				dataFile << "    " << CONSTANTS::typeTag << "Vector\n";
				Vector vec = varObj.GetVector();
				dataFile << "    " << CONSTANTS::valueTag << vec.X << "," << vec.Y << "," << vec.Z << "\n";
			}
			dataFile << "}\n";
		}
	}
	dataFile.close();

	LOG("File {}.data has been saved", params[1]);
	sequence.ActivateRemoteEvents("MEPDataSaved");
}

void SaveDataPrefixCommand::NetcodeHandler(const std::vector<std::string>& params)
{
}

std::string SaveDataPrefixCommand::GetNetcodeIdentifier()
{
	return "SP";
}
