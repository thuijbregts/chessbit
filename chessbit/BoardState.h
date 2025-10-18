#pragma once

#include "Definitions.h"
#include "Zobrist.h"

using namespace defs;
using namespace zobrist;

namespace bstate {
    struct BoardState {
        U64 pM;
        U64 nM;
        U64 bM;
        U64 rM;
        U64 qM;
        U64 kM;

        U64 pE;
        U64 nE;
        U64 bE;
        U64 rE;
        U64 qE;
        U64 kE;

        int kMS;
        int kES;

        U64 kMA;
        U64 kEA;

        U64 occM;
        U64 occE;
        U64 occB;

        U64 checks;
        int casPerms;
        int eP;

        bool side;

        Zobrist zobrist;

        constexpr BoardState(
            U64 pM, U64 nM, U64 bM, U64 rM, U64 qM, U64 kM,
            U64 pE, U64 nE, U64 bE, U64 rE, U64 qE, U64 kE,
            int kMS, int kES, U64 kMA, U64 kEA,
            U64 occM, U64 occE, U64 occB,
            U64 checks, int casPerms, int eP, bool side, Zobrist zobrist) :
            pM(pM), nM(nM), bM(bM), rM(rM), qM(qM), kM(kM),
            pE(pE), nE(nE), bE(bE), rE(rE), qE(qE), kE(kE),
            kMS(kMS), kES(kES), kMA(kMA), kEA(kEA),
            occM(occM), occE(occE), occB(occB),
            checks(checks), casPerms(casPerms), eP(eP), side(side), zobrist(zobrist)
        {

        }

        ForceInline U64 sliderChecks(U64 bM, U64 rM, U64 qM, U64 occB, int kES) {
            U64 checks = 0ULL;
            if (BISHOP_XRAYS[kES] & (bM | qM))  checks |= getBishopAttacks(kES, occB) & (bM | qM);
            if (ROOK_XRAYS[kES] & (rM | qM))    checks |= getRookAttacks(kES, occB) & (rM | qM);
            return checks;
        }

        template <Piece piece, bool side, bool capture>
        ForceInline BoardState make(int from, int to, const BoardState& board) {
            const U64 t = (1ULL << to);
            const U64 move = (1ULL << from) | t;

            U64 pM = board.pM;
            U64 nM = board.nM;
            U64 bM = board.bM;
            U64 rM = board.rM;
            U64 qM = board.qM;
            U64 kM = board.kM;

            int casPerms = board.casPerms;

            const U64 occM = board.occM ^ move;
            U64 occE = board.occE;
            const U64 occB = occM | occE;

            U64 checks = 0ULL;

            if constexpr (Piece::Pawn == piece) {
                pM ^= move;
                checks |= PAWN_CAPTURES[!side][board.kES] & pM;
            }
            if constexpr (Piece::Knight == piece) {
                nM ^= move;
                checks |= KNIGHT_ATTACKS[board.kES] & nM;
            }
            if constexpr (Piece::Bishop == piece)   bM ^= move;
            if constexpr (Piece::Rook == piece) {
                rM ^= move;
                casPerms &= NO_CASTLE_ROOK[from];
            }
            if constexpr (Piece::Queen == piece)    qM ^= move;
            if constexpr (Piece::King == piece) {
                kM ^= move;
                casPerms &= NO_CASTLE[side];
            }

            checks |= sliderChecks(bM, rM, qM, occB, board.kES);

            if constexpr (capture) {
                occE ^= t;
                casPerms &= NO_CASTLE_ROOK[to];

                const Zobrist zobrist = zobrist::basic<piece, side, capture>(from, to, casPerms, board.casPerms, board.eP, board.pE, board.nE, board.bE, board.rE, board.qE, board.zobrist);

                checks |= sliderChecks(bM, rM, qM, occB, board.kES);
                if constexpr (Piece::King == piece)         return BoardState(board.pE & occE, board.nE & occE, board.bE & occE, board.rE & occE, board.qE & occE, board.kE, pM, nM, bM, rM, qM, kM, board.kES, to, board.kEA, getKingAttacks(to), occE, occM, occB, checks, casPerms, noSquare, !side, zobrist);
                else                                        return BoardState(board.pE & occE, board.nE & occE, board.bE & occE, board.rE & occE, board.qE & occE, board.kE, pM, nM, bM, rM, qM, kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare, !side, zobrist);
            }
            else {
                const Zobrist zobrist = zobrist::basic<piece, side, capture>(from, to, casPerms, board.casPerms, board.eP, board.pE, board.nE, board.bE, board.rE, board.qE, board.zobrist);

                if constexpr (Piece::King == piece)         return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, nM, bM, rM, qM, kM, board.kES, to, board.kEA, getKingAttacks(to), occE, occM, occB, checks, casPerms, noSquare, !side, zobrist);
                else                                        return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, nM, bM, rM, qM, kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare, !side, zobrist);
            }
        }

        template <Piece piece, bool side, bool capture>
        ForceInline BoardState makePromotion(int from, int to, const BoardState& board) {
            const U64 f = (1ULL << from);
            const U64 t = (1ULL << to);

            const U64 pM = board.pM ^ f;
            U64 nM = board.nM;
            U64 bM = board.bM;
            U64 rM = board.rM;
            U64 qM = board.qM;

            const U64 occM = board.occM ^ (f | t);
            U64 occE = board.occE;
            const U64 occB = occM | occE;

            U64 checks = 0ULL;

            if constexpr (Piece::Knight == piece) {
                nM ^= t;
                checks |= KNIGHT_ATTACKS[board.kES] & nM;
            }
            if constexpr (Piece::Bishop == piece)   bM ^= t;
            if constexpr (Piece::Rook == piece)     rM ^= t;
            if constexpr (Piece::Queen == piece)    qM ^= t;

            checks |= sliderChecks(bM, rM, qM, occB, board.kES);

            if constexpr (capture) {
                occE ^= t;
                const int casPerms = board.casPerms & NO_CASTLE_ROOK[to];

                const Zobrist zobrist = zobrist::promotion<piece, side, capture>(from, to, casPerms, board.casPerms, board.eP, board.nE, board.bE, board.rE, board.qE, board.zobrist);

                return BoardState(board.pE, board.nE & occE, board.bE & occE, board.rE & occE, board.qE & occE, board.kE, pM, nM, bM, rM, qM, board.kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare, !side, zobrist);
            }
            else {
                const Zobrist zobrist = zobrist::promotion<piece, side, capture>(from, to, board.casPerms, board.casPerms, board.eP, board.nE, board.bE, board.rE, board.qE, board.zobrist);

                return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, nM, bM, rM, qM, board.kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, board.casPerms, noSquare, !side, zobrist);
            }
        }

        template <bool side>
        ForceInline BoardState makeDoublePush(int from, int to, const BoardState& board) {
            const U64 move = (1ULL << from) | (1ULL << to);

            const U64 pM = board.pM ^ move;
            const U64 occM = board.occM ^ move;

            const U64 occB = occM | board.occE;

            const U64 checks = (PAWN_CAPTURES[!side][board.kES] & pM) | sliderChecks(board.bM, board.rM, board.qM, occB, board.kES);

            const int eP = from + PAWN_PUSH[side];

            const Zobrist zobrist = zobrist::doublePush<side>(from, to, eP, board.eP, board.zobrist);

            return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, board.nM, board.bM, board.rM, board.qM, board.kM, board.kES, board.kMS, board.kEA, board.kMA, board.occE, occM, occB, checks, board.casPerms, eP, !side, zobrist);
        }

        template <bool side>
        ForceInline BoardState makeEnPassant(int from, int to, const BoardState& board) {
            const U64 move = (1ULL << from) | (1ULL << to);

            const U64 pM = board.pM ^ move;
            const U64 occM = board.occM ^ move;

            const int ePS = to + PAWN_PUSH[!side];
            const U64 ePP = (1ULL << ePS);
            const U64 pE = board.pE ^ ePP;
            const U64 occE = board.occE ^ ePP;

            const U64 occB = occM | occE;

            const U64 checks = (PAWN_CAPTURES[!side][board.kES] & pM) | sliderChecks(board.bM, board.rM, board.qM, occB, board.kES);

            const Zobrist zobrist = zobrist::enPassant<side>(from, to, ePS, board.eP, board.zobrist);

            return BoardState(pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, board.nM, board.bM, board.rM, board.qM, board.kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, board.casPerms, noSquare, !side, zobrist);
        }

        template <int castlingSide>
        ForceInline U64 rookSwitch() {
            if constexpr (castlingSide == 0) return 0xa000000000000000;
            if constexpr (castlingSide == 1) return 0x900000000000000;
            if constexpr (castlingSide == 2) return 0xa0;
            if constexpr (castlingSide == 3) return 0x9;
        }

        template <int castlingSide>
        ForceInline U64 kingSwitch() {
            if constexpr (castlingSide == 0) return 0x5000000000000000;
            if constexpr (castlingSide == 1) return 0x1400000000000000;
            if constexpr (castlingSide == 2) return 0x50;
            if constexpr (castlingSide == 3) return 0x14;
        }

        template <int castlingSide>
        ForceInline U64 bothSwitch() {
            if constexpr (castlingSide == 0) return 0xa000000000000000 | 0x5000000000000000;
            if constexpr (castlingSide == 1) return 0x900000000000000 | 0x1400000000000000;
            if constexpr (castlingSide == 2) return 0xa0 | 0x50;
            if constexpr (castlingSide == 3) return 0x9 | 0x14;
        }

        template <int castlingSide>
        ForceInline BoardState makeCastling(const BoardState& board) {
            const U64 kM = board.kM ^ kingSwitch<castlingSide>();
            const U64 rM = board.rM ^ rookSwitch<castlingSide>();

            const U64 occM = board.occM ^ bothSwitch<castlingSide>();
            const U64 occB = occM | board.occE;

            const bool side = CASTLING_SIDE[castlingSide];
            const int casPerms = board.casPerms & NO_CASTLE[side];

            const U64 checks = getRookAttacks(board.kES, occB) & rM;

            const int to = CASTLING_KING_TARGET_SQUARE[castlingSide];

            const Zobrist zobrist = zobrist::castle<castlingSide>(casPerms, board.casPerms, board.eP, board.zobrist);

            return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, board.pM, board.nM, board.bM, rM, board.qM, kM, board.kES, to, board.kEA, getKingAttacks(to), board.occE, occM, occB, checks, casPerms, noSquare, !side, zobrist);
        }
    };

    inline static BoardState dummy = BoardState(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0 });

    struct Entry {
        BoardState board;
        int qty;

        constexpr Entry() : board(dummy), qty(0) {}
        constexpr Entry(const BoardState& board, int qty) : board(board), qty(qty) {}
    };
}