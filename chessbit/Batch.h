#pragma once

#include "BoardState.h"
#include "TranspositionTable.h"

using namespace bstate;
using namespace tt;

namespace batch {
    struct Batch {
        static constexpr int MAX = 256;
        static constexpr int MAX_KING = 8;
        BoardState normal[MAX];
        BoardState king[MAX_KING];
        BoardState merged[MAX];
        int nSize = 0;
        int kSize = 0;

        template <int depth, bool k, bool useTT>
        __forceinline void add(const BoardState& b) noexcept {
            if constexpr (useTT) tt::prefetch<depth - 1>(b.zobrist);

            if constexpr (k)    king[kSize++] = b;
            else                normal[nSize++] = b;
        }

        __forceinline void reset() noexcept {
            nSize = 0;
            kSize = 0;
        }

        __forceinline int size() noexcept {
            return nSize + kSize;
        }

        __forceinline BoardState* moves() noexcept {
            std::copy(king, king + kSize, merged);
            std::copy(normal, normal + nSize, merged + kSize);
            return merged;
        }
    };

    inline Batch batch;
}