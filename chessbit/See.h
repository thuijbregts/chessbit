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

    ForceInline int seeGE(const BoardState& board, int threshold) {
        int swap = SEE_VALUE[board.vctm] - threshold;
        if (swap < 0) return false;
        swap = SEE_VALUE[board.atkr] - swap;
        if (swap <= 0) return true;

        const U64 pieceBB[2][6] = {
            { board.pE, board.nE, board.bE, board.rE, board.qE, board.kE },
            { board.pM, board.nM, board.bM, board.rM, board.qM, board.kM }
        };
        const U64 bq = board.bM | board.qM | board.bE | board.qE;
        const U64 rq = board.rM | board.qM | board.rE | board.qE;

        U64 fromBit;
        U64 occ = board.occB;
        U64 attackers = attackersTo(board, board.to, occ);

        int side = 0;
        bool res = true;

        do {
            side ^= 1;

            U64 subset = 0; int pc = p;
            for (; pc <= k; ++pc) { subset = attackers & pieceBB[side][pc]; if (subset) break; }
            if (!subset) break;
            res = !res;
            if (pc == k) {
                const U64 other = pieceBB[side ^ 1][p] | pieceBB[side ^ 1][n] | pieceBB[side ^ 1][b]
                    | pieceBB[side ^ 1][r] | pieceBB[side ^ 1][q] | pieceBB[side ^ 1][k];
                return (attackers & other) ? !res : res;
            }

            fromBit = _blsi_u64(subset);
    
            if ((swap = SEE_VALUE[pc] - swap) < res) break;

            attackers ^= fromBit;
            occ ^= fromBit;
            attackers |= ((getBishopAttacks(board.to, occ) & bq) | (getRookAttacks(board.to, occ) & rq)) & occ;

        } while (true);

        return res;
    }

    ForceInline void findAtkers(const BoardState& child, int(&atkrs)[2][16], int* atkrsSize, U64* bAtkrs, U64* rAtkrs, U64* qAtkrs, U64* kAtkrs, U64& occ, bool side) {
        int sq = child.to;
        U64 pAtks;

        U64 bb = pAtks = PAWN_CAPTURES[!side][sq] & child.pE;
        Bitloop(bb) { atkrs[side][atkrsSize[side]++] = p; }

        pAtks |= bb = PAWN_CAPTURES[side][sq] & child.pM;
        Bitloop(bb) { atkrs[!side][atkrsSize[!side]++] = p; }

        occ ^= pAtks;

        U64 nAtks = getKnightAttacks(sq);
        bb = nAtks & child.nE;
        Bitloop(bb) { atkrs[side][atkrsSize[side]++] = n; }

        bb = nAtks & child.nM;
        Bitloop(bb) { atkrs[!side][atkrsSize[!side]++] = n; }

        const U64 bq = child.bM | child.qM | child.bE | child.qE;
        const U64 rq = child.rM | child.qM | child.rE | child.qE;
        U64 bAtks = getBishopAttacks(sq, occ ^ bq);
        U64 rAtks = getRookAttacks(sq, occ ^ rq);

        bAtkrs[side] = bAtks & child.bE;
        bAtkrs[!side] = bAtks & child.bM;
        rAtkrs[side] = rAtks & child.rE;
        rAtkrs[!side] = rAtks & child.rM;
        qAtkrs[side] = (bAtks | rAtks) & child.qE;
        qAtkrs[!side] = (bAtks | rAtks) & child.qM;

        const U64 kAtks = getKingAttacks(sq);
        if (Bitcount(kAtks & (child.kE | child.kM)) == 1) [[unlikely]] {
            kAtkrs[side] = kAtks & child.kE;
            kAtkrs[!side] = kAtks & child.kM;
        }
    }

    ForceInline bool seeGE1(const BoardState& child, int threshold) {
        int atkrs[2][16]{};
        int atkrsSize[2]{};
        int atkrsIndex[2]{};

        U64 bAtkrs[2]{};
        U64 rAtkrs[2]{};
        U64 qAtkrs[2]{};
        U64 kAtkrs[2]{};

        bool side = !child.side;
        U64 occ = child.occB;

        findAtkers(child, atkrs, atkrsSize, bAtkrs, rAtkrs, qAtkrs, kAtkrs, occ, side);

        int atkr = child.atkr;

        int swap = SEE_VALUE[atkr] - (SEE_VALUE[child.vctm] - threshold);

        bool res = true;
        while (atkrsIndex[!side] < atkrsSize[!side] || (bAtkrs[!side] | rAtkrs[!side] | qAtkrs[!side] | kAtkrs[!side])) {
            if (atkr == k) return !res;

            side = !side;
            res = !res;

            atkr = k;
            if (atkrsIndex[side] < atkrsSize[side]) atkr = atkrs[side][atkrsIndex[side]++];
            else {
                U64 piece;
                if (bAtkrs[side] && (piece = _blsi_u64(getBishopAttacks(child.to, occ) & bAtkrs[side]))) {
                    atkr = b;
                    bAtkrs[side] ^= piece;
                    occ ^= piece;
                }
                else if (rAtkrs[side] && (piece = _blsi_u64(getRookAttacks(child.to, occ) & rAtkrs[side]))) {
                    atkr = r;
                    rAtkrs[side] ^= piece;
                    occ ^= piece;
                }
                else if (qAtkrs[side] && (piece = _blsi_u64(getQueenAttacks(child.to, occ) & qAtkrs[side]))) {
                    atkr = q;
                    qAtkrs[side] ^= piece;
                    occ ^= piece;
                }
            }

            if ((swap = SEE_VALUE[atkr] - swap) < res) break;
        }
        return res;
    }
}