#pragma once

#include "MoveGenerator.h"
#include "Eval.h"
#include "Utils.h"
#include "Nnue.h"
#include "See.h"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <utility>
#include <atomic>

namespace engine {

    using namespace defs;
    using namespace bstate;
    using namespace movegen;
    using namespace batch;
    using namespace nnue;

    struct SearchStats {
        uint64_t nodes;
        uint64_t leaves;
        uint64_t generated;
        uint64_t betaCutoffs;
        uint64_t firstMoveCutoffs;
        uint64_t iirFired;

        void reset() { nodes = leaves = generated = betaCutoffs = firstMoveCutoffs = iirFired = 0; }
    };

    struct StopSearch {};
    inline std::atomic<bool> stopSearch{ false };
    inline std::chrono::steady_clock::time_point searchDeadline;
    inline bool useDeadline = false;

    ForceInline bool checkTime() {
        if (stopSearch.load(std::memory_order_relaxed)) return true;
        if (useDeadline && std::chrono::steady_clock::now() >= searchDeadline) {
            stopSearch.store(true, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    inline Zobrist repHistory[101 + MAX_PLY];
    inline int repCount;

    inline Batch batches[MAX_PLY];
    inline SearchStats stats;
    inline int lmrTable[MAX_PLY][256];

    ForceInline void initLmr() {
        for (int d = 1; d < MAX_PLY; ++d) {
            for (int i = 1; i < 256; ++i) {
                lmrTable[d][i] = int(0.75 + std::log(d) * std::log(i) * 0.5);
            }
        }
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

    ForceInline bool isRepetition(const BoardState& board, int ply) {
        int current = repCount + ply;
        int lower = std::max(0, current - board.halfClock);

        for (int i = current - 2; i >= lower; i -= 2) {
            if (repHistory[i] == board.zobrist) return true;
        }

        return false;
    }

    template <bool side, uint8_t kMoved>
    static int quiescence(const BoardState& board, int ply, int alpha, int beta) {
        stats.nodes++;
        if ((stats.nodes & 2047) == 0 && checkTime()) throw StopSearch{};

        if (ply >= MAX_PLY - 1) { stats.leaves++; return board.score; }

        constexpr uint8_t kMovedK = kMoved | KING_MOVED[side];

        int standPat = -INF;
        int best = -INF;

        Batch& batch = batches[ply];
        batch.init(false, 0, 0, 0, ply);

        if (!board.checks) {
            standPat = nnue::evaluate<side>(accumulators[ply]);
            best = standPat;

            if (standPat >= beta) { stats.leaves++; return standPat; }
            if (standPat > alpha) alpha = standPat;

            if (standPat + see::SEE_VALUE[q] + DELTA_MARGIN < alpha) {
                stats.leaves++;
                return alpha;
            }

            movegen::generate<false, side, kMoved, true>(board, ply, &batch);
        }
        else movegen::generate<false, side, kMoved>(board, ply, &batch);

        stats.generated += batch.size;

        if (batch.size == 0) return board.checks ? -MATE + ply : standPat;

        const int futilityBase = standPat + DELTA_MARGIN;
        int score;

        for (int i = 0; i < batch.size; ++i) {
            batch.pick(i);
            BoardState& m = batch[i];

            if (!board.checks) {
                //futility pruning
                if (futilityBase + see::SEE_VALUE[m.vctm] <= alpha) continue;

                //SEE pruning
                if (!see::seeGE(m, SEE_MARGIN)) continue;
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

    template <bool first, bool side, uint8_t kMoved, bool null = false>
    static int alphaBeta(int depth, const BoardState& board, int ply, int alpha, int beta, int extensions, BoardState* bestMove = nullptr) {
        if (ply >= MAX_PLY - 1) return board.score;

        if constexpr (!first) {
            if (isRepetition(board, ply)) return 0;

            if (board.halfClock >= 100) {
                if (!board.checks) return 0;

                return movegen::generate<true, side, kMoved>(board) ? 0 : (-MATE + ply);
            }

            repHistory[repCount + ply] = board.zobrist;
        }

        if (depth == 0) return quiescence<side, kMoved>(board, ply, alpha, beta);

        //mate distance pruning
        alpha = std::max(alpha, -MATE + ply);
        beta = std::min(beta, MATE - ply - 1);
        if (alpha >= beta) return alpha;

        stats.nodes++;
        if ((stats.nodes & 2047) == 0 && checkTime()) throw StopSearch{};

        constexpr uint8_t kMovedK = kMoved | KING_MOVED[side];
        bool doTT = ttEnabled && depth > 1;
        
        const int   alphaOrig = alpha;
        tt::Bucket* ttBucket = nullptr;
        uint16_t    ttMove = 0;

        if (doTT) {
            const Zobrist z = board.zobrist;
            ttBucket = &tt::bucket(z);
            tt::TTData e;
            if (tt::probe(*ttBucket, z, e)) {
                ttMove = e.move;

                if (e.depth >= depth) {
                    const int s = valueFromTT(e.score, ply);
                    if (e.bound == tt::BOUND_EXACT || (e.bound == tt::BOUND_LOWER && s >= beta) || (e.bound == tt::BOUND_UPPER && s <= alpha))
                        return s;
                }
            }
            else if (!null && depth >= IIR_MIN_DEPTH && !board.checks) {
                stats.iirFired++;
                depth--;
            }
        }

        bool pvNode = beta - alpha > 1;

        if constexpr (!first) {
            if (!pvNode && !board.checks) {
                //Reverse futility pruning
                if (depth <= RFP_MAX_DEPTH && beta < MATE_IN_MAX && board.score - RFP_MARGIN * depth >= beta) return board.score;

                //Razoring
                if (board.score + RAZOR_MARGIN + RAZOR_VAR * depth * depth < alpha) return quiescence<side, kMoved>(board, ply, alpha, beta);
            }
        }

        //Null move pruning
        if constexpr (!null && !first) {
            int nullDepth = depth - NULL_REDUCTION - 1;
            if (nullDepth >= 0 && !pvNode && !board.checks && board.score >= beta && BoardState::hasEnoughMaterial(board)) {
                const BoardState nullBoard = board.makeNull<side>(board);

                int score;
                if (nullDepth == 0) score = -quiescence<!side, kMoved>(nullBoard, ply + 1, -beta, -beta + 1);
                else                score = -alphaBeta<false, !side, kMoved, true>(nullDepth, nullBoard, ply + 1, -beta, -beta + 1, extensions);

                if (score >= beta) return score >= MATE_IN_MAX ? beta : score;
            }
        }

        uint16_t idMove = 0;
        if constexpr (first) {
            if (bestMove) idMove = packMove(bestMove->from, bestMove->to);
        }

        Batch& batch = batches[ply];
        batch.init(doTT, ttMove, idMove, depth, ply);

        movegen::generate<false, side, kMoved>(board, ply, &batch);

        stats.generated += batch.size;

        if (batch.size == 0) return board.checks ? -MATE + ply : 0;

        BoardState* bestMoveTT = nullptr;
        int best = -INF, searched = 0, score, bonus = depth * depth;

        for (int i = 0; i < batch.size; ++i) {
            batch.pick(i);

            BoardState& m = batch[i];

            const bool quiet = !m.cap && !m.promo;
            int& h = history[side][m.from][m.to];
            int& ch = captHistory[side][m.atkr][m.to][m.vctm];
            int& cth = continuationHistory[board.atkr][board.to][m.atkr][m.to];

            if constexpr (!first) {
                if (quiet && i > 0 && best > -MATE_IN_MAX && !(board.checks | m.checks)) {

                    //Late move pruning with history modulation
                    if (!pvNode && depth <= LMP_MAX_DEPTH) {
                        int lmp = LMP_LIMIT[depth] + std::clamp(h / HIST_DIVISOR, -LMP_HIST_CLAMP, LMP_HIST_CLAMP);

                        if (i >= lmp) continue;
                    }

                    //quiet SEE pruning
                    if (depth <= SEE_PRUNING_MAX_DEPTH && !see::seeGE(m, SEE_MARGIN_QUIET)) continue;
                } 
            }

            int extension = 0;
            if (!first && m.checks && !board.checks && extensions < MAX_EXTENSIONS) extension = 1;

            int newExtensions = extensions + extension;

            if (!first && depth >= 2 && i > 0) {
                const uint16_t pm = packMove(m.from, m.to);
                int reduction = 0;
                bool canReduce = quiet && !(board.checks | m.checks) && i >= 3 && best > -MATE_IN_MAX
                    && pm != ttMove && pm != killers[ply][0] && pm != killers[ply][1] && pm != counterMove[side][board.atkr][board.to];
                if (canReduce) {
                    reduction = lmrTable[depth][i];

                    if (!pvNode) reduction++;
                    if (h < 0)   reduction++;
                    reduction = std::clamp(reduction, 0, depth - 1);
                }

                const int reducedDepth = depth - 1 - reduction;

                if (m.king) score = -alphaBeta<false, !side, kMovedK>(reducedDepth, m, ply + 1, -alpha - 1, -alpha, newExtensions);
                else        score = -alphaBeta<false, !side, kMoved>(reducedDepth, m, ply + 1, -alpha - 1, -alpha, newExtensions);

                if (score > alpha && reduction > 0) {
                    if (m.king) score = -alphaBeta<false, !side, kMovedK>(depth - 1 + extension, m, ply + 1, -alpha - 1, -alpha, newExtensions);
                    else        score = -alphaBeta<false, !side, kMoved>(depth - 1 + extension, m, ply + 1, -alpha - 1, -alpha, newExtensions);
                }

                if (score > alpha && score < beta) {
                    if (m.king) score = -alphaBeta<false, !side, kMovedK>(depth - 1 + extension, m, ply + 1, -beta, -alpha, newExtensions);
                    else        score = -alphaBeta<false, !side, kMoved>(depth - 1 + extension, m, ply + 1, -beta, -alpha, newExtensions);
                }
            }
            else {
                if (m.king) score = -alphaBeta<false, !side, kMovedK>(depth - 1 + extension, m, ply + 1, -beta, -alpha, newExtensions);
                else        score = -alphaBeta<false, !side, kMoved>(depth - 1 + extension, m, ply + 1, -beta, -alpha, newExtensions);
            }

            if (score > best) {
                best = score;
                bestMoveTT = &m;
                if constexpr (first) *bestMove = m;
            }

            if constexpr (!first) {
                if (score >= beta) {
                    stats.betaCutoffs++;
                    if (searched == 0) stats.firstMoveCutoffs++;

                    const uint16_t pm = packMove(m.from, m.to);

                    counterMove[side][board.atkr][board.to] = pm;

                    if (quiet) {
                        if (pm != killers[ply][0]) {
                            killers[ply][1] = killers[ply][0];
                            killers[ply][0] = pm;
                        }

                        cth += bonus - cth * bonus / HIST_MAX;
                        h += bonus - h * bonus / HIST_MAX;
                    }
                    else {
                        ch += bonus - ch * bonus / HIST_MAX;
                    }
                    break;
                }
            }

            if (score > alpha) alpha = score;
            else if constexpr (!first) {
                if (quiet) {
                    cth += -bonus - cth * bonus / HIST_MAX;
                    h += -bonus - h * bonus / HIST_MAX;
                }
                else {
                    ch += -bonus - ch * bonus / HIST_MAX;
                }
            }

            searched++;
        }

        if (doTT) {
            const uint8_t bound = (best >= beta) ? tt::BOUND_LOWER : (best > alphaOrig) ? tt::BOUND_EXACT : tt::BOUND_UPPER;
            uint16_t ttMove = packMove(bestMoveTT->from, bestMoveTT->to);
            tt::write(depth, *ttBucket, board.zobrist, valueToTT(best, ply), bound, ttMove, tt::GENERATION);
        }

        return best;
    }

    template <bool side>
    ForceInline int search(int depth, const BoardState& board, int ply, int alpha, int beta, BoardState* bestMove) {
        const uint8_t kMoved = (!(board.casPerms & (wk | wq)) ? KING_MOVED[white] : 0)
            | (!(board.casPerms & (bk | bq)) ? KING_MOVED[black] : 0);

        switch (kMoved) {
        case KING_MOVED[white]: return alphaBeta<true, side, KING_MOVED[white]>(depth, board, ply, alpha, beta, 0, bestMove);
        case KING_MOVED[black]: return alphaBeta<true, side, KING_MOVED[black]>(depth, board, ply, alpha, beta, 0, bestMove);
        case KING_MOVED[both]:  return alphaBeta<true, side, KING_MOVED[both]>(depth, board, ply, alpha, beta, 0, bestMove);
        default:                return alphaBeta<true, side, 0>(depth, board, ply, alpha, beta, 0, bestMove);
        }
    }

    template <bool side, class SearchResult>
    BoardState runSearch(int maxDepth, const BoardState& board, long long budgetMs, SearchResult results) {
        using clock = std::chrono::steady_clock;

        nnue::init<side>(nnue::accumulators[0], board.pM, board.nM, board.bM, board.rM, board.qM, board.kMS, board.pE, board.nE, board.bE, board.rE, board.qE, board.kES);
        clearHeuristics();
        tt::GENERATION++;

        repCount = 0;
        const int s = std::max(0, moveCount - (int)board.halfClock);
        for (int i = s; i <= moveCount; ++i)
            repHistory[repCount++] = game::movesPlayed[i].zobrist;
        repCount--;

        if (maxDepth >= MAX_PLY) maxDepth = MAX_PLY - 1;

        stopSearch.store(false, std::memory_order_relaxed);
        useDeadline = (budgetMs > 0);
        if (useDeadline)
            searchDeadline = clock::now() + std::chrono::milliseconds(budgetMs);

        BoardState best{}, lastBest{};
        bool have = false;
        int score = 0, newScore, alpha, beta, delta;
        const auto t0 = clock::now();

        for (int d = 1; d <= maxDepth; ++d) {
            stats.reset();
            const auto iterT0 = clock::now();
            bool aborted = false;

            try {
                if (d >= 4) {
                    delta = 25;
                    alpha = score - delta;
                    beta = score + delta;
                    while (true) {
                        newScore = search<side>(d, board, 0, alpha, beta, &best);
                        if (newScore <= alpha) { beta = (alpha + beta) / 2; delta *= 2; alpha = newScore - delta; }
                        else if (newScore >= beta) { delta *= 2; beta = newScore + delta; }
                        else break;
                        if (delta > 500) { alpha = -INF; beta = INF; }
                    }
                    score = newScore;
                }
                else {
                    score = search<side>(d, board, 0, -INF, INF, &best);
                }
            }
            catch (const StopSearch&) {
                aborted = true;
            }

            if (aborted) break;

            lastBest = best;
            have = true;

            const auto now = clock::now();
            const double    iterSec = std::chrono::duration<double>(now - iterT0).count();
            const long long totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();

            results(d, score, lastBest, iterSec, totalMs);

            if (score > MATE_IN_MAX || score < -MATE_IN_MAX)  break;
            if (useDeadline && totalMs * 2 >= budgetMs)       break;
            if (stopSearch.load(std::memory_order_relaxed))   break;
        }

        if (!have) {
            useDeadline = false;
            stopSearch.store(false, std::memory_order_relaxed);
            search<side>(1, board, 0, -INF, INF, &lastBest);
        }

        return lastBest;
    }

    template <bool side>
    ForceInline void start(int maxDepth, const BoardState& board, BoardState* bestMove, long long budgetMs = 0) {
        printf("depth   score  bestmove       nodes         generated     time      nps        EBF   iir          cutoffs      1st-move\n");
        printf("----------------------------------------------------------------------------------------------------------------------------\n");

        uint64_t prevNodes = 0;

        BoardState result = runSearch<side>(maxDepth, board, budgetMs,
            [&prevNodes](int d, int score, const BoardState& best, double iterSec, long long /*totalMs*/) {
                const double nps = iterSec > 0.0 ? stats.nodes / iterSec : 0.0;
                const double ebf = prevNodes ? (double)stats.nodes / (double)prevNodes : 0.0;
                const double firstPct = stats.betaCutoffs ? 100.0 * (double)stats.firstMoveCutoffs / (double)stats.betaCutoffs : 0.0;

                printf("%5d  %+6d  %-8s  %12llu  %12llu  %8.3fs  %9.0f  %5.2f  %10llu  %10llu  %6.1f%%\n",
                    d, score, utils::getMoveSimple(best).c_str(),
                    stats.nodes, stats.generated, iterSec, nps, ebf, stats.iirFired, stats.betaCutoffs, firstPct);

                prevNodes = stats.nodes;
            });

        *bestMove = result;
    }
}