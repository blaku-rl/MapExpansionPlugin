#include "pch.h"
#include "Utils.h"
#include <sstream>
#include <algorithm>

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
