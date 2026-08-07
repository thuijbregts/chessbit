#pragma once

#include "MoveGenerator.h"
#include "Utils.h"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <utility>

namespace engine {

    using namespace defs;
    using namespace bstate;
    using namespace movegen;
    using namespace batch;

    constexpr int MAX_PLY = 64;
    constexpr int INF = 30000;
    constexpr int MATE = 29000;
    constexpr int MATE_IN_MAX = MATE - MAX_PLY;

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    struct Engine;

    struct SearchStats {
        uint64_t nodes;
        uint64_t leaves;
        uint64_t generated;
        uint64_t betaCutoffs;
        uint64_t firstMoveCutoffs;

        void reset() { nodes = leaves = generated = betaCutoffs = firstMoveCutoffs = 0; }
    };

    inline SearchStats stats;

    inline int evaluate(const BoardState& board) {
        constexpr int P = 100, N = 320, B = 330, R = 500, Q = 900;
        return P * (Bitcount(board.pM) - Bitcount(board.pE))
            + N * (Bitcount(board.nM) - Bitcount(board.nE))
            + B * (Bitcount(board.bM) - Bitcount(board.bE))
            + R * (Bitcount(board.rM) - Bitcount(board.rE))
            + Q * (Bitcount(board.qM) - Bitcount(board.qE));
    }

    constexpr int SEE_VALUE[7] = { 100, 320, 330, 500, 900, 20000, 0 };

    ForceInline int pieceOnM(const BoardState& board, U64 bit) {
        if (board.pM & bit) return p;
        if (board.nM & bit) return n;
        if (board.bM & bit) return b;
        if (board.rM & bit) return r;
        if (board.qM & bit) return q;
        return k;
    }
    ForceInline int pieceOnE(const BoardState& board, U64 bit) {
        if (board.pE & bit) return p;
        if (board.nE & bit) return n;
        if (board.bE & bit) return b;
        if (board.rE & bit) return r;
        if (board.qE & bit) return q;
        return k;
    }

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

    ForceInline int see(const BoardState& board, int from, int to) {
        const U64 pieceBB[2][6] = {
            { board.pM, board.nM, board.bM, board.rM, board.qM, board.kM },
            { board.pE, board.nE, board.bE, board.rE, board.qE, board.kE }
        };
        const U64 bq = board.bM | board.qM | board.bE | board.qE;
        const U64 rq = board.rM | board.qM | board.rE | board.qE;

        int gain[32];
        int d = 0;

        U64 fromBit = (1ULL << from);
        U64 occ = board.occB;
        U64 attackers = attackersTo(board, to, occ);

        gain[0] = SEE_VALUE[pieceOnE(board, (1ULL << to))];
        int aPiece = pieceOnM(board, fromBit);
        int side = 0;

        do {
            d++;
            gain[d] = SEE_VALUE[aPiece] - gain[d - 1];
            if (std::max(-gain[d - 1], gain[d]) < 0) break;

            attackers ^= fromBit;
            occ ^= fromBit;
            attackers |= ((getBishopAttacks(to, occ) & bq)
                | (getRookAttacks(to, occ) & rq)) & occ;

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

    ForceInline int scoreMove(const BoardState& board, const BoardState& m) {
        constexpr int CAPTURE_BASE = 1'000'000;
        if (m.promo)  return CAPTURE_BASE + SEE_VALUE[q];
        //if (m.type == EnPassant)  return CAPTURE_BASE + SEE_VALUE[p]; 
        if (m.cap)            return CAPTURE_BASE + see(board, m.from, m.to);
        return 0;
    }

    ForceInline int quiescence(const BoardState& board, int ply, int alpha, int beta) {
        stats.leaves++;
        return evaluate(board);
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    ForceInline int alphaBeta(const BoardState& board, int ply, int alpha, int beta) {
        stats.nodes++;

        Batch batch;
        movegen::generate<depth, false, side, kMoved, useTT>(board, &batch);

        int n = batch.size();

        stats.generated += n;

        if (n == 0) {
            if (board.checks) return -MATE + ply;
            return 0;
        }

        BoardState* moves = batch.moves();

        int sc[Batch::MAX];
        for (int i = 0; i < n; ++i) sc[i] = scoreMove(board, moves[i]);

        int best = -INF;
        for (int i = 0; i < n; ++i) {
            int bi = i;
            for (int j = i + 1; j < n; ++j) if (sc[j] > sc[bi]) bi = j;
            if (bi != i) { std::swap(moves[i], moves[bi]); std::swap(sc[i], sc[bi]); }

            const int score = -Engine<depth - 1, !side, kMoved, useTT>::search(moves[i], ply + 1, -beta, -alpha);

            if (score > best) best = score;

            if (score >= beta) {
                stats.betaCutoffs++;
                if (i == 0) stats.firstMoveCutoffs++;
                break;
            }

            if (score > alpha) alpha = score; 
        }
        return best;
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    ForceInline int search(const BoardState& board, BoardState& bestMove) {
        Batch batch;
        movegen::generate<depth, false, side, kMoved, useTT>(board, &batch);

        int n = batch.size();

        stats.generated += n;
        stats.nodes++;

        if (n == 0) return board.checks ? -MATE : 0;

        BoardState* moves = batch.moves();

        int sc[Batch::MAX];
        for (int i = 0; i < n; ++i) sc[i] = scoreMove(board, moves[i]);

        int alpha = -INF, best = -INF;
        for (int i = 0; i < n; ++i) {
            int bi = i;
            for (int j = i + 1; j < n; ++j) if (sc[j] > sc[bi]) bi = j;
            if (bi != i) { std::swap(moves[i], moves[bi]); std::swap(sc[i], sc[bi]); }

            const int score = -Engine<depth - 1, !side, kMoved, useTT>::search(moves[i], 1, -INF, -alpha);

            if (score > best) { best = score; bestMove = moves[i]; }
            if (score > alpha) alpha = score;
        }
        return best;
    }

    template <bool side, uint8_t kMoved, bool useTT>
    ForceInline int search(int depth, const BoardState& board, BoardState& bestMove) {
        switch (depth) {
        case 18: return search<18, side, kMoved, useTT>(board, bestMove);
        case 17: return search<17, side, kMoved, useTT>(board, bestMove);
        case 16: return search<16, side, kMoved, useTT>(board, bestMove);
        case 15: return search<15, side, kMoved, useTT>(board, bestMove);
        case 14: return search<14, side, kMoved, useTT>(board, bestMove);
        case 13: return search<13, side, kMoved, useTT>(board, bestMove);
        case 12: return search<12, side, kMoved, useTT>(board, bestMove);
        case 11: return search<11, side, kMoved, useTT>(board, bestMove);
        case 10: return search<10, side, kMoved, useTT>(board, bestMove);
        case 9: return search<9, side, kMoved, useTT>(board, bestMove);
        case 8: return search<8, side, kMoved, useTT>(board, bestMove);
        case 7: return search<7, side, kMoved, useTT>(board, bestMove);
        case 6: return search<6, side, kMoved, useTT>(board, bestMove);
        case 5: return search<5, side, kMoved, useTT>(board, bestMove);
        case 4: return search<4, side, kMoved, useTT>(board, bestMove);
        case 3: return search<3, side, kMoved, useTT>(board, bestMove);
        case 2: return search<2, side, kMoved, useTT>(board, bestMove);
        default: return search<1, side, kMoved, useTT>(board, bestMove);
        }
    }

    template <bool useTT>
    ForceInline int search(int depth, const BoardState& board, BoardState& bestMove) {
        const uint8_t kMoved = (!(board.casPerms & (wk | wq)) ? KING_MOVED[white] : 0)
            | (!(board.casPerms & (bk | bq)) ? KING_MOVED[black] : 0);

        if (board.side == white) {
            switch (kMoved) {
            case KING_MOVED[white]: return search<white, KING_MOVED[white], useTT>(depth, board, bestMove);
            case KING_MOVED[black]: return search<white, KING_MOVED[black], useTT>(depth, board, bestMove);
            case KING_MOVED[both]:  return search<white, KING_MOVED[both], useTT>(depth, board, bestMove);
            default:                return search<white, 0, useTT>(depth, board, bestMove);
            }
        }
        else {
            switch (kMoved) {
            case KING_MOVED[white]: return search<black, KING_MOVED[white], useTT>(depth, board, bestMove);
            case KING_MOVED[black]: return search<black, KING_MOVED[black], useTT>(depth, board, bestMove);
            case KING_MOVED[both]:  return search<black, KING_MOVED[both], useTT>(depth, board, bestMove);
            default:                return search<black, 0, useTT>(depth, board, bestMove);
            }
        }
    }

    ForceInline int search(int depth, const BoardState& board, BoardState& bestMove) {
        if (ttEnabled)	return search<true>(depth, board, bestMove);
        else			return search<false>(depth, board, bestMove);

    }

    ForceInline void start(int maxDepth, const BoardState& board, BoardState& bestMove) {
        using clock = std::chrono::steady_clock;

        if (maxDepth >= MAX_PLY) maxDepth = MAX_PLY - 1;

        printf("depth   score  bestmove       nodes         time      nps        EBF   cutoffs      1st-move\n");
        printf("---------------------------------------------------------------------------------------------\n");

        uint64_t prevNodes = 0;
        for (int d = 1; d <= maxDepth; ++d) {
            stats.reset();
            
            const auto t0 = clock::now();
            const int score = search(d, board, bestMove);
            const auto t1 = clock::now();

            const double sec = std::chrono::duration<double>(t1 - t0).count();
            const double nps = sec > 0.0 ? stats.nodes / sec : 0.0;
            const double ebf = prevNodes ? (double)stats.nodes / (double)prevNodes : 0.0;
            const double firstPct = stats.betaCutoffs
                ? 100.0 * (double)stats.firstMoveCutoffs / (double)stats.betaCutoffs : 0.0;

            printf("%5d  %+6d  %-8s  %12llu  %8.3fs  %9.0f  %5.2f  %10llu  %6.1f%%\n",
                d, score, utils::getMoveSimple(bestMove).c_str(),
                (unsigned long long)stats.nodes, sec, nps, ebf,
                (unsigned long long)stats.betaCutoffs, firstPct);

            prevNodes = stats.nodes;

            if (score > MATE_IN_MAX || score < -MATE_IN_MAX) break;
        }
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    struct Engine {
        static __declspec(noinline) int search(const BoardState& board, int ply, int alpha, int beta) {
            return alphaBeta<depth, side, kMoved, useTT>(board, ply, alpha, beta);
        }
    };

    template <bool side, uint8_t kMoved, bool useTT>
    struct Engine<0, side, kMoved, useTT> {
        ForceInline int search(const BoardState& board, int ply, int alpha, int beta) {
            return quiescence(board, ply, alpha, beta);
        }
    };
}