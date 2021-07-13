#include "pch.h"
#include "MapExpansionPlugin.h"
#include <bakkesmod/wrappers/kismet/SequenceWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceVariableWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceOpWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceObjectWrapper.h>


BAKKESMOD_PLUGIN(MapExpansionPlugin, "write a plugin description here", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void MapExpansionPlugin::onLoad()
{
	_globalCvarManager = cvarManager;
	gameWrapper->HookEvent("Function TAGame.Car_TA.SetVehicleInput", bind(&MapExpansionPlugin::OnPhysicsTick, this, std::placeholders::_1));
}

void MapExpansionPlugin::onUnload()
{
}

void MapExpansionPlugin::OnPhysicsTick(std::string eventName)
{
	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;

	auto allVars = sequence.GetAllSequenceVariables(false);
	auto command = allVars.find("bmcommand");
	if (command == allVars.end() || !command->second.IsString() || command->second.GetString() == "") return;

	cvarManager->executeCommand(command->second.GetString());
	command->second.SetString("");
}