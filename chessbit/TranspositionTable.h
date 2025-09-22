#pragma once

#include "Definitions.h"

using namespace defs;

namespace tt {

	struct Entry {
		uint8_t zobrist;
		U64 key;
		U64 nodes;
	};

	//max on 9800x3d
	constexpr int SIZE = (1 << 28);
	constexpr U64 MASK = SIZE - 1;

	inline Entry** TT;

	static inline void init() {
		TT = new Entry*[MAX_DEPTH];
		for (int i = 0; i < MAX_DEPTH; i++) {
			TT[i] = new Entry[SIZE];
		}
	}
}