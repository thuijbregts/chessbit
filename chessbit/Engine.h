#pragma once

#include "MoveGenerator.h"
#include "Eval.h"
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

    template <int depth, bool first, bool side, uint8_t kMoved, bool useTT, bool null = false>
    struct Engine;

    struct SearchStats {
        uint64_t nodes;
        uint64_t leaves;
        uint64_t generated;
        uint64_t betaCutoffs;
        uint64_t firstMoveCutoffs;

        void reset() { nodes = leaves = generated = betaCutoffs = firstMoveCutoffs = 0; }
    };

    inline Batch batches[MAX_PLY];

    inline SearchStats stats;
    
    ForceInline int valueToTT(int v, int ply) {
        if (v >= MATE_IN_MAX) return v + ply;
        if (v <= -MATE_IN_MAX) return v - ply;
        return v;
    }
    ForceInline int valueFromTT(int v, int ply) {
        if (v >= MATE_IN_MAX) return v - ply;
        if (v <= -MATE_IN_MAX) return v + ply;
        return v;
    }

    ForceInline void clearHeuristics() {
        std::memset(killers, 0, sizeof(killers));
        std::memset(history, 0, sizeof(history));
        std::memset(captHistory, 0, sizeof(captHistory));
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

    template <bool side, bool useTT>
    ForceInline void scoreMoves(int* sc, BoardState* moves, int size, int ply, int ttMove = 0) {
        const uint16_t k0 = killers[ply][0], k1 = killers[ply][1];
        for (int i = 0; i < size; ++i) {
            const BoardState& m = moves[i];
            if constexpr (useTT) {
                if (packMove(m.from, m.to) == ttMove) { sc[i] = TT_MOVE_SCORE; continue; }
            }
            if (m.cap || m.promo) {
                sc[i] = CAPTURE_BASE + (PIECE_VALUE[m.vctm] * 16 - PIECE_VALUE[m.atkr]) + (captHistory[side][m.atkr][m.to][m.vctm] / 32);
            }
            else {
                const uint16_t pm = packMove(m.from, m.to);
                if (pm == k0) sc[i] = KILLER_1;
                else if (pm == k1) sc[i] = KILLER_2;
                else               sc[i] = history[side][m.from][m.to];
            }
        }
    }

    /*ForceInline void pickMove(BoardState* moves, int* sc, int size, int i) {
        int best = i;
        for (int j = i + 1; j < size; ++j) {
            if (sc[j] > sc[best]) best = j;
        }
        if (best != i) {
            std::swap(moves[i], moves[best]);
            std::swap(sc[i], sc[best]);
        }
    }*/

    template <bool side, uint8_t kMoved>
    static int quiescence(const BoardState& board, int ply, int alpha, int beta) {
        stats.nodes++;

        if (ply >= MAX_PLY - 1) { stats.leaves++; return board.score; }

        constexpr uint8_t kMovedK = kMoved | KING_MOVED[side];

        const bool inCheck = board.checks;

        int standPat = -INF;
        int best = -INF;

        Batch& batch = batches[ply];
        batch.size = 0;

        if (!inCheck) {
            standPat = board.score;
            best = standPat;

            if (standPat >= beta) { stats.leaves++; return standPat; }
            if (standPat > alpha) alpha = standPat;

            if (standPat + SEE_VALUE[q] + DELTA_MARGIN < alpha) {
                stats.leaves++;
                return alpha;
            }

            movegen::generate<0, false, side, kMoved, false, true>(board, &batch);
        }
        else movegen::generate<0, false, side, kMoved, false>(board, &batch);

        stats.generated += batch.size;

        if (batch.size == 0) return inCheck ? -MATE + ply : standPat;

        //int sc[Batch::MAX];
        //scoreMoves<side, false>(sc, moves, batch.size, ply);

        const int futilityBase = standPat + DELTA_MARGIN;
        int score;

        for (int i = 0; i < batch.size; ++i) {
            //pickMove(moves, sc, batch.size, i);
            batch.pick(i);
            BoardState& m = batch[i];

            if (!inCheck) {
                if (futilityBase + SEE_VALUE[m.vctm] <= alpha)
                    continue;

                if (see(board, m.from, m.to) < SEE_MARGIN)
                    continue;
            }

            if (m.king) [[unlikely]] score = -quiescence<!side, kMovedK>(m, ply + 1, -beta, -alpha);
            else                     score = -quiescence<!side, kMoved>(m, ply + 1, -beta, -alpha);

            if (score > best) {
                best = score;
                if (score > alpha) {
                    alpha = score;
                    if (score >= beta) return best;
                }
            }
        }

        return best;
    }

    template <int depth, bool first, bool side, uint8_t kMoved, bool useTT, bool null = false>
    ForceInline int alphaBeta(const BoardState& board, int ply, int alpha, int beta, BoardState* bestMove = nullptr) {
        constexpr uint8_t kMovedK = kMoved | KING_MOVED[side];
        constexpr int bonus = depth * depth;

        stats.nodes++;

        constexpr bool doTT = useTT && tt::USE_HASH<depth> && !first;
        constexpr int nullDepth = depth - NULL_REDUCTION - 1;

        if constexpr (!null && nullDepth >= 0) {
            if (!board.checks && board.score >= beta) {
                // staticEval >= beta && !zugzwang && enoughMaterial && !pvNode
                const BoardState nullBoard = board.makeNull<side>(board);

                int score;
                if constexpr (nullDepth == 0)   score = -quiescence<!side, kMoved>(nullBoard, ply + 1, -beta, -beta + 1);
                else                            score = -Engine<nullDepth, false, !side, kMoved, useTT, true>::search(nullBoard, ply + 1, -beta, -beta + 1);

                if (score >= beta) return score;
            }
        }

        const int   alphaOrig = alpha;
        tt::Bucket* ttBucket = nullptr;
        uint16_t    ttMove = 0;

        if constexpr (doTT) {
            const Zobrist z = board.zobrist;
            ttBucket = &tt::bucket<depth>(z);
            tt::TTData e;
            if (tt::probe(*ttBucket, z, e)) {
                if (e.depth >= depth) {
                    ttMove = e.move;
                    const int s = valueFromTT(e.score, ply);
                    if (e.bound == tt::BOUND_EXACT || (e.bound == tt::BOUND_LOWER && s >= beta) || (e.bound == tt::BOUND_UPPER && s <= alpha))
                        return s;
                }
            }
        }

        Batch& batch = batches[ply];
        batch.size = 0;
        movegen::generate<depth, false, side, kMoved, useTT>(board, &batch);

        stats.generated += batch.size;

        if (batch.size == 0) return board.checks ? -MATE + ply : 0;

        //int sc[Batch::MAX];
        //scoreMoves<side, doTT>(sc, moves, batch.size, ply, ttMove);

        BoardState* bestMoveTT = nullptr;
        int best = -INF, searched = 0, score;

        for (int i = 0; i < batch.size; ++i) {
            //pickMove(moves, sc, batch.size, i);
            batch.pick(i);

            BoardState& m = batch[i];

            const bool quiet = !m.cap && !m.promo;

            if (m.king) [[unlikely]] score = -Engine<depth - 1, false, !side, kMovedK, useTT>::search(m, ply + 1, -beta, -alpha);
            else                     score = -Engine<depth - 1, false, !side, kMoved, useTT>::search(m, ply + 1, -beta, -alpha);

            if (score > best) {
                best = score;
                if constexpr (doTT)  bestMoveTT = &m;
                if constexpr (first) *bestMove = m;
            }

            if constexpr (!first) {
                if (score >= beta) {
                    stats.betaCutoffs++;
                    if (searched == 0) stats.firstMoveCutoffs++;

                    if (quiet) {
                        const uint16_t pm = packMove(m.from, m.to);
                        if (pm != killers[depth][0]) {
                            killers[depth][1] = killers[depth][0];
                            killers[depth][0] = pm;
                        }

                        int& h = history[side][m.from][m.to];
                        h += bonus - h * bonus / HIST_MAX;
                    }
                    else {

                        int& ch = captHistory[side][m.atkr][m.to][m.vctm];
                        ch += bonus - ch * bonus / HIST_MAX;
                    }
                    break;
                }
            }

            if (quiet) {
                int& hm = history[side][m.from][m.to];
                hm += -bonus - hm * bonus / HIST_MAX;
            }

            if (score > alpha) alpha = score;
            searched++;
        }

        if constexpr (doTT) {
            const uint8_t bound = (best >= beta) ? tt::BOUND_LOWER : (best > alphaOrig) ? tt::BOUND_EXACT : tt::BOUND_UPPER;
            int ttMove = 0;
            if (bestMoveTT) ttMove = packMove(bestMoveTT->from, bestMoveTT->to);
            tt::write<depth>(*ttBucket, board.zobrist, valueToTT(best, ply), bound, ttMove, tt::GENERATION);
        }

        return best;
    }

    template <bool side, uint8_t kMoved, bool useTT>
    static int search(int depth, const BoardState& board, BoardState* bestMove) {
        switch (depth) {
            /*case 18: return Engine<18, true, side, kMoved, useTT>(board, 0, -INF, INF, bestMove);
            case 17: return Engine<17, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
            case 16: return Engine<16, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
            case 15: return Engine<15, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
            case 14: return Engine<14, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
            case 13: return Engine<13, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);*/
        case 12: return Engine<12, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
        case 11: return Engine<11, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
        case 10: return Engine<10, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
        case 9: return Engine<9, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
        case 8: return Engine<8, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
        case 7: return Engine<7, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
        case 6: return Engine<6, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
        case 5: return Engine<5, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
        case 4: return Engine<4, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
        case 3: return Engine<3, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
        case 2: return Engine<2, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
        default: return Engine<1, true, side, kMoved, useTT>::search(board, 0, -INF, INF, bestMove);
        }
    }

    template <bool useTT>
    static int search(int depth, const BoardState& board, BoardState* bestMove) {
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

    template <bool useTT>
    ForceInline void start(int maxDepth, const BoardState& board, BoardState* bestMove) {
        using clock = std::chrono::steady_clock;

        clearHeuristics();
        tt::GENERATION++;

        if (maxDepth >= MAX_PLY) maxDepth = MAX_PLY - 1;

        printf("depth   score  bestmove       nodes         generated     time      nps        EBF   cutoffs      1st-move\n");
        printf("---------------------------------------------------------------------------------------------------------------\n");
        uint64_t prevNodes = 0;
        for (int d = 1; d <= maxDepth; ++d) {
            stats.reset();

            const auto t0 = clock::now();
            const int score = search<useTT>(d, board, bestMove);
            const auto t1 = clock::now();

            const double sec = std::chrono::duration<double>(t1 - t0).count();
            const double nps = sec > 0.0 ? stats.nodes / sec : 0.0;
            const double ebf = prevNodes ? (double)stats.nodes / (double)prevNodes : 0.0;
            const double firstPct = stats.betaCutoffs
                ? 100.0 * (double)stats.firstMoveCutoffs / (double)stats.betaCutoffs : 0.0;

            printf("%5d  %+6d  %-8s  %12llu  %12llu  %8.3fs  %9.0f  %5.2f  %10llu  %6.1f%%\n",
                d, score, utils::getMoveSimple(*bestMove).c_str(),
                (unsigned long long)stats.nodes, (unsigned long long)stats.generated,
                sec, nps, ebf,
                (unsigned long long)stats.betaCutoffs, firstPct);

            prevNodes = stats.nodes;

            if (score > MATE_IN_MAX || score < -MATE_IN_MAX) break;
        }
    }

    template <int depth, bool first, bool side, uint8_t kMoved, bool useTT, bool null>
    struct Engine {
        static __declspec(noinline) int search(const BoardState& board, int ply, int alpha, int beta, BoardState* bestMove = nullptr) {
            return alphaBeta<depth, first, side, kMoved, useTT, null>(board, ply, alpha, beta, bestMove);
        }
    };

    template <bool side, bool first, uint8_t kMoved, bool useTT, bool null>
    struct Engine<1, first, side, kMoved, useTT, null> {
        ForceInline int search(const BoardState& board, int ply, int alpha, int beta, BoardState* bestMove = nullptr) {
            return alphaBeta<1, first, side, kMoved, useTT, null>(board, ply, alpha, beta, bestMove);
        }
    };

    template <bool side, bool first, uint8_t kMoved, bool useTT>
    struct Engine<0, first, side, kMoved, useTT> {
        ForceInline int search(const BoardState& board, int ply, int alpha, int beta) {
            return quiescence<side, kMoved>(board, ply, alpha, beta);
        }
    };
}