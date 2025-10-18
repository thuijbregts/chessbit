#pragma once
#include "Definitions.h"
#include <atomic>

using namespace defs;

namespace tt {

    struct Entry {
        U64 high;
        U64 low;
        U64 nodes;
    };

    constexpr U64 SIZE = 1ULL << 28;
    constexpr U64 MASK = SIZE - 1;

    inline Entry** TT;

    static inline void init() {
        TT = new Entry*[MAX_DEPTH];
        for (int d = 2; d <= 7; ++d) {
            TT[d] = new Entry[SIZE];
        }
    }

    template <int depth>
    __forceinline static void write(Zobrist zobrist, U64 nodes) {
        Entry& e = TT[depth][zobrist.high & MASK];
        e.high = zobrist.high ^ nodes;
        e.low = zobrist.low ^ nodes;
        e.nodes = nodes;
    }
}
