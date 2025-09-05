#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <regex>
using namespace std;

struct Piece;
struct MoveInfo;
class MoveArray;

namespace utils {
	const std::regex MOVE_REGEX{ "([a-h]{1}[1-8]{1}){2}[RNBQ]?" };

	vector<string> split(const string& str, const char delim);
	string getMoveSimple(MoveInfo& move);
	bool validMove(string& move);
	bool isPositiveDigits(string& str);
}

#endif