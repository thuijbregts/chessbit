#pragma once

#include "Eval.h"
#include "TranspositionTable.h"
#include "See.h"
#include <new>

using namespace bstate;
using namespace tt;

namespace batch {
     struct Batch {
        static constexpr int MAX = 256;
        BoardState moves[MAX];
        uint64_t   keys[MAX];
        int size = 0;
        uint16_t ttMove;

        template <int depth, bool side, bool cap, bool useTT, bool promo = false, class Build>
        __forceinline void add(Build&& build) noexcept {
            BoardState& b = *::new (&moves[size]) BoardState(build());

            if constexpr (useTT && tt::USE_HASH<depth>) tt::prefetch<depth - 1>(b.zobrist);

            int32_t ord;
            if (ttMove && packMove(b.from, b.to) == ttMove) ord = TT_MOVE_SCORE;
            else {
                if constexpr (promo) {
                    ord = PROMOTION_BASE + PIECE_VALUE[b.promoted] * 32;
                    if constexpr (cap) {
                        int mvvLva = MVV_LVA[b.vctm][b.atkr];
                        if (mvvLva > 0) ord += mvvLva + captHistory[side][b.atkr][b.to][b.vctm] / 32;
                        else {
                            int score = see::see(b);
                            if (score < 0)  ord = -b.score - score;
                            else            ord += score * 16 + captHistory[side][b.atkr][b.to][b.vctm] / 32;
                        }
                    }
                }
                else if constexpr (cap) {
                    int mvvLva = MVV_LVA[b.vctm][b.atkr];
                    if (mvvLva > 0) ord = CAPTURE_BASE + mvvLva + captHistory[side][b.atkr][b.to][b.vctm] / 32;
                    else {
                        int score = see::see(b);
                        if (score < 0)  ord = -b.score - score;
                        else            ord = CAPTURE_BASE + score * 16 + captHistory[side][b.atkr][b.to][b.vctm] / 32;
                    } 
                }
                else {
                    const uint16_t pm = packMove(b.from, b.to);
                    if (pm == killers[depth][0])        ord = KILLER_1;
                    else if (pm == killers[depth][1])   ord = KILLER_2;
                    else                                ord = -b.score + history[side][b.from][b.to];
                }
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