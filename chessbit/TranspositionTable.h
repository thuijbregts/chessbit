#pragma once
#include "Definitions.h"

using namespace defs;

namespace tt {

    struct alignas(16) Entry { U64 key; U64 nodes; };

    constexpr U64 SIZE = 1ULL << 28;
    constexpr U64 MASK = SIZE - 1;

    inline Entry** TT;

    static inline void init() {
        TT = new Entry*[MAX_DEPTH];
        for (int d = 2; d < 13; ++d) {
            TT[d] = new Entry[SIZE];
        }
    }

    template <int depth>
    __forceinline static void write(Zobrist zobrist, U64 nodes) {
        Entry& e = TT[depth][zobrist.low & MASK];
        e.key = zobrist.high ^ nodes;
        e.nodes = nodes;
    }
}
