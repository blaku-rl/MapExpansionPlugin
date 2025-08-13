#include "pch.h"
#include "WebRequestManager.h"

Utils::WebRequestManager::WebRequestManager(BasePlugin* plugin) : plugin(plugin)
{
	processingRequest = false;
	requestQueue = {};
}

bool Utils::WebRequestManager::IsProcessingRequests() inline const
{
	return processingRequest;
}

void Utils::WebRequestManager::AddRequestToQueue(RequestItem item)
{
	requestMutex.lock();
	requestQueue.push(item);
	requestMutex.unlock();

	if (processingRequest) return;

	ProcessNextRequest();
}

void Utils::WebRequestManager::ProcessNextRequest() 
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
	SendRequest(requestQueue.front());
	requestMutex.unlock();
}

void Utils::WebRequestManager::SendRequest(const RequestItem& item)
{
	CurlRequest req;
	req.url = item.url;
	req.verb = item.method;
	req.headers = {};
	for (const auto& [name, value] : item.headers)
		req.headers[name] = value;
	req.headers["User-Agent"] = "MEP (" + std::string(plugin->GetPluginVersion()) + ")";
	req.body = item.data.dump();

	LOG("Sending request to {} with body {}", req.url, req.body);

	HttpWrapper::SendCurlJsonRequest(req, [this, &item](int code, std::string data) mutable {
		plugin->gameWrapper->Execute([this, code, data, &item](GameWrapper*) mutable {
			if (200 <= code and code < 300) {
				item.successFunc(data);
			}
			else {
				LOG("Error code {} with message {}", std::to_string(code), data);

				if (item.errorFunc)
					item.errorFunc(data);
			}

			plugin->gameWrapper->Execute([this](GameWrapper*) {
				ProcessNextRequest();
				});
			});
		});
}