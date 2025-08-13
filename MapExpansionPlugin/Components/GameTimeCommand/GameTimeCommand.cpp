#include "pch.h"
#include "GameTimeCommand.h"
#include "../../Utility/Utils.h"

GameTimeCommand::GameTimeCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
}

void GameTimeCommand::CommandFunction(const std::vector<std::string>& params)
{
	if (params.size() != 2) {
		LOG(L"The gametime command expects 2 parameters. The first is one of the following 'add sub set' to either add time, subtract time, or set the time respectively. "
			"The second parameter is the time in seconds to change the game by.");
		return;
	}

	if (!Utils::IsNumeric(params[1])) {
		LOG("The second parameter must be a number representing the amount of seconds to change the time by.");
		return;
	}

	int timeChange = std::stoi(params[1]);

	if (plugin->gameWrapper->IsInOnlineGame()) return;
	auto server = plugin->gameWrapper->GetCurrentGameState();
	if (!server) return;

	if (params[0] == "add") {
		server.SetGameTimeRemaining(server.GetGameTimeRemaining() + timeChange);
	}
	else if (params[0] == "sub") {
		server.SetGameTimeRemaining(server.GetGameTimeRemaining() - timeChange);
	}
	else if (params[0] == "set") {
		server.SetGameTimeRemaining(timeChange);
	}
	else {
		LOG("{} is not recoginzed option. Only use add, sub, or set.");
	}
}

void GameTimeCommand::NetcodeHandler(const std::vector<std::string>& params)
{

}

std::string GameTimeCommand::GetNetcodeIdentifier()
{
	return "GT";
}
