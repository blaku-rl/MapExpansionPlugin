#include "pch.h"
#include "AnalyticsCommand.h"
#include "../../Utility/Utils.h"
#include <nlohmann/json.hpp>

AnalyticsCommand::AnalyticsCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
	std::random_device rd;
	auto threadIdHash = std::hash<std::thread::id>{}(std::this_thread::get_id());
	uint64_t mixedSeed = static_cast<uint64_t>(rd()) ^ static_cast<uint64_t>(threadIdHash);

	std::mt19937 generator(static_cast<unsigned int>(mixedSeed));
	uuidGenerator = std::make_unique<uuids::uuid_random_generator>(generator);
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

void AnalyticsCommand::OnMapExit()
{
	EndSession();
}

std::string AnalyticsCommand::GetNetcodeIdentifier()
{
	return "AL";
}

void AnalyticsCommand::TrackInitCommand(const std::vector<std::string>& params)
{
	if (params.size() != 2) {
		LOG("The analytics trackinit command expects 1 parameter and that is your API key for you map. To obtain your key, consult https://analytics.rocketleaguemapmaking.com/");
		return;
	}

	if (IsSessionActive()) {
		LOG("There is already a session active, cannot start a new one.");
		return;
	}

	StartSession(params[1]);
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
	
	SendAnalyticsRequest(mapAPIId + "sdk/sessions/" + sessionId + "/events", rebuiltParams);
}

void AnalyticsCommand::TrackEndCommand(const std::vector<std::string>& params)
{
	if (!IsSessionActive()) {
		LOG("The analytics trackend command can only be called to stop an existing session. Start one first with analytics trackinit.");
		return;
	}

	EndSession();
}

void AnalyticsCommand::StartSession(const std::string& apiId)
{
	if (IsSessionActive()) return;

	auto localPlayer = plugin->gameWrapper->GetPlayerController();
	if (!localPlayer) {
		LOG("Can't retrieve local players id. Not creating a session");
		return;
	}

	auto playerPRI = localPlayer.GetPRI();
	if (!playerPRI) {
		LOG("Can't retrieve local players id. Not creating a session");
		return;
	}

	auto playerId = playerPRI.GetUniqueIdWrapper().GetEpicAccountID();

	auto& gen = *uuidGenerator.get();
	sessionId = uuids::to_string(gen());

	nlohmann::json j = {
		{"id", sessionId},
		{"params", {
			{"user_id", playerId}
		}}
	};
	
	SendAnalyticsRequest(apiId + "/sdk/sessions", j.dump());
}

void AnalyticsCommand::EndSession()
{
	if (!IsSessionActive()) return;
	
	SendAnalyticsRequest(mapAPIId + "/sdk/sessions/" + sessionId + "/expire", "");

	sessionId = "";
	mapAPIId = "";
}

void AnalyticsCommand::SendAnalyticsRequest(const std::string& endpoint, const std::string& params)
{
	CurlRequest req;
	req.url = apiBase + endpoint;
	req.verb = "POST";
	req.headers = {};
	req.headers["Content-Type"] = "application/json";
	req.headers["User-Agent"] = "MEP (" + std::string(plugin->GetPluginVersion()) + ")";
	req.body = params;

	LOG("Sending request to {}", req.url);
	std::string sentSessionId = sessionId;
	HttpWrapper::SendCurlJsonRequest(req, [this, params, sentSessionId](int code, std::string data) mutable {
		if (code == 201) {
			LOG("Session {} created successfully", sentSessionId);
			plugin->SendInfoToMap("Analytics session started");
			return;
		}

		if (code == 204) {
			if (params == "") {
				LOG("Session {} ended successfully", sentSessionId);
				plugin->SendInfoToMap("Analytics session stopped");
			}
			else {
				LOG("Events for {} sent successfully", sentSessionId);
				plugin->SendInfoToMap("Analytics event tracked");
			}
			return;
		}

		LOG("Error code {} with message {}", std::to_string(code), data);
		plugin->SendInfoToMap("Analytics error");

		nlohmann::json j = nlohmann::json::parse(params);
		if (j.contains("id") and j["id"].is_string() and j["id"] == sessionId) {
			LOG("Unsetting session as one was not succesfully made");
			sessionId = "";
			mapAPIId = "";
		}
		});
}

bool AnalyticsCommand::IsSessionActive() inline const
{
	return sessionId != "";
}