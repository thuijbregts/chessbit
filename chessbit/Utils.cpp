#include "Utils.h"
#include "BoardState.h"
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <bit>
#include <cstdint>

using namespace defs;
using namespace bstate;

#if defined(_WIN32)
	#include <windows.h>
	U64 utils::availableMemory()
	{
		MEMORYSTATUSEX mem{};
		mem.dwLength = sizeof(mem);
		GlobalMemoryStatusEx(&mem);
		return mem.ullAvailPhys;
	}
#elif defined(__linux__)
	#include <unistd.h>

	U64 utils::availableMemory()
	{
		return U64(sysconf(_SC_AVPHYS_PAGES)) *
			U64(sysconf(_SC_PAGE_SIZE));
	}
#elif defined(__APPLE__)
	#include <mach/mach.h>

	U64 utils::availableMemory()
	{
		vm_statistics64_data_t vm;
		mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;

		host_statistics64(
			mach_host_self(),
			HOST_VM_INFO64,
			reinterpret_cast<host_info64_t>(&vm),
			&count);

		return U64(vm.free_count) * U64(getpagesize());
	}
#else
	U64 utils::availableMemory()
	{
		return 512;
	}
#endif

std::vector<std::string> utils::split(const std::string& str, const char delim)
{
	std::vector<std::string> result;
	std::string elem;
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

std::string utils::getMoveSimple(const BoardState& move) {
	std::string result;

	result += SQUARE_NAMES[move.from];
	result += SQUARE_NAMES[move.to];
	if (move.promo) {
		result += ASCII_PIECES[0][move.promoted];
	}
	
	return result;
}

bool utils::validMove(std::string& move) {
	return regex_match(move, MOVE_REGEX);
}

bool utils::isPositiveDigits(std::string& str) {
	return str.find_first_not_of("0123456789") == std::string::npos;
}