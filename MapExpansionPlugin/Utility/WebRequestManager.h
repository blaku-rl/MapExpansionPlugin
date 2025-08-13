#pragma once
#include <nlohmann/json.hpp>
#include "../BasePlugin.h"

namespace Utils {
	struct RequestItem {
		std::string url;
		std::string method;
		std::unordered_map<std::string, std::string> headers;
		std::unordered_map<std::string, std::string> extraData;
		std::function<void(std::string)> successFunc;
		std::function<void(std::string)> errorFunc;
		nlohmann::json data;
	};

	class WebRequestManager {
		std::atomic_bool processingRequest;
		std::mutex requestMutex;
		std::queue<RequestItem> requestQueue;
		
		BasePlugin* plugin;

	public:
		WebRequestManager(BasePlugin* plugin);
		bool IsProcessingRequests() inline const;
		void AddRequestToQueue(RequestItem item);

	private:
		void ProcessNextRequest();
		void SendRequest(const RequestItem& item);
	};
}
