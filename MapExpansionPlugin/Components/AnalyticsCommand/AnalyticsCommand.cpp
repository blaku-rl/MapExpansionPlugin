#include "pch.h"
#include "AnalyticsCommand.h"
#include "../../Utility/Utils.h"
#include <stdexcept>
#include <chrono>

AnalyticsCommand::AnalyticsCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
	threadState = EventThreadState::STOPPED;
	processingRequest = false;
	eventList = {};
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

void AnalyticsCommand::OnPluginUnload()
{
	if (IsSessionActive())
		EndSession();

	//wait until all requests are done
	while (processingRequest) {}
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

	LOG("Third");

	nlohmann::json j;

	try {
		j = nlohmann::json::parse(rebuiltParams);
	}
	catch (nlohmann::json::parse_error& e) {
		LOG("Error parsing json param: {}", e.what());
		return;
	}

	if (!j.contains("name") or !j.contains("params")) {
		LOG("The name and params properties must be set in the passed object");
		return;
	}

	j["player_id"] = playerId;
	j["params"]["player_platform"] = platformStr;
	j["timestamp"] = Utils::GetCurrentUTCTimeStamp();

	

	StartOrRestartEventDebounce();

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

	AnalyticsAPIRequestItem item;
	item.sentSessionId = sessionId;
	item.sentAPIKey = apiKey;
	item.sentProjectId = projId;
	item.endpoint = "";

	item.param = {
		{"id", sessionId},
		{"params", {
			{"player_id", playerId},
			{"player_platform", platformStr}
		}}
	};

	item.successFunc = [this, item](std::string) {
		LOG("Session {} created successfully", item.sentSessionId);
		plugin->SendInfoToMap("Analytics session started");
		};

	item.errorFunc = [this]() {
		LOG("Unsetting session as one was not succesfully made");
		sessionId = "";
		apiKey = "";
		projectId = "";
		};
	
	AddRequestToQueue(item);
}

void AnalyticsCommand::EndSession()
{
	if (!IsSessionActive()) return;

	AnalyticsAPIRequestItem item;
	item.sentSessionId = sessionId;
	item.sentAPIKey = apiKey;
	item.sentProjectId = projectId;
	item.endpoint = "/" + sessionId + "/expire";
	item.param = "";

	item.successFunc = [this, item](std::string) {
		LOG("Session {} ended successfully", item.sentSessionId);
		plugin->SendInfoToMap("Analytics session stopped");
		};

	eventsMutex.lock();
	float timeoutReq = 0.0f;
	if (eventList.size() > 0) {
		StopEventDebounce();
		timeoutReq = 0.1f;
	}

	plugin->gameWrapper->SetTimeout([this, item](GameWrapper*) {
		AddRequestToQueue(item);
		sessionId = "";
		projectId = "";
		apiKey = "";
		}, timeoutReq);
	eventsMutex.unlock();
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

void AnalyticsCommand::AddRequestToQueue(AnalyticsAPIRequestItem item)
{
	requestMutex.lock();
	requestQueue.push(item);
	requestMutex.unlock();

	if (processingRequest) return;

	
	ProcessNextRequest();
}

void AnalyticsCommand::ProcessNextRequest()
{
	requestMutex.lock();
	if (processingRequest) {
		requestQueue.pop();
	}

	if (requestQueue.size() <= 0) {
		processingRequest = false;
		requestMutex.unlock();
		return;
	}

	processingRequest = true;
	SendAnalyticsRequest(requestQueue.front());
	requestMutex.unlock();
}

void AnalyticsCommand::SendAnalyticsRequest(const AnalyticsAPIRequestItem& item)
{
	CurlRequest req;
	req.url = apiBase + item.sentProjectId + "/sdk/sessions" + item.endpoint;
	req.verb = "POST";
	req.headers = {};
	req.headers["Content-Type"] = "application/json";
	req.headers["User-Agent"] = "MEP (" + std::string(plugin->GetPluginVersion()) + ")";
	req.headers["Authorization"] = "Bearer " + item.sentAPIKey;
	req.body = item.param.dump();

	LOG("Sending request to {} with body {}", req.url, req.body);

	HttpWrapper::SendCurlJsonRequest(req, [this, &item](int code, std::string data) mutable {
		plugin->gameWrapper->Execute([this, code, data, &item](GameWrapper*) mutable {
			if (200 <= code and code < 300) {
				item.successFunc(data);
			}
			else {
				LOG("Error code {} with message {}", std::to_string(code), data);
				plugin->SendInfoToMap("Analytics error");

				if (item.errorFunc)
					item.errorFunc();
			}

			plugin->gameWrapper->Execute([this](GameWrapper*) {
				ProcessNextRequest();
				});
			});
		});
}

void AnalyticsCommand::StartOrRestartEventDebounce()
{
	if (threadState == EventThreadState::STOPPED) {
		StartEventDebounce();
	}
	else {
		threadState = EventThreadState::RESTARTING;
	}
}

void AnalyticsCommand::StopEventDebounce()
{
	if (threadState == EventThreadState::STOPPED) return;

	threadState = EventThreadState::STOPPING;
}

void AnalyticsCommand::StartEventDebounce()
{
	if (threadState != EventThreadState::STOPPED) return;

	eventDebounce = std::thread([this]() {
		threadState = EventThreadState::RUNNING;
		int loops = 0;
		do {
			loops++;
			if (threadState == EventThreadState::STOPPING)
				break;
				
			if (threadState == EventThreadState::RESTARTING) {
				threadState = EventThreadState::RUNNING;
				loops = 0;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		} while (loops < 100);

		threadState = EventThreadState::STOPPED;

		plugin->gameWrapper->Execute([this](GameWrapper*) {
			EndDebounceThread();
			});
		});
}

void AnalyticsCommand::EndDebounceThread()
{
	if (eventDebounce.joinable())
		eventDebounce.join();

	OnEventDebounceEnd();
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

	AnalyticsAPIRequestItem item;
	item.sentSessionId = sessionId;
	item.sentAPIKey = apiKey;
	item.sentProjectId = projectId;
	item.endpoint = "/" + sessionId + "/events";
	item.param = eventsObj;

	item.successFunc = [this, item](std::string) {
		LOG("Events for {} sent successfully", item.sentSessionId);
		plugin->SendInfoToMap("Analytics event tracked");
		};

	AddRequestToQueue(item);

	eventList.clear();
	eventsMutex.unlock();
}
