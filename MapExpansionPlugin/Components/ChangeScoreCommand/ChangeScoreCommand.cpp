#include "pch.h"
#include "ChangeScoreCommand.h"
#include "../../Utility/Utils.h"

ChangeScoreCommand::ChangeScoreCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
}

void ChangeScoreCommand::CommandFunction(const std::vector<std::string>& params)
{
	if (params.size() != 3) {
		LOG(L"The change score command expects 3 parameters. The first parameter is the team color either blue or orange. "
			"The second parameter is the operation either add or sub. The third parameter is the amount of goals to add or subtract.");
		return;
	}

	if (plugin->gameWrapper->IsInOnlineGame()) return;
	auto server = plugin->gameWrapper->GetCurrentGameState();
	if (!server) return;
	auto teams = server.GetTeams();
	if (teams.IsNull()) return;

	if (params[0] != "blue" && params[0] != "orange") {
		LOG(L"The team color must be either blue or orange.");
		return;
	}

	int teamNum = params[0] == "blue" ? 0 : 1;

	if (params[1] != "add" && params[1] != "sub") {
		LOG(L"The operation must be either add or sub.");
		return;
	}

	int mult = params[1] == "add" ? 1 : -1;

	if (!Utils::IsNumeric(params[2])) {
		LOG(L"The number of goals must be a number, no letters or symbols allowed.");
		return;
	}

	int goals = std::stoi(params[2]) * mult;

	for (int i = 0; i < teams.Count(); ++i) {
		auto team = teams.Get(i);
		if (!team.IsNull() && team.GetTeamNum2() == teamNum)
			team.ScorePoint(goals);
	}

	LOG("Changing score of " + params[0] + " by " + std::to_string(goals));
}

void ChangeScoreCommand::NetcodeHandler(const std::vector<std::string>& params)
{
}

std::string ChangeScoreCommand::GetNetcodeIdentifier()
{
	return "SC";
}
