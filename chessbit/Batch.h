#pragma once

#include "BoardState.h"
#include "TranspositionTable.h"
#include <new>

using namespace bstate;
using namespace tt;

namespace batch {
     struct Batch {
        static constexpr int MAX = 256;
        BoardState moves[MAX];
        uint64_t   keys[MAX];
        int size = 0;

        template <int depth, bool side, bool quiet, bool useTT, class Build>
        __forceinline void add(Build&& build) noexcept {
            BoardState& b = *::new (&moves[size]) BoardState(build());

            if constexpr (useTT) tt::prefetch<depth - 1>(b.zobrist);

            int32_t ord;
            if constexpr (!quiet) {
                ord = CAPTURE_BASE + (PIECE_VALUE[b.vctm] * 16 - PIECE_VALUE[b.atkr]) + captHistory[side][b.atkr][b.to][b.vctm] / 32;
            }
            else {
                const uint16_t pm = packMove(b.from, b.to);
                if      (pm == killers[depth][0])   ord = KILLER_1;
                else if (pm == killers[depth][1])   ord = KILLER_2;
                else                                ord = -b.score + history[side][b.from][b.to];
            }
            keys[size] = (uint64_t(uint32_t(ord) ^ 0x80000000u) << 32) | uint32_t(size);
            ++size;
        }

        __forceinline void reset() noexcept { size = 0; }

        __forceinline BoardState& operator[](int i) noexcept { 
            return moves[keys[i] & 0xFFFF]; 
        }

        __forceinline const BoardState& operator[](int i) const noexcept { 
            return moves[keys[i] & 0xFFFF]; 
        }

        __forceinline void pick(int i) noexcept {
            int best = i;
            uint64_t bestKey = keys[i];
            for (int j = i + 1; j < size; ++j) {
                if (keys[j] > bestKey) { best = j; bestKey = keys[j]; }
            }
            if (best != i) std::swap(keys[i], keys[best]);
        }
    };

    inline Batch batch;
}