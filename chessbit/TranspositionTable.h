#pragma once

#include "Definitions.h"

using namespace defs;

namespace tt {

	struct Data {
		U64 zobrist;
		U64 nodes;

		constexpr Data() : zobrist(0), nodes(0) { }
	};

	constexpr int SIZE = (1 << 16);
	constexpr U64 MASK = SIZE - 1;

	inline Data** TT;

	static inline void init() {
		TT = new Data*[18];
		for (int i = 0; i < 18; i++) {
			TT[i] = new Data[SIZE];
		}
	}
}