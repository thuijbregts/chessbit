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

    constexpr int MAX_PLY = 64;
    constexpr int INF = 30000;
    constexpr int MATE = 29000;
    constexpr int MATE_IN_MAX = MATE - MAX_PLY;

    constexpr int TT_MOVE_SCORE = 2'000'000;
    constexpr int CAPTURE_BASE = 1'000'000;
    constexpr int KILLER_1 = 900'000;
    constexpr int KILLER_2 = 800'000;
    constexpr int HIST_MAX = 16'384;
    constexpr int DELTA_MARGIN = 200;
    constexpr int SEE_MARGIN = -50;
    constexpr int NULL_REDUCTION = 2;

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
    inline uint16_t killers[MAX_PLY][2];
    inline int      history[2][64][64];
    inline int captHistory[2][6][64][6];
 
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

    ForceInline uint16_t packMove(const BoardState& m) {
        return uint16_t(m.from) | (uint16_t(m.to) << 6);
    }

    ForceInline void clearHeuristics() {
        std::memset(killers, 0, sizeof(killers));
        std::memset(history, 0, sizeof(history));
        std::memset(captHistory, 0, sizeof(captHistory));
    }

    ForceInline int evaluate(const BoardState& board) {
        int mg = 0, eg = 0, phase = 0;

        U64 bb = board.pM;
        Bitloop(bb) { int s = SquareOf(bb); mg += VAL_MG[p] + PAWN_MG[s];   eg += VAL_EG[p] + PAWN_EG[s]; }
        bb = board.nM;
        Bitloop(bb) { int s = SquareOf(bb); mg += VAL_MG[n] + KNIGHT_MG[s]; eg += VAL_EG[n] + KNIGHT_EG[s]; phase += 1; }
        bb = board.bM;
        Bitloop(bb) { int s = SquareOf(bb); mg += VAL_MG[b] + BISHOP_MG[s]; eg += VAL_EG[b] + BISHOP_EG[s]; phase += 1; }
        bb = board.rM;
        Bitloop(bb) { int s = SquareOf(bb); mg += VAL_MG[r] + ROOK_MG[s];   eg += VAL_EG[r] + ROOK_EG[s];   phase += 2; }
        bb = board.qM;
        Bitloop(bb) { int s = SquareOf(bb); mg += VAL_MG[q] + QUEEN_MG[s];  eg += VAL_EG[q] + QUEEN_EG[s];  phase += 4; }
        mg += KING_MG[board.kMS];
        eg += KING_EG[board.kMS];

        bb = board.pE;
        Bitloop(bb) { int s = SquareOf(bb) ^ 56; mg -= VAL_MG[p] + PAWN_MG[s];   eg -= VAL_EG[p] + PAWN_EG[s]; }
        bb = board.nE;
        Bitloop(bb) { int s = SquareOf(bb) ^ 56; mg -= VAL_MG[n] + KNIGHT_MG[s]; eg -= VAL_EG[n] + KNIGHT_EG[s]; phase += 1; }
        bb = board.bE;
        Bitloop(bb) { int s = SquareOf(bb) ^ 56; mg -= VAL_MG[b] + BISHOP_MG[s]; eg -= VAL_EG[b] + BISHOP_EG[s]; phase += 1; }
        bb = board.rE;
        Bitloop(bb) { int s = SquareOf(bb) ^ 56; mg -= VAL_MG[r] + ROOK_MG[s];   eg -= VAL_EG[r] + ROOK_EG[s];   phase += 2; }
        bb = board.qE;
        Bitloop(bb) { int s = SquareOf(bb) ^ 56; mg -= VAL_MG[q] + QUEEN_MG[s];  eg -= VAL_EG[q] + QUEEN_EG[s];  phase += 4; }
        mg -= KING_MG[board.kES ^ 56];
        eg -= KING_EG[board.kES ^ 56];

        int mgPhase = phase > 24 ? 24 : phase;
        int egPhase = 24 - mgPhase;
        return (mg * mgPhase + eg * egPhase) / 24;
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
                if (packMove(m) == ttMove) { sc[i] = TT_MOVE_SCORE; continue; }
            }
            if (m.cap || m.promo) {
                sc[i] = CAPTURE_BASE + (PIECE_VALUE[m.vctm] * 16 - PIECE_VALUE[m.atkr]) + (captHistory[side][m.atkr][m.to][m.vctm] / 32);
            }
            else {
                const uint16_t pm = packMove(m);
                if (pm == k0) sc[i] = KILLER_1;
                else if (pm == k1) sc[i] = KILLER_2;
                else               sc[i] = history[side][m.from][m.to];
            }
        }
    }

    ForceInline void pickMove(BoardState* moves, int* sc, int size, int i) {
        int best = i;
        for (int j = i + 1; j < size; ++j) {
            if (sc[j] > sc[best]) best = j;
        }
        if (best != i) {
            std::swap(moves[i], moves[best]);
            std::swap(sc[i], sc[best]);
        }
    }

    template <bool side, uint8_t kMoved>
    static int quiescence(const BoardState& board, int ply, int alpha, int beta) {
        stats.nodes++;

        if (ply >= MAX_PLY - 1) { stats.leaves++; return evaluate(board); }

        constexpr uint8_t kMovedK = kMoved | KING_MOVED[side];

        const bool inCheck = board.checks;

        int standPat = -INF;
        int best = -INF;

        Batch& batch = batches[ply];
        batch.size = 0;

        if (!inCheck) {
            standPat = evaluate(board);
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

        BoardState* moves = batch.moves;

        int sc[Batch::MAX];
        scoreMoves<side, false>(sc, moves, batch.size, ply);

        const int futilityBase = standPat + DELTA_MARGIN;
        int score;

        for (int i = 0; i < batch.size; ++i) {
            pickMove(moves, sc, batch.size, i);
            BoardState& m = moves[i];

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
            if (!board.checks && evaluate(board) >= beta) {
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

        BoardState* moves = batch.moves;

        int sc[Batch::MAX];
        scoreMoves<side, doTT>(sc, moves, batch.size, ply, ttMove);

        BoardState* bestMoveTT = nullptr;
        int best = -INF, searched = 0, score;

        for (int i = 0; i < batch.size; ++i) {
            pickMove(moves, sc, batch.size, i);

            const bool quiet = !moves[i].cap && !moves[i].promo;

            if (moves[i].king) [[unlikely]] score = -Engine<depth - 1, false, !side, kMovedK, useTT>::search(moves[i], ply + 1, -beta, -alpha);
            else                            score = -Engine<depth - 1, false, !side, kMoved, useTT>::search(moves[i], ply + 1, -beta, -alpha);

            if (score > best) {
                best = score;
                if constexpr (doTT)  bestMoveTT = &moves[i];
                if constexpr (first) *bestMove = moves[i];
            }

            if constexpr (!first) {
                if (score >= beta) {
                    stats.betaCutoffs++;
                    if (searched == 0) stats.firstMoveCutoffs++;

                    if (quiet) {
                        const uint16_t pm = packMove(moves[i]);
                        if (pm != killers[ply][0]) {
                            killers[ply][1] = killers[ply][0];
                            killers[ply][0] = pm;
                        }

                        int& h = history[side][moves[i].from][moves[i].to];
                        h += bonus - h * bonus / HIST_MAX;
                    }
                    else {

                        int& ch = captHistory[side][moves[i].atkr][moves[i].to][moves[i].vctm];
                        ch += bonus - ch * bonus / HIST_MAX;
                    }
                    break;
                }
            }

            if (quiet) {
                int& hm = history[side][moves[i].from][moves[i].to];
                hm += -bonus - hm * bonus / HIST_MAX;
            }

            if (score > alpha) alpha = score;
            searched++;
        }

        if constexpr (doTT) {
            const uint8_t bound = (best >= beta) ? tt::BOUND_LOWER : (best > alphaOrig) ? tt::BOUND_EXACT : tt::BOUND_UPPER;
            int ttMove = 0;
            if (bestMoveTT) ttMove = packMove(*bestMoveTT);
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