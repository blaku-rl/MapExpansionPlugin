#pragma once
#include <string>
#include <vector>

namespace Utils {
	std::vector<std::string> SplitStringByChar(const std::string& str, const char& sep);
	bool IsNumeric(const std::string& str);
	bool IsAlphaNumeric(const std::string& str);
	std::string& TrimString(std::string& str, const std::string& whitespace = " \r\n\t\v\f");
}