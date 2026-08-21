#pragma once

#include "Engine.h" 
#include "Game.h" 
#include <string>
#include <vector>
#include <fstream>
#include <random>
#include <cstdint>

namespace datagen {

    using namespace defs;
    using namespace bstate;

    struct Config {
        long long games = 1000;
        int  randomPlies = 8;
        int  depth = 15;
        int  maxPlies = 400;
        int  winAdjCp = 2000;
        int  winAdjPlies = 5;
        const char* outPath = "data.txt";
        uint64_t seed = 0xDA7A6E4;
    };

    struct Sample {
        std::string fen;
        int  score;
        bool stm;
    };

    ForceInline uint8_t kMovedOf(const BoardState& b) {
        return (!(b.casPerms & (wk | wq)) ? KING_MOVED[white] : 0)
            | (!(b.casPerms & (bk | bq)) ? KING_MOVED[black] : 0);
    }

    template <bool side>
    inline bool hasMoveS(const BoardState& b) {
        switch (kMovedOf(b)) {
        case KING_MOVED[white]: return movegen::generate<true, side, KING_MOVED[white]>(b);
        case KING_MOVED[black]: return movegen::generate<true, side, KING_MOVED[black]>(b);
        case KING_MOVED[both]:  return movegen::generate<true, side, KING_MOVED[both]>(b);
        default:                return movegen::generate<true, side, 0>(b);
        }
    }
    inline bool hasMove(const BoardState& b) {
        return b.side == white ? hasMoveS<white>(b) : hasMoveS<black>(b);
    }

    inline batch::Batch dgBatch;

    template <bool side, uint8_t kMoved>
    inline bool randChildKM(const BoardState& b, std::mt19937& rng, BoardState& out) {
        dgBatch.init(false, 0, 0, 0, 0);
        movegen::generate<false, side, kMoved>(b, &dgBatch);
        if (dgBatch.size == 0) return false;
        int r = std::uniform_int_distribution<int>(0, dgBatch.size - 1)(rng);
        out = dgBatch.moves[r];
        return true;
    }
    template <bool side>
    inline bool randChildS(const BoardState& b, std::mt19937& rng, BoardState& out) {
        switch (kMovedOf(b)) {
        case KING_MOVED[white]: return randChildKM<side, KING_MOVED[white]>(b, rng, out);
        case KING_MOVED[black]: return randChildKM<side, KING_MOVED[black]>(b, rng, out);
        case KING_MOVED[both]:  return randChildKM<side, KING_MOVED[both]>(b, rng, out);
        default:                return randChildKM<side, 0>(b, rng, out);
        }
    }
    inline bool randChild(const BoardState& b, std::mt19937& rng, BoardState& out) {
        return b.side == white ? randChildS<white>(b, rng, out)
            : randChildS<black>(b, rng, out);
    }

    inline BoardState labelSearch(const BoardState& b, int depth, int& scoreOut) {
        int sc = 0;
        auto cb = [&](int, int score, const BoardState&, double, long long) { sc = score; };
        BoardState child = (b.side == white)
            ? engine::runSearch<white>(depth, b, 0, cb)
            : engine::runSearch<black>(depth, b, 0, cb);
        scoreOut = sc;
        return child;
    }

    inline double playGame(BoardState board, std::mt19937& rng,
        const Config& cfg, std::vector<Sample>& samples) {
        for (int i = 0; i < cfg.randomPlies; ++i) {
            BoardState nxt;
            if (!randChild(board, rng, nxt)) return 0.5;
            board = nxt;
        }

        std::vector<Zobrist> hist;
        int   winCount = 0;
        int   winSign = 0;
        double whiteResult = 0.5;

        for (int ply = 0; ply < cfg.maxPlies; ++ply) {
            if (!hasMove(board)) {
                if (board.checks)
                    whiteResult = (board.side == white) ? 0.0 : 1.0;
                else
                    whiteResult = 0.5;
                break;
            }
            if (board.halfClock >= 100) { whiteResult = 0.5; break; }
            {
                int reps = 0;
                for (const Zobrist& z : hist) if (z == board.zobrist) ++reps;
                if (reps >= 2) { whiteResult = 0.5; break; }
            }
            hist.push_back(board.zobrist);

            int score;
            BoardState next = labelSearch(board, cfg.depth, score);

            const bool quietPos = !board.checks;
            const bool quietMove = !next.cap && !next.promo && !next.checks;
            if (quietPos && quietMove)
                samples.push_back({ game::getFen(board, ply / 2 + 1), score, board.side });

            int scoreW = (board.side == white) ? score : -score;
            int sign = (scoreW >= cfg.winAdjCp) ? 1 : (scoreW <= -cfg.winAdjCp ? -1 : 0);
            if (sign != 0 && sign == winSign) {
                if (++winCount >= cfg.winAdjPlies) { whiteResult = (sign > 0) ? 1.0 : 0.0; break; }
            }
            else { winSign = sign; winCount = (sign != 0) ? 1 : 0; }

            board = next;
        }

        return whiteResult;
    }

    inline void run(const Config& cfg, const BoardState& startpos) {
        std::mt19937 rng((uint32_t)cfg.seed);
        std::ofstream out(cfg.outPath, std::ios::out | std::ios::trunc);

        std::vector<Sample> samples;
        long long written = 0;

        ttEnabled = true;
        tt::init(64);

        for (long long g = 0; g < cfg.games; ++g) {
            samples.clear();
            tt::clear();

            const double wr = playGame(startpos, rng, cfg, samples);

            for (const Sample& s : samples) {
                const double res = (s.stm == white) ? wr : (1.0 - wr);
                out << s.fen << " | " << s.score << " | " << res << '\n';
                ++written;
            }

            if ((g & 1023) == 0) {
                out.flush();
                printf("parties=%lld  positions=%lld\n", g, written);
            }
        }
        out.flush();
        printf("FINI : %lld parties, %lld positions -> %s\n", cfg.games, written, cfg.outPath);
    }
}