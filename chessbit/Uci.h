#pragma once

#include "Engine.h"
#include "Game.h"
#include "TranspositionTable.h"

#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace uci {

    using namespace defs;
    using namespace bstate;
    using clock = std::chrono::steady_clock;

    inline const char* ENGINE_NAME = "chessbit";
    inline const char* ENGINE_AUTHOR = "Thomas Huijbregts";

    inline int  currentHashMb = 0;
    inline const int DEFAULT_HASH_MB = 128;

    inline void ensureTT(int mb) {
        if (mb <= 0) mb = DEFAULT_HASH_MB;
        if (tt::TABLE != nullptr && mb == currentHashMb) return;
        if (tt::TABLE != nullptr) tt::free();
        tt::init((size_t)mb);
        ttEnabled = true;
        currentHashMb = mb;
    }

    inline batch::Batch genBatch;

    inline void genLegal(const BoardState& b) {
        genBatch.init(false, 0, 0, 0, 0);
        genBatch.sort = false;

        const uint8_t kMoved = (!(b.casPerms & (wk | wq)) ? KING_MOVED[white] : 0)
            | (!(b.casPerms & (bk | bq)) ? KING_MOVED[black] : 0);
        if (b.side == white) {
            switch (kMoved) {
            case KING_MOVED[white]: movegen::generate<false, white, KING_MOVED[white]>(b, &genBatch); break;
            case KING_MOVED[black]: movegen::generate<false, white, KING_MOVED[black]>(b, &genBatch); break;
            case KING_MOVED[both]:  movegen::generate<false, white, KING_MOVED[both]>(b, &genBatch);  break;
            default:                movegen::generate<false, white, 0>(b, &genBatch);                 break;
            }
        }
        else {
            switch (kMoved) {
            case KING_MOVED[white]: movegen::generate<false, black, KING_MOVED[white]>(b, &genBatch); break;
            case KING_MOVED[black]: movegen::generate<false, black, KING_MOVED[black]>(b, &genBatch); break;
            case KING_MOVED[both]:  movegen::generate<false, black, KING_MOVED[both]>(b, &genBatch);  break;
            default:                movegen::generate<false, black, 0>(b, &genBatch);                 break;
            }
        }
    }

    inline std::string toLower(std::string s) {
        for (char& c : s) c = (char)std::tolower((unsigned char)c);
        return s;
    }

    inline std::string uciMove(const BoardState& m) {
        return toLower(utils::getMoveSimple(m));
    }

    inline bool applyMove(const std::string& mv) {
        genLegal(game::board);
        for (int i = 0; i < genBatch.size; ++i) {
            if (uciMove(genBatch.moves[i]) == mv) {
                game::makeMove(genBatch.moves[i]);
                return true;
            }
        }
        return false;
    }

    inline void cmdPosition(const std::vector<std::string>& tok) {
        size_t i = 1;
        if (i < tok.size() && tok[i] == "startpos") {
            game::setFen(StartPosition);
            ++i;
        }
        else if (i < tok.size() && tok[i] == "fen") {
            ++i;
            std::string fen;
            while (i < tok.size() && tok[i] != "moves") { fen += tok[i]; fen += ' '; ++i; }
            game::setFen(fen.c_str());
        }
        if (i < tok.size() && tok[i] == "moves") {
            ++i;
            for (; i < tok.size(); ++i) {
                if (!applyMove(tok[i])) break;
            }
        }
    }

    inline void emitInfo(int depth, int score, uint64_t nodes, long long ms, const BoardState& bm) {
        long long nps = ms > 0 ? (long long)(nodes * 1000ull / (uint64_t)ms) : 0;
        std::cout << "info depth " << depth;
        if (std::abs(score) > MATE_IN_MAX) {
            int plies = MATE - std::abs(score);
            int mvs = (plies + 1) / 2;
            if (score < 0) mvs = -mvs;
            std::cout << " score mate " << mvs;
        }
        else {
            std::cout << " score cp " << score;
        }
        std::cout << " nodes " << nodes
            << " nps " << nps
            << " time " << ms
            << " pv " << uciMove(bm)
            << "\n";
        std::cout.flush();
    }

    struct Limits {
        int  depth = MAX_PLY - 1;
        long long movetime = 0;
        long long wtime = 0, btime = 0, winc = 0, binc = 0;
        int  movestogo = 0;
        bool infinite = false;
        bool haveClock = false;
    };

    inline long long computeBudget(const Limits& lim) {
        if (lim.infinite)     return 0;
        if (lim.movetime > 0) return lim.movetime;
        if (!lim.haveClock)   return 0;

        long long myTime = (game::board.side == white) ? lim.wtime : lim.btime;
        long long myInc = (game::board.side == white) ? lim.winc : lim.binc;
        long long mtg = lim.movestogo > 0 ? lim.movestogo : 30;

        long long budget = myTime / mtg + myInc / 2;
        budget -= 10;
        long long cap = myTime - 20;
        if (budget > cap)  budget = cap;
        if (budget < 5)    budget = 5;
        return budget;
    }

    inline void cmdGo(const std::vector<std::string>& tok) {
        Limits lim;
        for (size_t i = 1; i < tok.size(); ++i) {
            const std::string& k = tok[i];
            auto next = [&](long long def) -> long long {
                return (i + 1 < tok.size()) ? std::stoll(tok[++i]) : def;
                };
            if (k == "depth")     lim.depth = (int)next(lim.depth);
            else if (k == "movetime")  lim.movetime = next(0);
            else if (k == "wtime") { lim.wtime = next(0); lim.haveClock = true; }
            else if (k == "btime") { lim.btime = next(0); lim.haveClock = true; }
            else if (k == "winc")      lim.winc = next(0);
            else if (k == "binc")      lim.binc = next(0);
            else if (k == "movestogo") lim.movestogo = (int)next(0);
            else if (k == "infinite")  lim.infinite = true;
        }
        if (lim.depth > MAX_PLY - 1) lim.depth = MAX_PLY - 1;

        genLegal(game::board);
        if (genBatch.size == 0) {
            std::cout << "bestmove 0000\n";
            std::cout.flush();
            return;
        }

        engine::initLmr();
        engine::clearHeuristics();
        tt::GENERATION++;

        engine::repCount = 0;
        const int histStart = std::max(0, game::moveCount - (int)game::board.halfClock);
        for (int i = histStart; i <= game::moveCount; ++i)
            engine::repHistory[engine::repCount++] = game::movesPlayed[i].zobrist;
        engine::repCount--;

        const long long budget = computeBudget(lim);
        engine::stopSearch.store(false, std::memory_order_relaxed);
        engine::useDeadline = (budget > 0);
        if (engine::useDeadline)
            engine::searchDeadline = clock::now() + std::chrono::milliseconds(budget);

        BoardState best;
        BoardState lastBest;
        bool have = false;
        uint64_t totalNodes = 0;
        int score = 0, newScore, alpha, beta, delta;

        const auto t0 = clock::now();

        for (int d = 1; d <= lim.depth; ++d) {
            engine::stats.reset();
            bool aborted = false;

            try {
                if (d >= 4) {
                    delta = 25;
                    alpha = score - delta;
                    beta = score + delta;
                    while (true) {
                        newScore = engine::search(d, game::board, 0, alpha, beta, &best);
                        if (newScore <= alpha) { beta = (alpha + beta) / 2; delta *= 2; alpha = newScore - delta; }
                        else if (newScore >= beta) { delta *= 2; beta = newScore + delta; }
                        else break;
                        if (delta > 500) { alpha = -INF; beta = INF; }
                    }
                    score = newScore;
                }
                else {
                    score = engine::search(d, game::board, 0, -INF, INF, &best);
                }
            }
            catch (const engine::StopSearch&) {
                aborted = true;
            }

            totalNodes += engine::stats.nodes;
            if (aborted) break;

            lastBest = best;
            have = true;

            const long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - t0).count();
            emitInfo(d, score, totalNodes, ms, lastBest);

            if (std::abs(score) > MATE_IN_MAX) break;
            if (engine::useDeadline && ms * 2 >= budget) break;
            if (engine::stopSearch.load(std::memory_order_relaxed)) break;
        }

        if (!have) {
            engine::useDeadline = false;
            engine::stopSearch.store(false, std::memory_order_relaxed);
            engine::search(1, game::board, 0, -INF, INF, &lastBest);
        }

        std::cout << "bestmove " << uciMove(lastBest) << "\n";
        std::cout.flush();
    }

    inline void cmdSetOption(const std::vector<std::string>& tok) {
        std::string name, value;
        size_t i = 1;
        if (i < tok.size() && tok[i] == "name") ++i;
        while (i < tok.size() && tok[i] != "value") { if (!name.empty()) name += ' '; name += tok[i]; ++i; }
        if (i < tok.size() && tok[i] == "value") { ++i; if (i < tok.size()) value = tok[i]; }

        if (name == "Hash") {
            int mb = 0;
            try { mb = std::stoi(value); }
            catch (...) { mb = DEFAULT_HASH_MB; }
            ensureTT(mb);
        }
    }

    inline void sendId() {
        std::cout << "id name " << ENGINE_NAME << "\n";
        std::cout << "id author " << ENGINE_AUTHOR << "\n";
        std::cout << "option name Hash type spin default " << DEFAULT_HASH_MB
            << " min 1 max 4096\n";
        std::cout << "uciok\n";
        std::cout.flush();
    }

    inline void loop() {
        sendId();

        std::string line;
        while (std::getline(std::cin, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
                line.pop_back();

            std::vector<std::string> tok = utils::split(line, ' ');
            if (tok.empty()) continue;
            const std::string& c = tok[0];

            if (c == "uci")        sendId();
            else if (c == "isready") {
                ensureTT(currentHashMb ? currentHashMb : DEFAULT_HASH_MB);
                std::cout << "readyok\n"; std::cout.flush();
            }
            else if (c == "ucinewgame") {
                game::setFen(StartPosition);
                ensureTT(currentHashMb ? currentHashMb : DEFAULT_HASH_MB);
                if (tt::TABLE) tt::clear();
            }
            else if (c == "setoption")  cmdSetOption(tok);
            else if (c == "position")   cmdPosition(tok);
            else if (c == "go")         cmdGo(tok);
            else if (c == "stop")       engine::stopSearch.store(true, std::memory_order_relaxed);
            else if (c == "quit")       break;
        }
    }
}