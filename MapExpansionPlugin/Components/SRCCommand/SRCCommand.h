#pragma once

#include <functional>
#include "../BaseCommand.h"

class SRCCommand : public BaseCommand {
	const std::string srcBaseUrl = "https://www.speedrun.com/api/v1/";
public:
	explicit SRCCommand(BasePlugin* plugin);
	void CommandFunction(const std::vector<std::string>& params) override;
	void NetcodeHandler(const std::vector<std::string>& params) override;
	std::string GetNetcodeIdentifier() override;
private:
	void SendSRCRequestWithRetries(const CurlRequest& req, std::function<std::string(const std::string& data)> successFunc, const int& retries = 5);
	void SendRawSRCQuery(const std::vector<std::string>& params);
	void SendFormattedRequest(const std::vector<std::string>& params);
	void FindSpecificInfo(const std::vector<std::string>& params, const std::string& param);
	std::pair<CurlRequest, int> BuildRequestFromInput(const std::vector<std::string>& params);
	std::unordered_map<std::string, std::string> ParseKeyValueString(const std::string& keyValStr);
	std::string FindAndDeleteKey(const std::string& key, std::unordered_map<std::string, std::string>& params);
	std::string ParseFormattedRequest(const std::string& data, const int& place);
	std::string GetTimeStringFromFloat(const float& time);
	std::string EnsureTwoWideNumber(const int& num);
};