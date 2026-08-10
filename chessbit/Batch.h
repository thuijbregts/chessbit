#pragma once

#include "BoardState.h"
#include "TranspositionTable.h"

using namespace bstate;
using namespace tt;

namespace batch {
    struct Batch {
        static constexpr int MAX = 256;
        static constexpr int MAX_KING = 8;
        BoardState moves[MAX];
        int size = 0;

        template <int depth, bool useTT>
        __forceinline void add(const BoardState& b) noexcept {
            if constexpr (useTT) tt::prefetch<depth - 1>(b.zobrist);

            moves[size++] = b;
        }

        __forceinline void reset() noexcept {
            size = 0;
        }
    };

    inline Batch batch;
}