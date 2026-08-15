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
        bool useTT = false;
        bool perft = false;
        int depth = 0;
        int ply = 0;
        uint16_t idMove;
        uint16_t ttMove;

        template <bool side, bool cap, bool promo = false, class Build>
        __forceinline void add(Build&& build) noexcept {
            BoardState& b = *::new (&moves[size]) BoardState(build());

            if (useTT) tt::prefetch(b.zobrist, depth - 1);

            if (!perft) {
                int32_t score;
                if (idMove && packMove(b.from, b.to) == idMove) score = ID_MOVE_SCORE;
                else {
                    if constexpr (promo) {
                        score = PROMOTION_BASE + PIECE_VALUE[b.promoted] * 32;
                        if constexpr (cap) {
                            int mvvLva = MVV_LVA[b.vctm][b.atkr];
                            if (mvvLva > 0) score += mvvLva + captHistory[side][b.atkr][b.to][b.vctm] / 32;
                            else {
                                if (!see::seeGE(b, 0))  score = -b.score - 1000;
                                else                    score += captHistory[side][b.atkr][b.to][b.vctm] / 32;
                            }
                        }
                    }
                    else if constexpr (cap) {
                        int mvvLva = MVV_LVA[b.vctm][b.atkr];
                        if (mvvLva > 0) score = CAPTURE_BASE + mvvLva + captHistory[side][b.atkr][b.to][b.vctm] / 32;
                        else {
                            if (!see::seeGE(b, 0))  score = -b.score - 1000;
                            else                    score = CAPTURE_BASE + captHistory[side][b.atkr][b.to][b.vctm] / 32;
                        }
                    }
                    else {
                        if (ttMove && packMove(b.from, b.to) == ttMove) score = TT_MOVE_SCORE;
                        else {
                            const uint16_t pm = packMove(b.from, b.to);
                            if (pm == killers[ply][0])        score = KILLER_1;
                            else if (pm == killers[ply][1])   score = KILLER_2;
                            else {
                                score = -b.score
                                    + history[side][b.from][b.to]
                                    + continuationHistory[b.atkrPrev][b.toPrev][b.atkr][b.to] * depth / 8;

                                if (pm == counterMove[side][b.atkrPrev][b.toPrev]) score += COUNTER_MOVE_BONUS;
                            }
                        }
                    }
                }

                keys[size] = (uint64_t(uint32_t(score) ^ 0x80000000u) << 32) | uint32_t(size);
            }
            ++size;
        }

        __forceinline void reset() noexcept { 
            size = 0; 
        }

        __forceinline void init(bool useTT, uint16_t ttMove, uint16_t idMove, int depth, int ply) noexcept {
            size = 0;
            this->useTT = useTT;
            this->ttMove = ttMove;
            this->idMove = idMove;
            this->depth = depth;
            this->ply = ply;
        }

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