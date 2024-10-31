#include "pch.h"
#include "SRCCommand.h"
#include "../../Utility/Utils.h"
#include <nlohmann/json.hpp>
#include <ranges>
#include <cmath>

SRCCommand::SRCCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
}

void SRCCommand::CommandFunction(const std::vector<std::string>& params)
{
	if (params.size() == 0) {
		LOG(
			L"The speedrun command can be run with a few different options. The get query option will pass along any query to the speedrun api and return the result."
			"The getinfo option will return the run data for a specific category and place on a map. The findmap option will return the map id and cateogry/values ids for a map"
			"Calling each of these options will give you further instructions on proper usage.");
		return;
	}

	if (params[0] == "get")
		SendRawSRCQuery(params);
	else if (params[0] == "getinfo")
		SendFormattedRequest(params);
	else if (params[0] == "getdate" or params[0] == "gettime" or params[0] == "getplayers")
		FindSpecificInfo(params, params[0].substr(3, std::string::npos));
	else
		plugin->SendInfoToMap(params[0] + " is not a valid option. Use get, getinfo, getdate, gettime, or getplayers");
}

void SRCCommand::NetcodeHandler(const std::vector<std::string>& params)
{
}

std::string SRCCommand::GetNetcodeIdentifier()
{
	return "SR";
}

void SRCCommand::SendSRCRequestWithRetries(const CurlRequest& req, std::function<std::string(std::string data)> successFunc, const int& retries)
{
	if (retries <= 0) {
		plugin->SendInfoToMap("Max Retries Reached For Request");
		return;
	}

	LOG("Sending request {}", req.url);
	HttpWrapper::SendCurlJsonRequest(req, [this, req, retries, successFunc](int code, std::string data) {
		plugin->gameWrapper->Execute([this, code, data, req, retries, successFunc](GameWrapper* gw) {
			LOG("Request for {} has returned", req.url);
			LOG("Response code from src: {}", std::to_string(code));

			if (code != 200) {
				plugin->gameWrapper->SetTimeout([this, req, retries, successFunc](GameWrapper* gw) {
					SendSRCRequestWithRetries(req, successFunc, retries - 1);
					}, 1.0f);
				return;
			}

			plugin->SendInfoToMap(successFunc(data));
			});
		});
}

void SRCCommand::SendRawSRCQuery(const std::vector<std::string>& params)
{
	if (params.size() != 2) {
		LOG(
			L"The get option expects only an attached query to be passed along with the speedrun command. This will build a complete request to src and pass along the response to the map"
			"An example of this would be to do 'speedrun get series/g7q25m57/games' this would send the request https://www.speedrun.com/api/v1/series/g7q25m57/games "
			"Full documentation of the src api can be found here: https://github.com/speedruncomorg/api");
		return;
	}

	CurlRequest req;
	req.url = srcBaseUrl + params[1];
	req.verb = "GET";

	SendSRCRequestWithRetries(req, [params](std::string data) {
		if (data.empty())
			return data;
		data.insert(1, "\"source\":\"" + params[1] + "\",");
		return data; 
		});
}

void SRCCommand::SendFormattedRequest(const std::vector<std::string>& params)
{
	if (params.size() != 2) {
		LOG(
			L"The getinfo options expects a & seperated list of key value pairs that will be used to build a query to src similar to a url query. "
			"The required key values that are needed are 'game=gameid&category=categoryid' where gameid and categoryid are actual ids on src. "
			"You can also optionally provide src variables to get info on subcategories. These take the form of 'varid=varvalueid'. "
			"Levels can also be targeted by attaching 'level=levelid' to the parameter. By default only the wr will be returned or if there are no runs, a message that says no runs found "
			"You can get information about a specific run placement by attaching 'place=number'. "
			"A full example looks like 'speedrun getinfo game=4d7ne4r6&category=q2513pwd&dloroydl=21g0056q&jlzgpwqn=klrkk9o1' gets the wr run for yoshis island for the 100% category "
			"for 1 lap and unlimited boost. Insert what the return looks like.");
		return;
	}

	const auto& [req, place] = BuildRequestFromInput(params);

	SendSRCRequestWithRetries(req, [this, place](std::string data) {
		return ParseFormattedRequest(data, place);
		});
}

void SRCCommand::FindSpecificInfo(const std::vector<std::string>& params, const std::string& param)
{
	if (params.size() != 2) {
		LOG("The get{} command expects the same input as the getinfo command. See that command for instructions on how to use this one.", param);
		return;
	}

	const auto& [req, place] = BuildRequestFromInput(params);

	SendSRCRequestWithRetries(req, [this, place, param](std::string data) {
		auto parsedStr = ParseFormattedRequest(data, place);
		auto values = ParseKeyValueString(parsedStr);
		if (values.contains(param))
			return values[param];
		return param + " not found in the string " + parsedStr;
		});
}

std::pair<CurlRequest, int> SRCCommand::BuildRequestFromInput(const std::vector<std::string>& params)
{
	CurlRequest req;
	req.verb = "GET";

	auto reqParams = ParseKeyValueString(params[1]);

	auto gameId = FindAndDeleteKey("game", reqParams);
	if (gameId.empty()) {
		LOG("No game id found in {}", params[1]);
		return std::make_pair(req, 0);
	}

	auto categoryId = FindAndDeleteKey("category", reqParams);
	if (categoryId.empty()) {
		LOG("No category id found in {}", params[1]);
		return std::make_pair(req, 0);
	}

	auto levelId = FindAndDeleteKey("level", reqParams);

	int place = 1;
	auto placeStr = FindAndDeleteKey("place", reqParams);
	if (!placeStr.empty()) {
		if (!Utils::IsNumeric(placeStr)) {
			LOG("The place value {} must be a number", placeStr);
			return std::make_pair(req, 0);
		}
		place = std::stoi(placeStr);
	}

	std::string varStr = "?embed=players&";
	for (const auto& [key, value] : reqParams)
		varStr += "var-" + key + "=" + value + "&";
	varStr = varStr.substr(0, varStr.size() - 1);

	if (!levelId.empty())
		req.url = srcBaseUrl + "leaderboards/" + gameId + "/level/" + levelId + "/" + categoryId + varStr;
	else
		req.url = srcBaseUrl + "leaderboards/" + gameId + "/category/" + categoryId + varStr;

	return std::make_pair(req, place);
}

std::unordered_map<std::string, std::string> SRCCommand::ParseKeyValueString(const std::string& keyValStr)
{
	std::unordered_map<std::string, std::string> keyValue = {};
	for (const auto& pair : Utils::SplitStringByChar(keyValStr, '&')) {
		const auto split = Utils::SplitStringByChar(pair, '=');
		keyValue[split[0]] = split[1];
	}
	return keyValue;
}

std::string SRCCommand::FindAndDeleteKey(const std::string& key, std::unordered_map<std::string, std::string>& params)
{
	if (!params.contains(key)) {
		return "";
	}
	std::string value = params[key];
	auto it = params.find(key);
	params.erase(it);
	return value;
}

std::string SRCCommand::ParseFormattedRequest(const std::string& data, const int& place)
{
	nlohmann::json js = nlohmann::json::parse(data);

	if (!js.contains("data") or !js["data"].is_object()) {
		LOG("No data item found");
		return "Invalid Response";
	}
	auto& fullData = js["data"];

	if (!fullData.contains("runs") or !fullData["runs"].is_array())
		return "No Runs Found";
	auto& runs = fullData["runs"];

	if (runs.size() < place) {
		LOG("Only {} runs found. {} is too big.", runs.size(), place);
		return "Invalid Place Argument";
	}
	auto& runObj = runs.at(place);

	if (!runObj.is_object() or !runObj.contains("run") or !runObj["run"].is_object())
		return "No Run Found";
	auto& run = runObj["run"];

	if (!run.contains("date") or !run["date"].is_string())
		return "No Date Found";
	auto& date = run["date"];

	if (!run.contains("times") or !run["times"].is_object())
		return "No Time Found";
	auto& timeObj = run["times"];

	if (!timeObj.contains("primary_t") or !timeObj["primary_t"].is_number_float())
		return "No Time Found";
	auto time = timeObj["primary_t"].template get<float>();

	if (!run.contains("players") or !run["players"].is_array())
		return "No Players Found";
	auto& playersArr = run["players"];

	std::vector<nlohmann::json> ids = {};
	for (auto& player : playersArr) {
		if (!player.is_object() or !player.contains("id") or !player["id"].is_string())
			return "No Player Id Found";

		ids.push_back(player["id"]);
	}

	if (ids.size() == 0)
		return "No Players Found";

	if (!fullData.contains("players") or !fullData["players"].is_object() or !fullData["players"].contains("data") or !fullData["players"]["data"].is_array())
		return "No Players Found";
	auto& allPlayersArr = fullData["players"]["data"];

	std::vector<std::string> players = {};
	for (auto& player : allPlayersArr) {
		if (!player.contains("id") or !player["id"].is_string())
			continue;
		if (!player.contains("names") or !player["names"].is_object() or !player["names"].contains("international") or !player["names"]["international"].is_string())
			continue;
		if (std::ranges::contains(ids, player["id"]))
			players.push_back(player["names"]["international"].template get<std::string>());
	}

	if (ids.size() != players.size())
		return "Not All Player Ids Found";

	std::string playersStr = std::ranges::fold_left(players, "&players=", [](std::string acc, std::string player) {return acc + player + ","; });
	playersStr = playersStr.substr(0, playersStr.size() - 1);

	return "date=" + date.template get<std::string>() + "&time=" + GetTimeStringFromFloat(time) + playersStr;
}

std::string SRCCommand::GetTimeStringFromFloat(const float& time)
{
	const int HOUR_MULTIPLIER = 3600;
	const int MINUTE_MULTIPLIER = 60;

	int totalSeconds = std::floor(time);
	int milli = 1000 * (time - totalSeconds);

	std::string timeString = "";

	int hours = totalSeconds / HOUR_MULTIPLIER;
	if (hours > 0) 
		timeString += std::to_string(hours) + ":";
	int remainingTime = totalSeconds - (hours * HOUR_MULTIPLIER);

	int minutes = remainingTime / MINUTE_MULTIPLIER;
	if (minutes > 0)
		timeString += EnsureTwoWideNumber(minutes) + ":";
	else if (!timeString.empty())
		timeString += "00:";

	int seconds = remainingTime - (minutes * MINUTE_MULTIPLIER);
	timeString += EnsureTwoWideNumber(seconds) + ".";

	if (milli < 10)
		timeString += "00";
	else if (milli < 100)
		timeString += "0";
	timeString += std::to_string(milli);

	// hh:mm:ss.mmm
	return timeString;
}

std::string SRCCommand::EnsureTwoWideNumber(const int& num)
{
	return (num < 10 ? "0" : "") + std::to_string(num);
}
