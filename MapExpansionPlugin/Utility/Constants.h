#pragma once
#include <string>

namespace Constants {
	namespace SaveData {
		constexpr const std::string nameTag = "KismetVar:";
		constexpr const std::string typeTag = "Type:";
		constexpr const std::string valueTag = "Value:";
	}

	namespace Functions {
		constexpr const char* Tick = "Function Engine.GameViewportClient.Tick";
		constexpr const char* Physics_Tick = "Function TAGame.Car_TA.SetVehicleInput";
		constexpr const char* Keypress = "Function TAGame.GameViewportClient_TA.HandleKeyPress";
		constexpr const char* Maploaded = "Function Engine.HUD.SetShowScores";
		constexpr const char* Mapunloaded = "Function TAGame.GameEvent_Soccar_TA.Destroyed";
	}

	namespace MEP {
		constexpr const char* MEPOutputVarName = "mepoutput";
		constexpr const char* MEPOutputEvent = "MEPOutputEvent";
		constexpr const char* MEPLoadedEvent = "MEPLoaded";
	}
}