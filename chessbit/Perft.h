#pragma once

#include "MoveGenerator.h"

using namespace movegen;
using namespace tt;

namespace perft {

    template <int depth, bool side, uint8_t kMoved>
    struct PerftGenerator;

    template <int depth, bool side, uint8_t kMoved>
    ForceInline U64 iterateBatch(const BoardState& board) noexcept {
        constexpr uint8_t kMovedK = kMoved | KING_MOVED[side];
        constexpr int nDepth = depth - 1;

        U64 nodes = 0ULL;

        Batch batch;
        movegen::generate<depth, false, side, kMoved, false, false>(board, &batch);

        for (int i = 0; i < batch.size; ++i) {
            /*if constexpr (useTT) {
                const Zobrist& z = batch.moves[i].zobrist;
                Bucket& b = tt::bucket<nDepth>(z);

                if (tt::probe<nDepth>(b, z, val)) nodes += val;
                else {
                    if (batch.moves[i].king)    val = PerftGenerator<nDepth, !side, kMovedK>::generate(batch.moves[i]);
                    else                        val = PerftGenerator<nDepth, !side, kMoved>::generate(batch.moves[i]);
                    tt::write<nDepth>(b, z, val);
                    nodes += val;
                }
            }
            else {
                if (batch.moves[i].king)    nodes += PerftGenerator<nDepth, !side, kMovedK>::generate(batch.moves[i]);
                else                        nodes += PerftGenerator<nDepth, !side, kMoved>::generate(batch.moves[i]);  
            }*/
            if (batch.moves[i].king)    nodes += PerftGenerator<nDepth, !side, kMovedK>::generate(batch.moves[i]);
            else                        nodes += PerftGenerator<nDepth, !side, kMoved>::generate(batch.moves[i]);
        }

        return nodes;
    }

    template <int depth, bool side, uint8_t kMoved>
    struct PerftGenerator {
        static __declspec(noinline) U64 generate(const BoardState& board) {
            return iterateBatch<depth, side, kMoved>(board);
        }
    };

    template <bool side, uint8_t kMoved>
    struct PerftGenerator<1, side, kMoved> {
        ForceInline U64 generate(const BoardState& board) {
            return movegen::generate<1, true, side, kMoved, false, false>(board);
        }
    };
}