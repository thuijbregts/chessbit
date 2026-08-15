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

    struct SearchStats {
        uint64_t nodes;
        uint64_t leaves;
        uint64_t generated;
        uint64_t betaCutoffs;
        uint64_t firstMoveCutoffs;
        uint64_t iirFired;

        void reset() { nodes = leaves = generated = betaCutoffs = firstMoveCutoffs = iirFired = 0; }
    };

    inline Zobrist repHistory[101 + MAX_PLY];
    inline int repCount;

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

    ForceInline bool isRepetition(const BoardState& board, int ply) {
        int current = repCount + ply;

        for (int i = current - 2; i >= current - board.halfClock; i -= 2) {
            if (repHistory[i] == board.zobrist) return true;
        }

        return false;
    }

    template <bool side, uint8_t kMoved>
    static int quiescence(const BoardState& board, int ply, int alpha, int beta) {
        stats.nodes++;

        if (ply >= MAX_PLY - 1) { stats.leaves++; return board.score; }

        constexpr uint8_t kMovedK = kMoved | KING_MOVED[side];

        int standPat = -INF;
        int best = -INF;

        Batch& batch = batches[ply];
        batch.init(false, 0, 0, 0, ply);

        if (!board.checks) {
            standPat = board.score;
            best = standPat;

            if (standPat >= beta) { stats.leaves++; return standPat; }
            if (standPat > alpha) alpha = standPat;

            if (standPat + see::SEE_VALUE[q] + DELTA_MARGIN < alpha) {
                stats.leaves++;
                return alpha;
            }

            movegen::generate<false, side, kMoved, true>(board, &batch);
        }
        else movegen::generate<false, side, kMoved>(board, &batch);

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

    template <bool first, bool side, uint8_t kMoved, bool null = false>
    static int alphaBeta(int depth, const BoardState& board, int ply, int alpha, int beta, int extensions, BoardState* bestMove = nullptr) {
        if (ply >= MAX_PLY - 1) return board.score;

        if constexpr (!first) {
            repHistory[repCount + ply] = board.zobrist;

            if (board.halfClock >= 100 || isRepetition(board, ply)) return 0;
        }

        if (depth == 0) return quiescence<side, kMoved>(board, ply, alpha, beta);

        alpha = std::max(alpha, -MATE + ply);
        beta = std::min(beta, MATE - ply - 1);
        if (alpha >= beta) return alpha;

        stats.nodes++;

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

        //Null move pruning
        if constexpr (!null && !first) {
            int nullDepth = depth - NULL_REDUCTION - 1;
            bool pvNode = beta - alpha > 1;
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

        movegen::generate<false, side, kMoved>(board, &batch);

        stats.generated += batch.size;

        if (batch.size == 0) return board.checks ? -MATE + ply : 0;

        BoardState* bestMoveTT = nullptr;
        int best = -INF, searched = 0, score, bonus = depth * depth;;

        for (int i = 0; i < batch.size; ++i) {
            batch.pick(i);

            BoardState& m = batch[i];

            const bool quiet = !m.cap && !m.promo;
            //quiet SEE pruning
            if constexpr (!first) {
                if (depth <= SEE_PRUNING_MAX_DEPTH && quiet && i > 0 && best > -MATE_IN_MAX && !(board.checks | m.checks) && !see::seeGE(m, SEE_MARGIN_QUIET)) continue;
            }

            int extension = 0;
            if (!first && m.checks && !board.checks && extensions < MAX_EXTENSIONS) extension = 1;

            int newExtensions = extensions + extension;

            if (depth >= 2 && i > 0 && !first) {
                const uint16_t pm = packMove(m.from, m.to);
                int reduction = 0;
                bool canReduce = quiet && !(board.checks | m.checks) && i >= 3 && best > -MATE_IN_MAX
                    && pm != ttMove && pm != killers[ply][0] && pm != killers[ply][1];//&& pm != counterMove[side][board.atkr][board.to];
                if (canReduce) {
                    bool pvNode = beta - alpha > 1;
                    reduction = lmrTable[depth][std::min(i, 255)];

                    if (!pvNode)              reduction++;
                    if (history[side][m.from][m.to] < 0) reduction++;
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
            }

            if (score > alpha) alpha = score;
            else if constexpr (!first) {
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

            searched++;
        }

        if (doTT) {
            const uint8_t bound = (best >= beta) ? tt::BOUND_LOWER : (best > alphaOrig) ? tt::BOUND_EXACT : tt::BOUND_UPPER;
            int ttMove = packMove(bestMoveTT->from, bestMoveTT->to);
            tt::write(depth, *ttBucket, board.zobrist, valueToTT(best, ply), bound, ttMove, tt::GENERATION);
        }

        return best;
    }

    ForceInline int search(int depth, const BoardState& board, int ply, int alpha, int beta, BoardState* bestMove) {
        const uint8_t kMoved = (!(board.casPerms & (wk | wq)) ? KING_MOVED[white] : 0)
            | (!(board.casPerms & (bk | bq)) ? KING_MOVED[black] : 0);

        if (board.side == white) {
            switch (kMoved) {
            case KING_MOVED[white]: return alphaBeta<true, white, KING_MOVED[white]>(depth, board, ply, alpha, beta, 0, bestMove);
            case KING_MOVED[black]: return alphaBeta<true, white, KING_MOVED[black]>(depth, board, ply, alpha, beta, 0, bestMove);
            case KING_MOVED[both]:  return alphaBeta<true, white, KING_MOVED[both]>(depth, board, ply, alpha, beta, 0, bestMove);
            default:                return alphaBeta<true, white, 0>(depth, board, ply, alpha, beta, 0, bestMove);
            }
        }
        else {
            switch (kMoved) {
            case KING_MOVED[white]: return alphaBeta<true, black, KING_MOVED[white]>(depth, board, ply, alpha, beta, 0, bestMove);
            case KING_MOVED[black]: return alphaBeta<true, black, KING_MOVED[black]>(depth, board, ply, alpha, beta, 0, bestMove);
            case KING_MOVED[both]:  return alphaBeta<true, black, KING_MOVED[both]>(depth, board, ply, alpha, beta, 0, bestMove);
            default:                return alphaBeta<true, black, 0>(depth, board, ply, alpha, beta, 0, bestMove);
            }
        }
    }

    ForceInline void start(int maxDepth, const BoardState& board, BoardState* bestMove) {
        using clock = std::chrono::steady_clock;

        initLmr();
        clearHeuristics();
        tt::GENERATION++;

        repCount = 0;
        const int start = std::max(0, moveCount - board.halfClock);

        for (int i = start; i <= moveCount; ++i) {
            repHistory[repCount++] = game::movesPlayed[i].zobrist;
        }
        repCount--;

        if (maxDepth >= MAX_PLY) maxDepth = MAX_PLY - 1;

        int score, newScore, alpha, beta, delta;

        printf("depth   score  bestmove       nodes         generated     time      nps        EBF   iir          cutoffs      1st-move\n");
        printf("----------------------------------------------------------------------------------------------------------------------------\n");
        uint64_t prevNodes = 0;
        for (int d = 1; d <= maxDepth; ++d) {
            stats.reset();

            const auto t0 = clock::now();

            if (d >= 4) {
                delta = 25;
                alpha = score - delta;
                beta = score + delta;
                while (true) {
                    newScore = search(d, board, 0, alpha, beta, bestMove);

                    if (newScore <= alpha) {
                        beta = (alpha + beta) / 2;
                        delta *= 2;
                        alpha = newScore - delta;
                    }
                    else if (newScore >= beta) {
                        delta *= 2;
                        beta = newScore + delta;
                    }
                    else break;

                    if (delta > 500) { alpha = -INF; beta = INF; }
                }
                score = newScore;
            }
            else {
                score = search(d, board, 0, -INF, INF, bestMove);
            }

            const auto t1 = clock::now();

            const double sec = std::chrono::duration<double>(t1 - t0).count();
            const double nps = sec > 0.0 ? stats.nodes / sec : 0.0;
            const double ebf = prevNodes ? (double)stats.nodes / (double)prevNodes : 0.0;
            const double firstPct = stats.betaCutoffs ? 100.0 * (double)stats.firstMoveCutoffs / (double)stats.betaCutoffs : 0.0;

            printf("%5d  %+6d  %-8s  %12llu  %12llu  %8.3fs  %9.0f  %5.2f  %10llu  %10llu  %6.1f%%\n",
                d, score, utils::getMoveSimple(*bestMove).c_str(),
                stats.nodes, stats.generated, sec, nps, ebf, stats.iirFired, stats.betaCutoffs, firstPct);

            prevNodes = stats.nodes;

            if (score > MATE_IN_MAX || score < -MATE_IN_MAX) break;
        }
    }
}