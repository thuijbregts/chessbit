#pragma once

#include "MoveGenerator.h"
#include "Eval.h"
#include "Utils.h"
#include "See.h"
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

    inline int lmrTable[MAX_PLY][256];

    inline void initLmr() {
        for (int d = 1; d < MAX_PLY; ++d)
            for (int i = 1; i < 256; ++i)
                lmrTable[d][i] = int(0.75 + std::log(d) * std::log(i) * 0.5);
    }
    
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
        std::memset(continuationHistory, 0, sizeof(continuationHistory));
        std::memset(counterMove, 0, sizeof(counterMove));
    }

    template <bool side, bool first, uint8_t kMoved, bool useTT>
    ForceInline int search(int depth, const BoardState& board, int ply, int alpha, int beta, BoardState* bestMove = nullptr) {
        switch (depth) {
        case 20: return Engine<20, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 19: return Engine<19, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 18: return Engine<18, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 17: return Engine<17, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 16: return Engine<16, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 15: return Engine<15, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 14: return Engine<14, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 13: return Engine<13, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 12: return Engine<12, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 11: return Engine<11, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 10: return Engine<10, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 9: return Engine<9, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 8: return Engine<8, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 7: return Engine<7, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 6: return Engine<6, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 5: return Engine<5, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 4: return Engine<4, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 3: return Engine<3, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 2: return Engine<2, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        case 1: return Engine<1, first, side, kMoved, useTT>::search(board, ply, alpha, beta, bestMove);
        default: return Engine<0, first, side, kMoved, useTT>::search(board, ply, alpha, beta);
        }
    }

    template <bool side, uint8_t kMoved>
    static int quiescence(const BoardState& board, int ply, int alpha, int beta) {
        stats.nodes++;

        if (ply >= MAX_PLY - 1) { stats.leaves++; return board.score; }

        constexpr uint8_t kMovedK = kMoved | KING_MOVED[side];

        int standPat = -INF;
        int best = -INF;

        Batch& batch = batches[ply];
        batch.size = 0;

        if (!board.checks) {
            standPat = board.score;
            best = standPat;

            if (standPat >= beta) { stats.leaves++; return standPat; }
            if (standPat > alpha) alpha = standPat;

            if (standPat + see::SEE_VALUE[q] + DELTA_MARGIN < alpha) {
                stats.leaves++;
                return alpha;
            }

            movegen::generate<0, false, side, kMoved, false, true>(board, &batch);
        }
        else movegen::generate<0, false, side, kMoved, false>(board, &batch);

        stats.generated += batch.size;

        if (batch.size == 0) return board.checks ? -MATE + ply : standPat;

        const int futilityBase = standPat + DELTA_MARGIN;
        int score;

        for (int i = 0; i < batch.size; ++i) {
            batch.pick(i);
            BoardState& m = batch[i];

            if (!board.checks) {
                if (futilityBase + see::SEE_VALUE[m.vctm] <= alpha)
                    continue;

                if (!see::seeGE(m, SEE_MARGIN))
                    continue;
            }

            if (m.king) score = -quiescence<!side, kMovedK>(m, ply + 1, -beta, -alpha);
            else        score = -quiescence<!side, kMoved>(m, ply + 1, -beta, -alpha);

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

        if (ply >= MAX_PLY - 1) return board.score;

        alpha = std::max(alpha, -MATE + ply);
        beta = std::min(beta, MATE - ply - 1);
        if (alpha >= beta) return alpha;

        stats.nodes++;

        constexpr bool doTT = useTT && tt::USE_HASH<depth>;
        constexpr int nullDepth = depth - NULL_REDUCTION - 1;

        const int   alphaOrig = alpha;
        tt::Bucket* ttBucket = nullptr;
        uint16_t    ttMove = 0;

        if constexpr (doTT) {
            const Zobrist z = board.zobrist;
            ttBucket = &tt::bucket<depth>(z);
            tt::TTData e;
            if (tt::probe(*ttBucket, z, e)) {
                ttMove = e.move;
                if (e.depth >= depth) {
                    const int s = valueFromTT(e.score, ply);
                    if (e.bound == tt::BOUND_EXACT || (e.bound == tt::BOUND_LOWER && s >= beta) || (e.bound == tt::BOUND_UPPER && s <= alpha))
                        return s;
                }
            }
        }

        //Null move pruning
        if constexpr (!null && !first && nullDepth >= 0) {
            if (!board.checks && board.score >= beta) {
                // !zugzwang && enoughMaterial && !pvNode
                const BoardState nullBoard = board.makeNull<side>(board);

                int score;
                if constexpr (nullDepth == 0)   score = -quiescence<!side, kMoved>(nullBoard, ply + 1, -beta, -beta + 1);
                else                            score = -Engine<nullDepth, false, !side, kMoved, useTT, true>::search(nullBoard, ply + 1, -beta, -beta + 1);

                if (score >= beta) return score >= MATE_IN_MAX ? beta : score;
            }
        }

        Batch& batch = batches[ply];
        batch.size = 0;
        batch.ttMove = ttMove;
        if constexpr (first) {
            if (bestMove) batch.idMove = packMove(bestMove->from, bestMove->to);
        }

        movegen::generate<depth, false, side, kMoved, useTT>(board, &batch);

        stats.generated += batch.size;

        if (batch.size == 0) return board.checks ? -MATE + ply : 0;

        BoardState* bestMoveTT = nullptr;
        int best = -INF, searched = 0, score;

        for (int i = 0; i < batch.size; ++i) {
            batch.pick(i);

            BoardState& m = batch[i];

            const bool quiet = !m.cap && !m.promo;
            //quiet SEE pruning
            if constexpr (depth <= SEE_PRUNING_MAX_DEPTH && !first) {
                if (quiet && i > 0 && best > -MATE_IN_MAX && !(board.checks | m.checks) && !see::seeGE(m, SEE_MARGIN_QUIET)) continue;
            }

            if (!first && m.checks && !board.checks) {
                if (m.king) score = -search<!side, false, kMovedK, useTT>(depth, m, ply + 1, -beta, -alpha);
                else        score = -search<!side, false, kMoved, useTT>(depth, m, ply + 1, -beta, -alpha);
            }
            //LMR
            else if (depth >= 2 && i > 0 && !first) {
                int reduction = 0;
                bool canReduce = quiet
                    && !(board.checks | m.checks)
                    && i >= 3
                    && packMove(m.from, m.to) != ttMove
                    && packMove(m.from, m.to) != killers[depth][0]
                    && packMove(m.from, m.to) != killers[depth][1];
                if (canReduce) {
                    bool pvNode = beta - alpha > 1;
                    reduction = lmrTable[depth][std::min(i, 255)];

                    if (!pvNode)              reduction++;
                    if (history[side][m.from][m.to] < 0) reduction++;
                    reduction = std::clamp(reduction, 0, depth - 1);
                }

                const int reducedDepth = depth - 1 - reduction;

                if (m.king) score = -search<!side, false, kMovedK, useTT>(reducedDepth, m, ply + 1, -alpha - 1, -alpha);
                else        score = -search<!side, false, kMoved, useTT>(reducedDepth, m, ply + 1, -alpha - 1, -alpha);

                if (score > alpha && reduction > 0) {
                    if (m.king) score = -search<!side, false, kMovedK, useTT>(depth - 1, m, ply + 1, -alpha - 1, -alpha);
                    else        score = -search<!side, false, kMoved, useTT>(depth - 1, m, ply + 1, -alpha - 1, -alpha);
                }

                if (score > alpha && score < beta) {
                    if (m.king) score = -Engine<depth - 1, false, !side, kMovedK, useTT>::search(m, ply + 1, -beta, -alpha);
                    else        score = -Engine<depth - 1, false, !side, kMoved, useTT>::search(m, ply + 1, -beta, -alpha);
                }
            }
            else {
                if (m.king) score = -Engine<depth - 1, false, !side, kMovedK, useTT>::search(m, ply + 1, -beta, -alpha);
                else        score = -Engine<depth - 1, false, !side, kMoved, useTT>::search(m, ply + 1, -beta, -alpha);
            }

            if (score > best) {
                best = score;
                if constexpr (doTT)  bestMoveTT = &m;
                if constexpr (first) *bestMove = m;
            }

            if constexpr (!first) {
                if (score >= beta) {
                    stats.betaCutoffs++;
                    if (searched == 0) stats.firstMoveCutoffs++;

                    const uint16_t pm = packMove(m.from, m.to);

                    counterMove[side][board.atkr][board.to] = pm;

                    if (quiet) {
                        if (pm != killers[depth][0]) {
                            killers[depth][1] = killers[depth][0];
                            killers[depth][0] = pm;
                        }
        
                        int& cth = continuationHistory[board.atkr][board.to][m.atkr][m.to];
                        cth += bonus - cth * bonus / HIST_MAX;

                        int& h = history[side][m.from][m.to];
                        h += bonus - h * bonus / HIST_MAX;
                    }
                    else {
                        int& ch = captHistory[side][m.atkr][m.to][m.vctm];
                        ch += bonus - ch * bonus / HIST_MAX;
                    }
                    break;
                }

                if (quiet) {
                    int& cth = continuationHistory[board.atkr][board.to][m.atkr][m.to];
                    cth += -bonus - cth * bonus / HIST_MAX;

                    int& hm = history[side][m.from][m.to];
                    hm += -bonus - hm * bonus / HIST_MAX;
                }
                else {
                    int& ch = captHistory[side][m.atkr][m.to][m.vctm];
                    ch += -bonus - ch * bonus / HIST_MAX;
                }
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

    template <bool useTT>
    ForceInline int search(int depth, const BoardState& board, int ply, int alpha, int beta, BoardState* bestMove) {
        const uint8_t kMoved = (!(board.casPerms & (wk | wq)) ? KING_MOVED[white] : 0)
            | (!(board.casPerms & (bk | bq)) ? KING_MOVED[black] : 0);

        if (board.side == white) {
            switch (kMoved) {
            case KING_MOVED[white]: return search<white, true, KING_MOVED[white], useTT>(depth, board, ply, alpha, beta, bestMove);
            case KING_MOVED[black]: return search<white, true, KING_MOVED[black], useTT>(depth, board, ply, alpha, beta, bestMove);
            case KING_MOVED[both]:  return search<white, true, KING_MOVED[both], useTT>(depth, board, ply, alpha, beta, bestMove);
            default:                return search<white, true, 0, useTT>(depth, board, ply, alpha, beta, bestMove);
            }
        }
        else {
            switch (kMoved) {
            case KING_MOVED[white]: return search<black, true, KING_MOVED[white], useTT>(depth, board, ply, alpha, beta, bestMove);
            case KING_MOVED[black]: return search<black, true, KING_MOVED[black], useTT>(depth, board, ply, alpha, beta, bestMove);
            case KING_MOVED[both]:  return search<black, true, KING_MOVED[both], useTT>(depth, board, ply, alpha, beta, bestMove);
            default:                return search<black, true, 0, useTT>(depth, board, ply, alpha, beta, bestMove);
            }
        }
    }

    template <bool useTT>
    ForceInline void start(int maxDepth, const BoardState& board, BoardState* bestMove) {
        using clock = std::chrono::steady_clock;

        initLmr();
        clearHeuristics();
        tt::GENERATION++;

        if (maxDepth >= MAX_PLY) maxDepth = MAX_PLY - 1;

        printf("depth   score  bestmove       nodes         generated     time      nps        EBF   cutoffs      1st-move\n");
        printf("---------------------------------------------------------------------------------------------------------------\n");
        uint64_t prevNodes = 0;
        for (int d = 1; d <= maxDepth; ++d) {
            stats.reset();

            const auto t0 = clock::now();
            const int score = search<useTT>(d, board, 0, -INF, INF, bestMove);
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