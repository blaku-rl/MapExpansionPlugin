#pragma once

#include "InputCommand/InputCommand.h"
#include "KeyListenCommand/KeyListenCommand.h"
#include "SaveDataCommand/SaveDataCommand.h"
#include "LoadDataCommand/LoadDataCommand.h"
#include "RemoteEventCommand/RemoteEventCommand.h"
#include "ChangeScoreCommand/ChangeScoreCommand.h"
#include "ChangeStatsCommand/ChangeStatsCommand.h"
#include "GameStateCommand/GameStateCommand.h"
#include "SaveDataPrefixCommand/SaveDataPrefixCommand.h"
#include "SRCCommand/SRCCommand.h"
#include "AnalyticsCommand/AnalyticsCommand.h"
#include "GameTimeCommand/GameTimeCommand.h"

namespace CustomCommands {
	std::unordered_map<std::string, std::unique_ptr<BaseCommand>> GetCustomCommands(BasePlugin* plugin) {
		return {
			/*{"input", std::make_unique<InputCommand>(plugin)},
			{"keylisten", std::make_unique<KeyListenCommand>(plugin)},
			{"savedata", std::make_unique<SaveDataCommand>(plugin)},
			{"loaddata", std::make_unique<LoadDataCommand>(plugin)},
			{"remoteevent", std::make_unique<RemoteEventCommand>(plugin)},
			{"changescore", std::make_unique<ChangeScoreCommand>(plugin)},
			{"changestats", std::make_unique<ChangeStatsCommand>(plugin)},
			{"gamestate", std::make_unique<GameStateCommand>(plugin)},
			{"savedataprefix", std::make_unique<SaveDataPrefixCommand>(plugin)},
			{"speedrun", std::make_unique<SRCCommand>(plugin)},
			{"analytics", std::make_unique<AnalyticsCommand>(plugin)},
			{"gametime", std::make_unique<GameTimeCommand>(plugin)},*/
		};
	}
}