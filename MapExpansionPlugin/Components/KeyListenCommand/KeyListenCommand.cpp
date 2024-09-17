#include "pch.h"
#include "KeyListenCommand.h"
#include "../../Utility/Utils.h"

KeyListenCommand::KeyListenCommand(BasePlugin* plugin) : BaseCommand(plugin)
{
}

void KeyListenCommand::CommandFunction(const std::vector<std::string>& params)
{
	if (params.size() != 2) {
		LOG(
			L"keylisten command expects 2 parameters. First is a comma separated list(without spaces) of keys that are required."
			"The second is the name of the remote event that will be called once the required keys are pressed");
		return;
	}

	MapBind curBind;
	auto keyList = Utils::SplitStringByChar(params[0], ',');
	for (auto& key : keyList) {
		if (!plugin->DoesKeyExist(key)) {
			LOG("Key '{}' is not a supported key. View documentation for supported key names. keylisten command will not be added", key);
			return;
		}
		curBind.keyListFnameIndex.push_back(plugin->GetIndexFromKey(key));
	}

	curBind.remoteEvent = params[1];
	curBind.allKeysPressed = false;

	plugin->AddKeyBind(curBind);
	LOG("Making bind with keys '{}' and remote event '{}'", params[0], params[1]);
}

void KeyListenCommand::NetcodeHandler(const std::vector<std::string>& params)
{
}

std::string KeyListenCommand::GetNetcodeIdentifier()
{
	return "KL";
}
