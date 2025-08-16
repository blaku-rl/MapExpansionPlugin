#include "pch.h"
#include "Utils.h"
#include "../../Includes/uuid.h"
#include <sstream>
#include <algorithm>
#include <chrono>
#include <format>
#include <Windows.h>
#include <shellapi.h>

std::vector<std::string> Utils::SplitStringByChar(const std::string& str, const char& sep)
{
	std::vector<std::string> splitString;
	std::stringstream splitStringStream(str);
	std::string stringSegment;
	while (std::getline(splitStringStream, stringSegment, sep)) {
		splitString.push_back(stringSegment);
	}

	return splitString;
}

std::string Utils::ConcatVectorByDelim(const std::span<const std::string>& vec, const char& sep)
{
	return std::ranges::fold_left(vec, "", [sep](std::string acc, std::string part) {return acc + sep + part; });
}

bool Utils::IsNumeric(const std::string& str)
{
	return str.find_first_not_of("0123456789") == std::string::npos;
}

bool Utils::IsAlphaNumeric(const std::string& str)
{
	return std::find_if(str.begin(), str.end(), [](char c) { return !isalnum(c); }) == str.end();
}

static std::string& ltrim(std::string& str, std::string const& whitespace = " \r\n\t\v\f")
{
	str.erase(0, str.find_first_not_of(whitespace));
	return str;
}

static std::string& rtrim(std::string& str, std::string const& whitespace = " \r\n\t\v\f")
{
	str.erase(str.find_last_not_of(whitespace) + 1);
	return str;
}

std::string& Utils::TrimString(std::string& str, const std::string& whitespace)
{
	return ltrim(rtrim(str, whitespace), whitespace);
}

std::string Utils::GetCurrentUTCTimeStamp()
{
	const auto now = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::utc_clock::now());
	return std::format("{:%F %T}", now);
}

std::string Utils::GenUUID()
{
	std::random_device rd;
	auto threadIdHash = std::hash<std::thread::id>{}(std::this_thread::get_id());
	uint64_t mixedSeed = static_cast<uint64_t>(rd()) ^ static_cast<uint64_t>(threadIdHash);

	std::mt19937 generator(static_cast<unsigned int>(mixedSeed));
	auto uuidGenerator = std::make_unique<uuids::uuid_random_generator>(generator);
	auto& gen = *uuidGenerator.get();

	return uuids::to_string(gen());
}

void Utils::OpenURL(const std::string& url) 
{
	ShellExecute(NULL, NULL, url.c_str(), NULL, NULL, SW_SHOWNOACTIVATE);
}