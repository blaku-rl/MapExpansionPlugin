#pragma once

#include "../BaseCommand.h"
#include "../../Includes/uuid.h"
#include <nlohmann/json.hpp>

class AnalyticsCommand : public BaseCommand {
	std::string sessionId;
	std::string apiKey;
	std::string projectId;
	const std::string apiBase = "https://analytics.ghostrider-05.com/api/v1/projects/";

public:
	explicit AnalyticsCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	void OnMapExit(const bool& wait) override;
	std::string GetNetcodeIdentifier() override;

private:
	void TrackInitCommand(const std::vector<std::string>& params);
	void TrackEventCommand(const std::vector<std::string>& params);
	void TrackEndCommand(const std::vector<std::string>& params);
	bool IsSessionActive() inline const;
	void StartSession(const std::string& projId, const std::string& apiId);
	void EndSession();
	void SendAnalyticsRequest(const std::string& endpoint, const nlohmann::json& params);
	std::string GenUUID() const;
	std::pair<std::string, std::string> GetUserInfo() const;
};