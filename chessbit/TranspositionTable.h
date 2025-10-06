#pragma once
#include "Definitions.h"
#include <atomic>

using namespace defs;

namespace tt {

    struct Entry {
        std::atomic<U64> high;
        std::atomic<U64> low;
        std::atomic<U64> nodes;
    };

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
        Entry& e = TT[depth][zobrist.high & MASK];
        e.high.store(zobrist.high ^ nodes, std::memory_order_relaxed);
        e.low.store(zobrist.low ^ nodes, std::memory_order_relaxed);
        e.nodes.store(nodes, std::memory_order_relaxed);
    }

    template <int depth>
    __forceinline static bool read(Zobrist zobrist, U64& nodes) {
        Entry& e = TT[depth][zobrist.high & MASK];
        U64 h = e.high.load(std::memory_order_relaxed);
        U64 l = e.low.load(std::memory_order_relaxed);
        U64 n = e.nodes.load(std::memory_order_relaxed);
        if ((h ^ n) == zobrist.high && (l ^ n) == zobrist.low) {
            nodes = n;
            return true;
        }
        return false;
    }
}
