#include "pch.h"
#include "AnalyticsCommand.h"
#include "../../Utility/Utils.h"
#include <stdexcept>
#include <chrono>

AnalyticsCommand::AnalyticsCommand(BasePlugin* plugin) : 
	BaseCommand(plugin),
	debounce(plugin, [this]() {OnEventDebounceEnd(); }),
	reqManager(plugin)
{
	eventList = {};
}

void AnalyticsCommand::CommandFunction(const std::vector<std::string>& params)
{
	if (!plugin->GetPluginSettings().analyticsAllowed) {
		LOG("Analytics have been disabled by the user. The analytics command will not be executed.");
		plugin->SendInfoToMap("Analytics Refused");
		return;
	}

	auto incorrectParamsMessage = L"The analytics command expects a valid option to interact with the analytics api. The options are either trackinit, trackevent, or trackend "
		"The trackinit command must first be called to start a session with a provided API key for your map. Only one session may be open at a time. "
		"Details on setting that up can be found here https://analytics.rocketleaguemapmaking.com/ The trackevent option will let you pass json data "
		"to the analytics api for tracking. The trackend command will stop the tracking for a session that has been created. The plugin will automatically "
		"close any existing sessions when a map is unloaded.";

	if (params.size() == 0) {
		LOG(incorrectParamsMessage);
		return;
	}

	if (params[0] == "trackinit") return TrackInitCommand(params);
	if (params[0] == "trackevent") return TrackEventCommand(params);
	if (params[0] == "trackend") return TrackEndCommand(params);

	LOG(incorrectParamsMessage);
}

void AnalyticsCommand::NetcodeHandler(const std::vector<std::string>& params)
{
}

void AnalyticsCommand::OnMapExit()
{
	EndSession();
}

void AnalyticsCommand::OnPluginUnload()
{
	EndSession();

	//wait until all requests are done
	while (reqManager.IsProcessingRequests()) {}
}

std::string AnalyticsCommand::GetNetcodeIdentifier()
{
	return "AL";
}

void AnalyticsCommand::TrackInitCommand(const std::vector<std::string>& params)
{
	if (params.size() != 3) {
		LOG("The analytics trackinit command expects 2 parameters, a project ID and an API Key. For details on how to obtain, consult https://analytics.rocketleaguemapmaking.com/");
		return;
	}

	if (IsSessionActive()) {
		LOG("There is already a session active, cannot start a new one.");
		return;
	}

	StartSession(params[1], params[2]);
}

void AnalyticsCommand::TrackEventCommand(const std::vector<std::string>& params)
{
	if (params.size() == 1) {
		LOG("The analytics trackevent command expects expects json data to pass to the API for tracking events. For more information check https://analytics.rocketleaguemapmaking.com/");
		return;
	}

	if (!IsSessionActive()) {
		LOG("There must be a session active in order to track events. Start one first with analytics trackinit.");
		return;
	}

	std::span<const std::string> subView(params);
	auto rebuiltParams = Utils::ConcatVectorByDelim(subView.subspan(1, params.size()));

	nlohmann::json playerObj;

	try {
		playerObj = GetPlayerJson();
	}
	catch (std::exception& e) {
		LOG("{}", e.what());
		return;
	}

	nlohmann::json j;

	try {
		j = nlohmann::json::parse(rebuiltParams);
	}
	catch (nlohmann::json::parse_error& e) {
		LOG("Error parsing json param: {}", e.what());
		return;
	}

	if (!j.contains("name")) {
		LOG("The name property must be set in the passed object");
		return;
	}

	if (!j.contains("params")) {
		j["params"] = nlohmann::json({});
	}

	j["params"]["player"] = playerObj;
	j["player_id"] = playerObj["id"];
	j["timestamp"] = Utils::GetCurrentUTCTimeStamp();

	debounce.StartOrRestartEventDebounce();

	eventsMutex.lock();
	eventList.push_back(j);
	eventsMutex.unlock();

	LOG("Event {} added to event queue", j.dump());
}

void AnalyticsCommand::TrackEndCommand(const std::vector<std::string>& params)
{
	if (!IsSessionActive()) {
		LOG("The analytics trackend command can only be called to stop an existing session. Start one first with analytics trackinit.");
		return;
	}

	EndSession();
}

void AnalyticsCommand::StartSession(const std::string& projId, const std::string& apiId)
{
	if (IsSessionActive()) return;

	nlohmann::json playerObj;

	try {
		playerObj = GetPlayerJson();
	}
	catch (std::exception& e) {
		LOG("{}", e.what());
		return;
	}

	sessionId = Utils::GenUUID();
	apiKey = apiId;
	projectId = projId;

	auto item = GenerateAnalyticsReqItem("");

	item.data = {
		{"id", sessionId},
		{"player_id", playerObj["id"]},
		{"params", nlohmann::json({
			{"player", playerObj}
			})}
	};

	item.successFunc = [this, item](std::string) {
		LOG("Session {} created successfully", item.extraData.at(sessionIdText));
		plugin->SendInfoToMap("Analytics session started");
		};

	item.errorFunc = [this](std::string) {
		LOG("Unsetting session as one was not succesfully made");
		sessionId = "";
		apiKey = "";
		projectId = "";
		};
	
	reqManager.AddRequestToQueue(item);
}

void AnalyticsCommand::EndSession()
{
	if (!IsSessionActive()) return;

	auto item = GenerateAnalyticsReqItem("/" + sessionId + "/expire");
	item.data = "";

	item.successFunc = [this, item](std::string) {
		LOG("Session {} ended successfully", item.extraData.at(sessionIdText));
		plugin->SendInfoToMap("Analytics session stopped");
		};

	eventsMutex.lock();
	float timeoutReq = 0.0f;
	if (eventList.size() > 0) {
		debounce.StopEventDebounce();
		timeoutReq = 0.1f;
	}

	plugin->gameWrapper->SetTimeout([this, item](GameWrapper*) {
		reqManager.AddRequestToQueue(item);
		sessionId = "";
		projectId = "";
		apiKey = "";
		}, timeoutReq);
	eventsMutex.unlock();
}

nlohmann::json AnalyticsCommand::GetPlayerJson() const
{
	nlohmann::json playerObj;
	std::string playerId = "";
	std::string platformStr = "";

	auto localPlayer = plugin->gameWrapper->GetPlayerController();
	if (!localPlayer) {
		throw std::exception("Can't retrieve local players id. Not creating a session");
	}

	auto playerPRI = localPlayer.GetPRI();
	if (!playerPRI) {
		throw std::exception("Can't retrieve local players id. Not creating a session");
	}

	auto playerNameObj = plugin->gameWrapper->GetPlayerName();
	if (!playerNameObj) {
		throw std::exception("Can't retrieve local players name. Not creating a session");
	}

	playerObj["id"] = std::to_string(playerPRI.GetUniqueIdWrapper().GetUID());
	playerObj["name"] = playerNameObj.ToString();

	switch (playerPRI.GetUniqueIdWrapper().GetPlatform()) {
	case OnlinePlatform_Steam:
		playerObj["platform"] = "steam";
		break;
	case OnlinePlatform_Epic:
		playerObj["platform"] = "epic";
		break;
	default:
		playerObj["platform"] = "unknown";
	}

	return playerObj;
}

Utils::RequestItem AnalyticsCommand::GenerateAnalyticsReqItem(const std::string& endpoint) const
{
	Utils::RequestItem item;

	item.url = apiBase + projectId + "/sdk/sessions" + endpoint;
	item.method = "POST";

	item.headers = {};
	item.headers["Content-Type"] = "application/json";
	item.headers["Authorization"] = "Bearer " + apiKey;

	item.extraData = {};
	item.extraData[apiKeyText] = apiKey;
	item.extraData[sessionIdText] = sessionId;
	item.extraData[projectIdText] = projectId;

	return item;
}

bool AnalyticsCommand::IsSessionActive() inline const
{
	return sessionId != "";
}

void AnalyticsCommand::OnEventDebounceEnd()
{
	eventsMutex.lock();

	if (eventList.size() <= 0) {
		eventsMutex.unlock();
		return;
	}

	nlohmann::json eventsArray = nlohmann::json::array();
	for (auto j : eventList) {
		eventsArray.push_back(j);
	}

	nlohmann::json eventsObj;
	eventsObj["events"] = eventsArray;

	auto item = GenerateAnalyticsReqItem("/" + sessionId + "/events");
	item.data = eventsObj;

	item.successFunc = [this, item](std::string) {
		LOG("Events for {} sent successfully", item.extraData.at(sessionIdText));
		plugin->SendInfoToMap("Analytics event tracked");
		};

	reqManager.AddRequestToQueue(item);

	eventList.clear();
	eventsMutex.unlock();
}
