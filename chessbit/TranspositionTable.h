#pragma once
#include "Definitions.h"

using namespace defs;

namespace tt {

	struct alignas(16) Entry { U64 key; U64 nodes; };

	constexpr int SIZE[MAX_DEPTH] = {
		//   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17
			 0,  0, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17,  0,  0,  0
	};

	template <int depth>
	constexpr U64 MASK = (1ULL << SIZE[depth]) - 1;

	inline Entry* TT[MAX_DEPTH];

	static inline void init() {
		for (int d = 0; d < MAX_DEPTH; ++d)
			if (SIZE[d])
				TT[d] = new Entry[1ULL << SIZE[d]];
	}

	template <int depth>
	ForceInline void write(Zobrist zobrist, U64 nodes) {
		Entry& e = TT[depth][zobrist.low & MASK<depth>];
		e.key = zobrist.high ^ nodes;
		e.nodes = nodes;
	}
}