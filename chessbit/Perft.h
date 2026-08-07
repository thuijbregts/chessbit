#pragma once

#include "MoveGenerator.h"

using namespace movegen;
using namespace tt;

namespace perft {

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    struct PerftGenerator;

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    ForceInline U64 iterateBatch(const BoardState& board) noexcept {
        constexpr uint8_t kMovedK = kMoved | KING_MOVED[side];
        constexpr int nDepth = depth - 1;

        U64 nodes = 0ULL;

        Batch batch;
        movegen::generate<depth, false, side, kMoved, useTT>(board, &batch);

        U64 val;
        for (int i = 0; i < batch.nSize; ++i) {
            if constexpr (useTT) {
                const Zobrist& z = batch.normal[i].zobrist;
                Bucket& b = tt::bucket<nDepth>(z);

                if (tt::probe<nDepth>(b, z, val)) nodes += val;
                else {
                    val = PerftGenerator<nDepth, !side, kMoved, useTT>::generate(batch.normal[i]);
                    tt::write<nDepth>(b, z, val);
                    nodes += val;
                }
            }
            else nodes += PerftGenerator<nDepth, !side, kMoved, useTT>::generate(batch.normal[i]);
        }

        for (int i = 0; i < batch.kSize; ++i) {
            if constexpr (useTT) {
                const Zobrist& z = batch.king[i].zobrist;
                Bucket& b = tt::bucket<nDepth>(z);

                if (tt::probe<nDepth>(b, z, val)) nodes += val;
                else {
                    val = PerftGenerator<nDepth, !side, kMovedK, useTT>::generate(batch.king[i]);
                    tt::write<nDepth>(b, z, val);
                    nodes += val;
                }
            }
            else nodes += PerftGenerator<nDepth, !side, kMovedK, useTT>::generate(batch.king[i]);
        }

        return nodes;
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    struct PerftGenerator {
        static __declspec(noinline) U64 generate(const BoardState& board) {
            return iterateBatch<depth, side, kMoved, useTT>(board);
        }
    };

    template <bool side, uint8_t kMoved, bool useTT>
    struct PerftGenerator<1, side, kMoved, useTT> {
        ForceInline U64 generate(const BoardState& board) {
            return movegen::generate<1, true, side, kMoved, false>(board);
        }
    };
}