#pragma once

#include "Definitions.h"
#include "BoardState.h"
#include <algorithm>

using namespace defs;
using namespace bstate;

namespace see {
    constexpr int SEE_VALUE[7] = { 100, 320, 330, 500, 900, 50000, 0 };

    ForceInline U64 attackersTo(const BoardState& board, int sq, U64 occ) {
        U64 att = 0;
        att |= PAWN_CAPTURES[!board.side][sq] & board.pM;
        att |= PAWN_CAPTURES[board.side][sq] & board.pE;
        att |= getKnightAttacks(sq) & (board.nM | board.nE);
        att |= getKingAttacks(sq) & (board.kM | board.kE);
        const U64 bq = board.bM | board.qM | board.bE | board.qE;
        const U64 rq = board.rM | board.qM | board.rE | board.qE;
        att |= getBishopAttacks(sq, occ) & bq;
        att |= getRookAttacks(sq, occ) & rq;
        return att;
    }

    ForceInline int see(const BoardState& board) {
        const U64 pieceBB[2][6] = {
            { board.pE, board.nE, board.bE, board.rE, board.qE, board.kE },
            { board.pM, board.nM, board.bM, board.rM, board.qM, board.kM }
        };
        const U64 bq = board.bM | board.qM | board.bE | board.qE;
        const U64 rq = board.rM | board.qM | board.rE | board.qE;

        int gain[32];
        int d = 0;

        U64 fromBit = (1ULL << board.from);
        U64 occ = board.occB | fromBit;
        U64 attackers = fromBit | attackersTo(board, board.to, occ);

        gain[0] = SEE_VALUE[board.vctm];
        int aPiece = board.atkr;
        int side = 0;

        do {
            d++;
            gain[d] = SEE_VALUE[aPiece] - gain[d - 1];
            if (std::max(-gain[d - 1], gain[d]) < 0) break;

            attackers ^= fromBit;
            occ ^= fromBit;
            attackers |= ((getBishopAttacks(board.to, occ) & bq) | (getRookAttacks(board.to, occ) & rq)) & occ;

            side ^= 1;

            U64 subset = 0; int pc = p;
            for (; pc <= k; ++pc) { subset = attackers & pieceBB[side][pc]; if (subset) break; }
            if (!subset) break;

            if (pc == k) {
                const U64 other = pieceBB[side ^ 1][p] | pieceBB[side ^ 1][n] | pieceBB[side ^ 1][b]
                    | pieceBB[side ^ 1][r] | pieceBB[side ^ 1][q] | pieceBB[side ^ 1][k];
                if (attackers & other) break;
            }

            fromBit = _blsi_u64(subset);
            aPiece = pc;
        } while (true);

        while (--d) gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
        return gain[0];
    }
}