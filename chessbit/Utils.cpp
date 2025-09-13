#include <stdexcept>
#include "Utils.h"
#include "Definitions.h"
#include <iomanip>
#include <sstream>

using namespace defs;

vector<string> utils::split(const string& str, const char delim)
{
	vector<string> result;
	string elem;
	for (int i = 0; i < str.length(); ++i) {
		if (str[i] == delim) {
			if (elem.length() > 0) {
				result.push_back(elem);
			}
			elem.clear();
			continue;
		}
		elem += str[i];
	}
	result.push_back(elem);
	return result;
}

string utils::getMoveSimple(MoveInfo& move) {
	string result;

	result += SQUARE_NAMES[move.from];
	result += SQUARE_NAMES[move.to];
	if (move.promo != noPiece) {
		result += ASCII_PIECES[0][move.promo];
	}
	
	return result;
}

bool utils::validMove(string& move) {
	return regex_match(move, MOVE_REGEX);
}

bool utils::isPositiveDigits(string& str) {
	return str.find_first_not_of("0123456789") == std::string::npos;
}