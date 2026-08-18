#pragma once
//
// Datagen.h - generation de donnees d'entrainement NNUE par self-play.
//
// IDEE CLE : la datagen est AU-DESSUS de la recherche, pas dedans.
// On ne touche PAS a alphaBeta. Pour chaque coup de la partie :
//   1. runSearch(position) -> (score cote-trait, meilleur enfant)
//   2. si la position est "calme", on l'enregistre (fen, score, trait)
//   3. on avance sur l'enfant retourne, on recommence
//   4. en fin de partie, on connait le resultat -> on l'ecrit, oriente
//      selon le trait de CHAQUE position.
//
// Sortie : fichier texte, une ligne par position :  <FEN> | <score> | <result>
//   score  : centipions, POINT DE VUE DU CAMP AU TRAIT (positif = bon pour lui)
//   result : 1.0 / 0.5 / 0.0, POINT DE VUE DU CAMP AU TRAIT a cette position
// Ensuite : outil de bulletformat pour convertir ce .txt en binaire 32 octets.
//
// THREADS : l'etat de recherche est global (batches, stats, accumulators, tt,
// repHistory) -> NON thread-safe. Pour paralleliser : lance PLUSIEURS PROCESSUS,
// chacun ecrit son fichier, puis concatene + brasse avant d'entrainer.
//
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
        long long games = 10;   // nb de parties
        int  randomPlies = 8;        // coups d'ouverture aleatoires (diversite !)
        int  depth = 9;        // profondeur fixe du search de labelling
        int  maxPlies = 400;      // securite anti-partie-infinie
        int  winAdjCp = 2000;     // |score| >= ce seuil ...
        int  winAdjPlies = 5;        // ... pendant N plies consecutifs -> adjuge gagne
        const char* outPath = "data.txt";
        uint64_t seed = 0xDA7A6E4;
    };

    struct Sample {
        std::string fen;
        int  score;   // cote-trait
        bool stm;     // trait a cette position (white/black)
    };

    // ================================================================
    //  Glue moteur : dispatch side + kMoved, comme engine::search<side>
    // ================================================================
    ForceInline uint8_t kMovedOf(const BoardState& b) {
        return (!(b.casPerms & (wk | wq)) ? KING_MOVED[white] : 0)
            | (!(b.casPerms & (bk | bq)) ? KING_MOVED[black] : 0);
    }

    // -- existe-t-il au moins un coup legal ? --
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

    // -- un enfant legal choisi au hasard (pour l'ouverture aleatoire) --
    inline batch::Batch dgBatch;   // batch local a la datagen

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

    // -- search de labelling : score (cote-trait) + meilleur enfant --
    inline BoardState labelSearch(const BoardState& b, int depth, int& scoreOut) {
        int sc = 0;
        auto cb = [&](int, int score, const BoardState&, double, long long) { sc = score; };
        BoardState child = (b.side == white)
            ? engine::runSearch<white>(depth, b, 0, cb)
            : engine::runSearch<black>(depth, b, 0, cb);
        scoreOut = sc;
        return child;   // position apres le meilleur coup
    }

    // ================================================================
    //  Une partie : remplit 'samples', renvoie le resultat cote BLANC
    //  (1.0 blanc gagne, 0.5 nulle, 0.0 noir gagne).
    // ================================================================
    inline double playGame(BoardState board, std::mt19937& rng,
        const Config& cfg, std::vector<Sample>& samples) {
        // --- ouverture aleatoire (non enregistree) ---
        for (int i = 0; i < cfg.randomPlies; ++i) {
            BoardState nxt;
            if (!randChild(board, rng, nxt)) return 0.5;  // terminal precoce -> nulle
            board = nxt;
        }

        std::vector<Zobrist> hist;   // pour la triple repetition
        int   winCount = 0;          // plies consecutifs |scoreW| >= seuil
        int   winSign = 0;          // +1 avantage blanc, -1 avantage noir
        double whiteResult = 0.5;

        for (int ply = 0; ply < cfg.maxPlies; ++ply) {
            // --- conditions de fin AVANT de chercher ---
            if (!hasMove(board)) {
                if (board.checks)  // mat : le camp au trait est mat -> il perd
                    whiteResult = (board.side == white) ? 0.0 : 1.0;
                else               // pat
                    whiteResult = 0.5;
                break;
            }
            if (board.halfClock >= 100) { whiteResult = 0.5; break; }   // 50 coups
            {   // triple repetition
                int reps = 0;
                for (const Zobrist& z : hist) if (z == board.zobrist) ++reps;
                if (reps >= 2) { whiteResult = 0.5; break; }  // 2 occurrences passees + actuelle = 3
            }
            hist.push_back(board.zobrist);

            // --- labelling ---
            int score;
            BoardState next = labelSearch(board, cfg.depth, score);

            // --- filtre : on n'enregistre QUE les positions calmes ---
            // (pas en echec, et meilleur coup ni capture, ni promo, ni echec donne)
            const bool quietPos = !board.checks;
            const bool quietMove = !next.cap && !next.promo && !next.checks;
            if (quietPos && quietMove)
                samples.push_back({ game::getFen(board, ply / 2 + 1), score, board.side });

            // --- adjudication de gain (accelere enormement la datagen) ---
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

    // ================================================================
    //  Boucle principale : N parties -> fichier texte.
    // ================================================================
    inline void run(const Config& cfg, const BoardState& startpos) {
        std::mt19937 rng((uint32_t)cfg.seed);
        std::ofstream out(cfg.outPath, std::ios::out | std::ios::trunc);

        std::vector<Sample> samples;
        long long written = 0;

        for (long long g = 0; g < cfg.games; ++g) {
            samples.clear();
            const double wr = playGame(startpos, rng, cfg, samples);

            for (const Sample& s : samples) {
                // resultat ORIENTE selon le trait de la position :
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