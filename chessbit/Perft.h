#pragma once

#include "MoveGenerator.h"

using namespace movegen;
using namespace tt;

namespace perft {
    template <bool side, uint8_t kMoved>
    ForceInline U64 iterateBatch(int depth, const BoardState& board) noexcept {
        constexpr uint8_t kMovedK = kMoved | KING_MOVED[side];

        U64 nodes = 0ULL;
        Batch batch;
        batch.perft = true;
        if (depth <= 1) return movegen::generate<true, side, kMoved>(board);

        movegen::generate<false, side, kMoved>(board, &batch);

        for (int i = 0; i < batch.size; ++i) {
            if (batch.moves[i].king)    nodes += iterateBatch<!side, kMovedK>(depth - 1, batch.moves[i]);
            else                        nodes += iterateBatch<!side, kMoved>(depth - 1, batch.moves[i]);
        }

        return nodes;
    }

    template <bool side, uint8_t kMoved>
    ForceInline U64 start(int depth, const BoardState& board, Batch* batch = nullptr) noexcept {
        if (depth <= 1) return movegen::generate<false, side, kMoved>(board, batch);

        return iterateBatch<side, kMoved>(depth, board);
    }
}