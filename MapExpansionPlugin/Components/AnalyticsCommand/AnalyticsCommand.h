#pragma once

#include "../BaseCommand.h"
#include "../../Includes/uuid.h"

class AnalyticsCommand : public BaseCommand {
	std::string sessionId;
	std::string mapAPIId;
	const std::string apiBase = "https://analytics.ghostrider-05.com/api/v1/projects/";
	std::unique_ptr<uuids::uuid_random_generator> uuidGenerator = nullptr;

public:
	explicit AnalyticsCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	void OnMapExit() override;
	std::string GetNetcodeIdentifier() override;

private:
	void TrackInitCommand(const std::vector<std::string>& params);
	void TrackEventCommand(const std::vector<std::string>& params);
	void TrackEndCommand(const std::vector<std::string>& params);
	bool IsSessionActive() inline const;
	void StartSession(const std::string& apiId);
	void EndSession();
	void SendAnalyticsRequest(const std::string& endpoint, const std::string& params);
};