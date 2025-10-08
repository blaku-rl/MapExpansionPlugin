#pragma once
#include <nlohmann/json.hpp>

struct MEPSettings {
	static const int CURRENT_VERSION = 1;
	int version = 0;
	bool analyticsMessageShown;
	bool analyticsAllowed;
	bool loggingAllowed;

	void SetDefaultValues();
	bool IsCurrentVersion() const;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MEPSettings, version, analyticsMessageShown, analyticsAllowed, loggingAllowed)