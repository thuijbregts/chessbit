#pragma once
//
// NnueOracle.h — evaluation de reference INDEPENDANTE pour valider Nnue.h.
//
// Ne partage AUCUN code avec l'implementation optimisee :
//   - formule d'index HalfKA reecrite a la main (pas d'appel a halfKAIndex)
//   - accumulateurs reconstruits depuis une liste (couleur,type,case) explicite
//   - forward 100% scalaire (pas de mullo/madd, pas de pre-clamp, pas de SIMD)
//
// But : si la convention (miroir, relColor, roi) ou le forward diverge, l'oracle
// et evaluate() donnent des scores differents. S'ils concordent sur une variete
// de positions, ta NNUE est correcte de bout en bout (aux poids courants pres).
//
// Hypothese : identique a l'implem, |x*w| < 32768 dans L1 (pas d'overflow int16).
// Vrai a l'init aleatoire (|w|<=64, x<=255) et pour un net bien quantifie.
//
#include "Definitions.h"
#include "Nnue.h"
#include <cstdio>
#include <cstdlib>

namespace nnue_oracle {

    using namespace defs;
    using nnue::net;
    using nnue::L1; using nnue::L2; using nnue::L3;
    using nnue::COLORS; using nnue::PIECE_TYPES; using nnue::SQUARES;
    using nnue::QA; using nnue::QB; using nnue::SCALE;

    // ---- helpers scalaires reecrits (pas ceux de Nnue.h) ----
    inline int clampS(int v) { return v < 0 ? 0 : (v > QA ? QA : v); }

    inline long long screluHiddenS(long long pre) {
        long long t = pre / ((long long)QA * QB);
        long long x = t < 0 ? 0 : (t > QA ? QA : t);
        return x * x;
    }

    // ---- index HalfKA, ecrit independamment ----
    // persp, pc : couleurs (white/black). type : 0..5. sq : 0..63.
    // wKing/bKing : cases ABSOLUES des rois blanc et noir.
    inline int idxOracle(bool persp, bool pc, int type, int sq, int wKing, int bKing) {
        const int king = (persp == white) ? wKing : (bKing ^ 56);
        const int s = (persp == white) ? sq : (sq ^ 56);
        const int rel = (pc == persp) ? 0 : 1;   // 0 = a nous, 1 = adverse
        return ((king * COLORS + rel) * PIECE_TYPES + type) * SQUARES + s;
    }

    // ---- collecte des pieces du board en (couleur,type,case) ----
    struct PList {
        bool col[32];
        int  typ[32];
        int  sqr[32];
        int  n = 0;
        void push(bool c, int t, int s) { col[n] = c; typ[n] = t; sqr[n] = s; ++n; }
    };

    template <class Board>
    inline void collect(const Board& board, PList& pl) {
        const bool stmW = (board.side == white);
        // bitboards absolus
        const U64 wP = stmW ? board.pM : board.pE, bP = stmW ? board.pE : board.pM;
        const U64 wN = stmW ? board.nM : board.nE, bN = stmW ? board.nE : board.nM;
        const U64 wB = stmW ? board.bM : board.bE, bB = stmW ? board.bE : board.bM;
        const U64 wR = stmW ? board.rM : board.rE, bR = stmW ? board.rE : board.rM;
        const U64 wQ = stmW ? board.qM : board.qE, bQ = stmW ? board.qE : board.qM;
        const U64 wK = stmW ? board.kM : board.kE, bK = stmW ? board.kE : board.kM;

        auto scan = [&](U64 bb, bool c, int t) {
            while (bb) { int s = SquareOf(bb); bb &= (bb - 1); pl.push(c, t, s); }
            };
        scan(wP, white, int(Piece::Pawn));   scan(bP, black, int(Piece::Pawn));
        scan(wN, white, int(Piece::Knight)); scan(bN, black, int(Piece::Knight));
        scan(wB, white, int(Piece::Bishop)); scan(bB, black, int(Piece::Bishop));
        scan(wR, white, int(Piece::Rook));   scan(bR, black, int(Piece::Rook));
        scan(wQ, white, int(Piece::Queen));  scan(bQ, black, int(Piece::Queen));
        scan(wK, white, int(Piece::King));   scan(bK, black, int(Piece::King));
    }

    // ---- reconstruction d'une perspective (int16, comme l'implem) ----
    inline void buildAccum(int16_t* acc, bool persp, const PList& pl, int wKing, int bKing) {
        for (int i = 0; i < L1; ++i) acc[i] = net->ftBias[i];
        for (int p = 0; p < pl.n; ++p) {
            const int f = idxOracle(persp, pl.col[p], pl.typ[p], pl.sqr[p], wKing, bKing);
            const int16_t* w = net->ftW[f];
            for (int i = 0; i < L1; ++i) acc[i] += w[i];   // wrap int16, comme v[]
        }
    }

    // ---- eval de reference complete ----
    template <bool side, class Board>
    inline int oracleEval(const Board& board) {
        const int wKing = (board.side == white) ? board.kMS : board.kES;
        const int bKing = (board.side == white) ? board.kES : board.kMS;

        PList pl; collect(board, pl);

        int16_t accW[L1], accB[L1];
        buildAccum(accW, white, pl, wKing, bKing);
        buildAccum(accB, black, pl, wKing, bKing);

        const int16_t* accStm = (side == white) ? accW : accB;
        const int16_t* accNstm = (side == white) ? accB : accW;

        // L1 : SCReLU(x)=clamp(x,0,QA)^2, dot avec l1W[p][j][i]
        long long h[2 * L2];
        for (int j = 0; j < L2; ++j) {
            long long s0 = net->l1Bias[0][j];
            long long s1 = net->l1Bias[1][j];
            for (int i = 0; i < L1; ++i) {
                long long x0 = clampS(accStm[i]);
                long long x1 = clampS(accNstm[i]);
                s0 += x0 * x0 * net->l1W[0][j][i];
                s1 += x1 * x1 * net->l1W[1][j][i];
            }
            h[j] = screluHiddenS(s0);
            h[L2 + j] = screluHiddenS(s1);
        }

        // L2 : layout transpose [L3][2*L2]
        long long h2[L3];
        for (int j = 0; j < L3; ++j) {
            long long s = net->l2Bias[j];
            for (int i = 0; i < 2 * L2; ++i)
                s += h[i] * net->l2W[j][i];
            h2[j] = screluHiddenS(s);
        }

        // sortie
        long long out = net->outBias;
        for (int i = 0; i < L3; ++i)
            out += h2[i] * net->outW[i];

        return (int)((out / ((long long)QA * QB)) * SCALE / QA);
    }

    // ================================================================
    //  Harnais 1 : verifie une position isolee (refresh + forward).
    //  Force un recalcul propre de accumulators[0], puis compare.
    // ================================================================
    template <bool side, class Board>
    inline bool checkRoot(const Board& board) {
        nnue::accumulators[0].computed[white] = false;
        nnue::accumulators[0].computed[black] = false;
        nnue::accumulators[0].dirty.clear();

        const int fast = nnue::evaluate<side>(0, board);
        const int ref = oracleEval<side>(board);
        printf("hello \n");
        if (fast != ref) {
            printf("[ORACLE] mismatch (root)  fast=%d  ref=%d  side=%d\n", fast, ref, (int)side);
            return false;
        }
        return true;
    }

    // ================================================================
    //  Harnais 2 : a appeler A CHAQUE NOEUD de la recherche (debug).
    //  Compare l'eval INCREMENTALE (accumulators[ply]) a l'oracle qui
    //  reconstruit tout depuis le board. Concordance => incremental +
    //  convention + forward tous corrects. Subsume debugCheck cote eval.
    // ================================================================
    template <bool side, class Board>
    inline bool checkNode(int ply, const Board& board) {
        const int fast = nnue::evaluate<side>(ply, board);
        const int ref = oracleEval<side>(board);
        if (fast != ref) {
            printf("[ORACLE] mismatch ply=%d from=%d to=%d cap=%d promo=%d king=%d ep=%d side=%d  fast=%d ref=%d\n",
                ply, board.from, board.to, board.cap, board.promo, board.king, board.eP,
                (int)board.side, fast, ref);
            return false;
        }
        return true;
    }

} // namespace nnue_oracle