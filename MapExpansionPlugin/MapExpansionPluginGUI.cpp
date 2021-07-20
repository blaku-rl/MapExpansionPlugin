#include "pch.h"
#include "MapExpansionPlugin.h"

void MapExpansionPlugin::RenderSettings()
{
	if (!isSetupComplete) {
		ImGui::TextUnformatted("Map Expansion Plugin is still loading");
	}
	else {
		ImGui::TextUnformatted("The Map Expansion Plugin is designed for map makers to leverage bakkesmod with their maps");
		ImGui::TextUnformatted("Usage Details can be found here: https://github.com/blaku-rl/MapExpansionPlugin");
	}
}

std::string MapExpansionPlugin::GetPluginName()
{
	return "Map Expansion Plugin 0.1";
}

// Don't call this yourself, BM will call this function with a pointer to the current ImGui context
void MapExpansionPlugin::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}