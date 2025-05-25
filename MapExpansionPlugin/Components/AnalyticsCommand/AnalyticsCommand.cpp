#include "pch.h"
#include "AnalyticsCommand.h"
#include "../../Utility/Utils.h"
#include <stdexcept>

AnalyticsCommand::AnalyticsCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
	
}

void AnalyticsCommand::CommandFunction(const std::vector<std::string>& params)
{
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

void AnalyticsCommand::OnMapExit(const bool& wait)
{
	//If we send a request when the plugin is unloading and we are in a session, the return will crash the game
	//Let the session auto expire on the website in this case
	if (!wait) return;
	EndSession();
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

	std::string playerId, platformStr;

	try {
		auto pair = GetUserInfo();
		playerId = pair.first;
		platformStr = pair.second;
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
	
	if (j.contains("events") and j["events"].is_array()) {
		for (int i = 0; i < j["events"].size(); ++i) {
			j["events"].at(i)["user_id"] = playerId;
			j["events"].at(i)["user_platform"] = platformStr;
		}
	}
	
	SendAnalyticsRequest("/" + sessionId + "/events", j);
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

	std::string playerId, platformStr;

	try {
		auto pair = GetUserInfo();
		playerId = pair.first;
		platformStr = pair.second;
	}
	catch (std::exception& e) {
		LOG("{}", e.what());
		return;
	}

	sessionId = GenUUID();
	apiKey = apiId;
	projectId = projId;

	nlohmann::json j = {
		{"id", sessionId},
		{"params", {
			{"user_id", playerId},
			{"user_platform", platformStr}
		}}
	};
	
	SendAnalyticsRequest("", j);
}

void AnalyticsCommand::EndSession()
{
	if (!IsSessionActive()) return;
	
	SendAnalyticsRequest("/" + sessionId + "/expire", "");

	sessionId = "";
	projectId = "";
	apiKey = "";
}

void AnalyticsCommand::SendAnalyticsRequest(const std::string& endpoint, const nlohmann::json& params)
{
	CurlRequest req;
	req.url = apiBase + projectId + "/sdk/sessions" + endpoint;
	req.verb = "POST";
	req.headers = {};
	req.headers["Content-Type"] = "application/json";
	req.headers["User-Agent"] = "MEP (" + std::string(plugin->GetPluginVersion()) + ")";
	req.headers["Authorization"] = "Bearer " + apiKey;
	req.body = params.dump();

	LOG("Sending request to {} with body {}", req.url, req.body);

	std::string sentSessionId = sessionId;
	HttpWrapper::SendCurlJsonRequest(req, [this, params, sentSessionId](int code, std::string data) mutable {
		plugin->gameWrapper->Execute([this, params, sentSessionId, code, data](GameWrapper*) mutable {
			if (code == 204) {
				LOG("Session {} ended successfully", sentSessionId);
				plugin->SendInfoToMap("Analytics session stopped");
				return;
			}

			bool isCreateReq = params.contains("id") and params["id"].is_string() and params["id"] == sessionId;;

			if (code == 201) {
				if (isCreateReq) {
					LOG("Session {} created successfully", sentSessionId);
					plugin->SendInfoToMap("Analytics session started");
				}
				else {
					LOG("Events for {} sent successfully", sentSessionId);
					plugin->SendInfoToMap("Analytics event tracked");
				}
				return;
			}

			LOG("Error code {} with message {}", std::to_string(code), data);
			plugin->SendInfoToMap("Analytics error");

			if (isCreateReq) {
				LOG("Unsetting session as one was not succesfully made");
				sessionId = "";
				apiKey = "";
				projectId = "";
			}
			});
		});
}

std::string AnalyticsCommand::GenUUID() const
{
	std::random_device rd;
	auto threadIdHash = std::hash<std::thread::id>{}(std::this_thread::get_id());
	uint64_t mixedSeed = static_cast<uint64_t>(rd()) ^ static_cast<uint64_t>(threadIdHash);

	std::mt19937 generator(static_cast<unsigned int>(mixedSeed));
	auto uuidGenerator = std::make_unique<uuids::uuid_random_generator>(generator);
	auto& gen = *uuidGenerator.get();

	return uuids::to_string(gen());
}

std::pair<std::string, std::string> AnalyticsCommand::GetUserInfo() const
{
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

	playerId = std::to_string(playerPRI.GetUniqueIdWrapper().GetUID());

	switch (playerPRI.GetUniqueIdWrapper().GetPlatform()) {
	case OnlinePlatform_Steam:
		platformStr = "steam";
		break;
	case OnlinePlatform_Epic:
		platformStr = "epic";
		break;
	default:
		platformStr = "unknown";
	}

	return std::make_pair(playerId, platformStr);
}

bool AnalyticsCommand::IsSessionActive() inline const
{
	return sessionId != "";
}