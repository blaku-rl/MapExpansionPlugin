#include "pch.h"
#include "InputCommand.h"

InputCommand::InputCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
}

void InputCommand::CommandFunction(const std::vector<std::string>& params)
{
	if (params.size() != 1) {
		LOG("Input command expects 1 parameter. Either block or allow.");
		return;
	}

	if (params[0] == "block") {
		LOG("Map {} is blocking input.", plugin->gameWrapper->GetCurrentMap());
		plugin->BlockInput(true);
	}
	else if (params[0] == "allow") {
		LOG("Map {} is allowing input.", plugin->gameWrapper->GetCurrentMap());
		plugin->BlockInput(false);
	}
	else {
		LOG("Param '{}'  is not a valid paramater for input command. Use block or allow.", params[0]);
	}
}

void InputCommand::NetcodeHandler(const std::vector<std::string>& params)
{
}

std::string InputCommand::GetNetcodeIdentifier()
{
	return "IP";
}
