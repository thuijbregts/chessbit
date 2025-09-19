#pragma once

#include "Definitions.h"

using namespace defs;

namespace tt {

	struct Entry {
		U64 zobrist;
		U64 nodes;

		constexpr Entry() : zobrist(0), nodes(0) { }
	};

	constexpr int SIZE = (1 << 20);
	constexpr U64 MASK = SIZE - 1;

	inline Entry** TT;

	static inline void init() {
		TT = new Entry *[18];
		for (int i = 0; i < 18; i++) {
			TT[i] = new Entry[SIZE];
		}
	}
}