#pragma once
#include <nlohmann/json.hpp>

struct MEPSettings {
	bool analyticsMessageShown;
	bool analyticsAllowed;
	bool loggingAllowed;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MEPSettings, analyticsMessageShown, analyticsAllowed, loggingAllowed)