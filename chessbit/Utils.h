#ifndef UTILS_H
#define UTILS_H

#include "Definitions.h"
#include <vector>
#include <string>
#include <regex>

namespace bstate {
	struct BoardState;
}

namespace utils {
	const std::regex MOVE_REGEX{ "([a-h]{1}[1-8]{1}){2}[RNBQ]?" };

	U64 availableMemory();
	std::vector<std::string> split(const std::string& str, const char delim);
	std::string getMoveSimple(const bstate::BoardState& move);
	bool validMove(std::string& move);
	bool isPositiveDigits(std::string& str);
}

#endif