#include "pch.h"
#include "RemoteEventCommand.h"
#include "../../Utility/KismetIncludes.h"

RemoteEventCommand::RemoteEventCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
}

void RemoteEventCommand::CommandFunction(const std::vector<std::string>& params)
{
	if (params.size() != 1) {
		LOG("The remote event command expects 1 parameter and that's the name of the remote event to be activated");
		return;
	}

	auto sequence = plugin->gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;
	LOG("Map " + plugin->gameWrapper->GetCurrentMap() + " is activating remote event " + params[0]);
	sequence.ActivateRemoteEvents(params[0]);
}

void RemoteEventCommand::NetcodeHandler(const std::vector<std::string>& params)
{
}

std::string RemoteEventCommand::GetNetcodeIdentifier()
{
	return "RE";
}
