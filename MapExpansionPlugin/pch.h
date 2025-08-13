#pragma once

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include "bakkesmod/plugin/bakkesmodplugin.h"

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "imgui/imgui.h"

#include "fmt/core.h"
#include "fmt/ranges.h"

extern std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
extern bool loggingIsAllowed;

template<typename S, typename... Args>
void LOG(const S& format_str, Args&&... args)
{
	if (!loggingIsAllowed) return;
	_globalCvarManager->log(fmt::format(format_str, args...));
}