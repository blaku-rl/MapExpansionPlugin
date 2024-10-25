#include "pch.h"
#include "GameStateCommand.h"
#include "../../Utility/Utils.h"

GameStateCommand::GameStateCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
}

void GameStateCommand::CommandFunction(const std::vector<std::string>& params)
{
	if (params.size() != 1) {
		LOG(L"The gamestate command expects 1 parameter. Use 'end' to end the match and go to the podium screen. "
			"Use 'kickoff' to reset from kickoff");
		return;
	}

	if (plugin->gameWrapper->IsInOnlineGame()) return;
	auto server = plugin->gameWrapper->GetCurrentGameState();
	if (!server) return;

	if (params[0] == "end") {
		server.EndGame();
		LOG("Ending match.");
	}
	else if (params[0] == "kickoff") {
		server.StartNewRound();
		LOG("Reseting from kickoff.");
	}
}

void GameStateCommand::NetcodeHandler(const std::vector<std::string>& params)
{
	
}

std::string GameStateCommand::GetNetcodeIdentifier()
{
	return "GS";
}
