#pragma once
#include "Definitions.h"
#include <cstdint>
#include <atomic>

using namespace defs;

namespace tt {

    struct Entry {
        U64 key;
        U64 nodes;
    };

    constexpr U64 SIZE = 1ULL << 28;
    constexpr U64 MASK = SIZE - 1;

    inline Entry** TT;

    static inline void init() {
        TT = new Entry*[MAX_DEPTH];
        for (int d = 0; d < MAX_DEPTH; ++d) {
            TT[d] = new Entry[SIZE];
        }
    }

    template <int depth>
    __forceinline static void write(U64 zobrist, U64 nodes) {
        Entry& e = TT[depth][zobrist & MASK];
        e.nodes = nodes;
        e.key = zobrist ^ nodes;
    }
}
