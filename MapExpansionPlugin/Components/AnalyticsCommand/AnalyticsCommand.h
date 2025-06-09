#pragma once

#include "../BaseCommand.h"
#include "../../Includes/uuid.h"
#include <nlohmann/json.hpp>

enum EventThreadState {
	STOPPED,
	RUNNING,
	RESTARTING,
	STOPPING,

};

struct AnalyticsAPIRequestItem {
	std::string endpoint;
	std::string sentSessionId;
	std::string sentAPIKey;
	std::string sentProjectId;
	nlohmann::json param;
	std::function<void(std::string)> successFunc;
	std::function<void()> errorFunc = nullptr;
};

class AnalyticsCommand : public BaseCommand {
	std::atomic_bool processingRequest;
	std::string sessionId;
	std::string apiKey;
	std::string projectId;
	const std::string apiBase = "https://analytics.ghostrider-05.com/api/v1/projects/";

	std::mutex requestMutex;
	std::queue<AnalyticsAPIRequestItem> requestQueue;
	
	std::mutex eventsMutex;
	std::vector<nlohmann::json> eventList;
	std::thread eventDebounce;
	std::atomic<EventThreadState> threadState;

public:
	explicit AnalyticsCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	void OnMapExit() override;
	void OnPluginUnload() override;
	std::string GetNetcodeIdentifier() override;

private:
	void TrackInitCommand(const std::vector<std::string>& params);
	void TrackEventCommand(const std::vector<std::string>& params);
	void TrackEndCommand(const std::vector<std::string>& params);
	bool IsSessionActive() inline const;
	void StartSession(const std::string& projId, const std::string& apiId);
	void EndSession();
	std::string GenUUID() const;
	std::pair<std::string, std::string> GetUserInfo() const;

	void AddRequestToQueue(AnalyticsAPIRequestItem item);
	void ProcessNextRequest();
	void SendAnalyticsRequest(const AnalyticsAPIRequestItem& item);

	void StartOrRestartEventDebounce();
	void StopEventDebounce();
	void StartEventDebounce();
	void EndDebounceThread();
	void OnEventDebounceEnd();
};