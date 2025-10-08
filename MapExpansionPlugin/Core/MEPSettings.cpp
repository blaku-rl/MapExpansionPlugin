#include "pch.h"
#include "MEPSettings.h"

void MEPSettings::SetDefaultValues()
{
	if (version >= CURRENT_VERSION) {
		version = CURRENT_VERSION;
		return;
	}

	version = CURRENT_VERSION;
	analyticsAllowed = true;
	analyticsMessageShown = false;
	loggingAllowed = true;
}

bool MEPSettings::IsCurrentVersion() const
{
	return version == CURRENT_VERSION;
}
