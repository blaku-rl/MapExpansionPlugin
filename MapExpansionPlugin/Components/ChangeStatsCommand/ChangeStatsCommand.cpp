#include "pch.h"
#include "ChangeStatsCommand.h"
#include "../../Utility/Utils.h"
#include <ranges>

ChangeStatsCommand::ChangeStatsCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
}

void ChangeStatsCommand::CommandFunction(const std::vector<std::string>& params)
{
	if (params.size() != 4) {
		LOG(L"The change stats command expects 4 parameters. The first is the stat to change. The avaiable stats to change are: "
			"'assists goals saves score shots'. The second parameter is is the operation either add or sub. The third parameter is the amount you would "
			"like to increment or decrement the stat by. The fourth parameter is the playerid to change stats for.");
		return;
	}

	if (plugin->gameWrapper->IsInOnlineGame()) return;
	auto server = plugin->gameWrapper->GetCurrentGameState();
	if (!server) return;

	if (!validStats.contains(params[0])) {
		auto statsStr = std::ranges::fold_left(validStats, "", [](std::string acc, std::string val) {return acc + " " + val; });
		LOG("The first parameter must be one of the following: '{}'. {} is not recognized.", statsStr, params[0]);
		return;
	}

	if (!validOpps.contains(params[1])) {
		auto oppsStr = std::ranges::fold_left(validOpps, "", [](std::string acc, std::string val) {return acc + " " + val; });
		LOG("The second parameter must be one of the following: '{}'. {} is not recognized.", oppsStr, params[1]);
		return;
	}

	if (!Utils::IsNumeric(params[2])) {
		LOG("The third parameter is the amount to change the stat by, and must be a number. No letters or symbols are allowed.");
		return;
	}

	if (!Utils::IsNumeric(params[3])) {
		LOG("The fourth parameter must be a number that is the playerid. You can access the playerid through the object property chain of Player > PRI > PlayerID");
		return;
	}

	plugin->NotifyPlayers("CS " + params[0] + " " + params[1] + " " + params[2] + " " + params[3]);
}

void ChangeStatsCommand::NetcodeHandler(const std::vector<std::string>& params)
{
	if (params.size() != 4) return;
	int modAmount = std::stoi(params[2]);
	modAmount = modAmount * (params[1] == "add" ? 1 : -1);
	int playerId = std::stoi(params[1]);
	std::string statType = params[0];

	auto server = plugin->gameWrapper->GetCurrentGameState();
	if (!server) return;
	auto cars = server.GetCars();
	if (cars.IsNull()) return;
	for (int i = 0; i < cars.Count(); ++i) {
		auto car = cars.Get(i);
		if (!car) continue;
		auto pri = car.GetPRI();
		if (!pri) continue;
		if (pri.GetPlayerID() != playerId) continue;

		if (statType == "assists") {
			pri.SetMatchAssists(pri.GetMatchAssists() + modAmount);
		}
		else if (statType == "goals") {
			pri.SetMatchGoals(pri.GetMatchGoals() + modAmount);
		}
		else if (statType == "saves") {
			pri.SetMatchSaves(pri.GetMatchSaves() + modAmount);
		}
		else if (statType == "score") {
			pri.SetMatchScore(pri.GetMatchScore() + modAmount);
		}
		else if (statType == "shots") {
			pri.SetMatchShots(pri.GetMatchShots() + modAmount);
		}

		break;
	}
}

std::string ChangeStatsCommand::GetNetcodeIdentifier()
{
	return "CS";
}
