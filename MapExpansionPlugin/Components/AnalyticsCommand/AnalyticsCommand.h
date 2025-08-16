#pragma once

#include "../BaseCommand.h"
#include <nlohmann/json.hpp>
#include "../../Utility/Debounce.h"
#include "../../Utility/WebRequestManager.h"

class AnalyticsCommand : public BaseCommand {
	inline static const char* apiKeyText = "apiKey";
	inline static const char* sessionIdText = "sessionId";
	inline static const char* projectIdText = "projectId";

	Utils::WebRequestManager reqManager;
	std::string sessionId;
	std::string apiKey;
	std::string projectId;
	const std::string apiBase = "https://analytics.ghostrider-05.com/api/v1/projects/";
	
	std::mutex eventsMutex;
	std::vector<nlohmann::json> eventList;
	Utils::Debounce<> debounce;

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
	nlohmann::json GetPlayerJson() const;
	Utils::RequestItem GenerateAnalyticsReqItem(const std::string& endpoint) inline const;

	void OnEventDebounceEnd();
};