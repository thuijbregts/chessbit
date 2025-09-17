#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <regex>
#include "Game.h"

using namespace std;
using namespace game;

namespace utils {
	const std::regex MOVE_REGEX{ "([a-h]{1}[1-8]{1}){2}[RNBQ]?" };

	vector<string> split(const string& str, const char delim);
	string getMoveSimple(const MoveInfo& move);
	bool validMove(string& move);
	bool isPositiveDigits(string& str);
}

#endif