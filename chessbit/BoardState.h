#pragma once

#include <cstdint>
#include "Definitions.h"
#include "Zobrist.h"

using namespace defs;
using namespace zobrist;

namespace moveinfo {
    struct MoveInfo;
}

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

        U64 kMA;
        U64 kEA;

        U64 occM;
        U64 occE;
        U64 occB;

        U64 checks;

        Zobrist zobrist;

        int8_t from;
        int8_t to;
        int8_t kMS;
        int8_t kES;
        int8_t casPerms;
        int8_t eP;
        int8_t vctm;
        int8_t atkr;
        bool   side;
        bool   cap;
        bool   promo;
        bool   king;

        constexpr BoardState() = default;

        constexpr BoardState(
            int8_t from, int8_t to,
            U64 pM, U64 nM, U64 bM, U64 rM, U64 qM, U64 kM,
            U64 pE, U64 nE, U64 bE, U64 rE, U64 qE, U64 kE,
            int8_t kMS, int8_t kES, U64 kMA, U64 kEA,
            U64 occM, U64 occE, U64 occB,
            U64 checks, int8_t casPerms, int8_t eP, int8_t vctm, int8_t atkr,
            bool side, bool cap, bool promo, bool king, Zobrist zobrist) noexcept :
            pM(pM), nM(nM), bM(bM), rM(rM), qM(qM), kM(kM),
            pE(pE), nE(nE), bE(bE), rE(rE), qE(qE), kE(kE),
            kMA(kMA), kEA(kEA),
            occM(occM), occE(occE), occB(occB),
            checks(checks), zobrist(zobrist),
            from(from), to(to),
            kMS(kMS), kES(kES),
            casPerms(casPerms), eP(eP), vctm(vctm), atkr(atkr),
            side(side), cap(cap), promo(promo), king(king)
        {

        }

        template <Piece piece, bool side, bool capture, uint8_t kMoved>
        ForceInline BoardState make(int from, int to, const BoardState& board, U64 discovers) noexcept {
            constexpr bool kMMoved = kMoved & KING_MOVED[side];
            constexpr bool kEMoved = kMoved & KING_MOVED[!side];

            int casPerms = board.casPerms;
            if constexpr (!kMMoved) {
                if      constexpr (Piece::Rook == piece)    casPerms &= NO_CASTLE_ROOK[from];
                else if constexpr (Piece::King == piece)    casPerms &= NO_CASTLE[side];
            }
            if constexpr (capture && !kEMoved) {
                casPerms &= NO_CASTLE_ROOK[to];
            }

            int v = NO_CAPTURE;
            Zobrist zobrist = zobrist::basic<piece, side, capture>(from, to, casPerms, board.casPerms, board.eP, board.pE, board.nE, board.bE, board.rE, board.qE, board.zobrist, v);

            const U64 t = (1ULL << to);
            const U64 move = (1ULL << from) | t;

            U64 pM = board.pM;
            U64 nM = board.nM;
            U64 bM = board.bM;
            U64 rM = board.rM;
            U64 qM = board.qM;
            U64 kM = board.kM;

            const U64 occM = board.occM ^ move;
            U64 occE = board.occE;
            U64 occB;
            if constexpr (capture) occB = board.occB ^ (1ULL << from);
            else                   occB = board.occB ^ move;

            U64 checks = 0ULL;

            if constexpr (Piece::Pawn == piece) {
                pM ^= move;
                checks |= PAWN_CAPTURES[!side][board.kES] & pM;
            }
            if constexpr (Piece::Knight == piece) {
                nM ^= move;
                checks |= KNIGHT_ATTACKS[board.kES] & nM;
            }
            if constexpr (Piece::Bishop == piece) {
                bM ^= move;
                if ((BISHOP_XRAYS[board.kES] & t) && !(PIN_MASKS[board.kES][to] & occB)) [[unlikely]] checks |= t;
            }
            if constexpr (Piece::Rook == piece) {
                rM ^= move;
                if ((ROOK_XRAYS[board.kES] & t) && !(PIN_MASKS[board.kES][to] & occB)) [[unlikely]] checks |= t;
            }
            if constexpr (Piece::Queen == piece) {
                qM ^= move;
                if ((QUEEN_XRAYS[board.kES] & t) && !(PIN_MASKS[board.kES][to] & occB)) [[unlikely]] checks |= t;
            }
            if constexpr (Piece::King == piece) kM ^= move;

            if constexpr (Piece::Queen != piece) {
                if (discovers) [[unlikely]] {
                    U64 disc = discovers & DISCOVER_RAYS[board.kES][from];
                    if (disc && !(DISCOVER_RAYS[board.kES][to] & disc)) [[unlikely]] checks |= disc;
                }
            }

            if constexpr (capture) {
                occE ^= t;

                if constexpr (Piece::King == piece)         return BoardState(from, to, board.pE & occE, board.nE & occE, board.bE & occE, board.rE & occE, board.qE & occE, board.kE, pM, nM, bM, rM, qM, kM, board.kES, to, board.kEA, getKingAttacks(to), occE, occM, occB, checks, casPerms, noSquare, v, k, !side, true, false, true, zobrist);
                else                                        return BoardState(from, to, board.pE & occE, board.nE & occE, board.bE & occE, board.rE & occE, board.qE & occE, board.kE, pM, nM, bM, rM, qM, kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare, v, static_cast<int>(piece), !side, true, false, false, zobrist);
            }
            else {
                if constexpr (Piece::King == piece)         return BoardState(from, to, board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, nM, bM, rM, qM, kM, board.kES, to, board.kEA, getKingAttacks(to), occE, occM, occB, checks, casPerms, noSquare, 0, 0, !side, false, false, true, zobrist);
                else                                        return BoardState(from, to, board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, nM, bM, rM, qM, kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare, 0, 0, !side, false, false, false, zobrist);
            }
        }

        template <Piece piece, bool side, bool capture, uint8_t kMoved>
        ForceInline BoardState makePromotion(int from, int to, const BoardState& board, U64 discovers) noexcept {
            constexpr bool kEMoved = kMoved & KING_MOVED[!side];

            int casPerms = board.casPerms;
            if constexpr (capture && !kEMoved)  casPerms &= NO_CASTLE_ROOK[to];

            int v = NO_CAPTURE;
            Zobrist zobrist = zobrist::promotion<piece, side, capture>(from, to, casPerms, board.casPerms, board.eP, board.nE, board.bE, board.rE, board.qE, board.zobrist, v);

            const U64 f = (1ULL << from);
            const U64 t = (1ULL << to);

            const U64 pM = board.pM ^ f;
            U64 nM = board.nM;
            U64 bM = board.bM;
            U64 rM = board.rM;
            U64 qM = board.qM;

            const U64 occM = board.occM ^ (f | t);
            U64 occE = board.occE;

            U64 occB;
            if constexpr (capture) occB = board.occB ^ f;
            else                   occB = board.occB ^ (f | t);

            U64 checks = 0ULL;

            if constexpr (Piece::Knight == piece) {
                nM ^= t;
                checks |= KNIGHT_ATTACKS[board.kES] & nM;
            }
            if constexpr (Piece::Bishop == piece) {
                bM ^= t;
                if ((BISHOP_XRAYS[board.kES] & t) && !(PIN_MASKS[board.kES][to] & occB)) [[unlikely]] checks |= t;
            }
            if constexpr (Piece::Rook == piece) {
                rM ^= t;
                if ((ROOK_XRAYS[board.kES] & t) && !(PIN_MASKS[board.kES][to] & occB)) [[unlikely]] checks |= t;
            }
            if constexpr (Piece::Queen == piece) {
                qM ^= t;
                if ((QUEEN_XRAYS[board.kES] & t) && !(PIN_MASKS[board.kES][to] & occB)) [[unlikely]] checks |= t;
            }

            if (discovers) [[unlikely]] checks |= discovers & DISCOVER_RAYS[board.kES][from];

            if constexpr (capture) {
                occE ^= t;

                return BoardState(from, to, board.pE, board.nE & occE, board.bE & occE, board.rE & occE, board.qE & occE, board.kE, pM, nM, bM, rM, qM, board.kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare, q, p, !side, true, true, false, zobrist);
            }
            else {
                return BoardState(from, to, board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, nM, bM, rM, qM, board.kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, board.casPerms, noSquare, 0, 0, !side, false, true, false, zobrist);
            }
        }

        template <bool side>
        ForceInline BoardState makeDoublePush(int from, int to, const BoardState& board, U64 discovers) noexcept {
            int8_t eP = from + PAWN_PUSH[side];
            if (!(PAWN_CAPTURES[side][eP] & board.pE)) eP = noSquare;

            Zobrist zobrist = zobrist::doublePush<side>(from, to, eP, board.eP, board.zobrist);

            const U64 t = (1ULL << to);
            const U64 move = (1ULL << from) | t;

            const U64 pM = board.pM ^ move;
            const U64 occM = board.occM ^ move;

            const U64 occB = board.occB ^ move;

            U64 checks;
            if (discovers) [[unlikely]] {
                U64 disc = discovers & DISCOVER_RAYS[board.kES][from] & ~FILE_BIT[from];
                if (disc) [[unlikely]]  checks = disc;
                else                    checks = (PAWN_CAPTURES[!side][board.kES] & pM);
            }
            else                        checks = (PAWN_CAPTURES[!side][board.kES] & pM);

            return BoardState(from, to, board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, board.nM, board.bM, board.rM, board.qM, board.kM, board.kES, board.kMS, board.kEA, board.kMA, board.occE, occM, occB, checks, board.casPerms, eP, 0, 0, !side, false, false, false, zobrist);
        }

        template <bool side>
        ForceInline BoardState makeEnPassant(int from, int to, const BoardState& board) noexcept {
            const int ePS = (to + PAWN_PUSH[!side]);

            Zobrist zobrist = zobrist::enPassant<side>(from, to, ePS, board.eP, board.zobrist);

            const U64 move = (1ULL << from) | (1ULL << to);

            const U64 pM = board.pM ^ move;
            const U64 occM = board.occM ^ move;

            const U64 ePB = (1ULL << ePS);
            const U64 pE = board.pE ^ ePB;
            const U64 occE = board.occE ^ ePB;

            const U64 occB = occM | occE;

            U64 checks = (PAWN_CAPTURES[!side][board.kES] & pM);

            const U64 bqM = board.bM | board.qM;
            const U64 rqM = board.rM | board.qM;
            if (!checks && (BISHOP_XRAYS[board.kES] & (BISHOP_XRAYS[from] | BISHOP_XRAYS[ePS]) & bqM)) [[unlikely]]
                checks |= getBishopAttacks(board.kES, occB) & bqM;
            if (ROOK_XRAYS[board.kES] & ROOK_XRAYS[from] & rqM) [[unlikely]]
                checks |= getRookAttacks(board.kES, occB) & rqM;

            return BoardState(from, to, pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, board.nM, board.bM, board.rM, board.qM, board.kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, board.casPerms, noSquare, p, p, !side, true, false, false, zobrist);
        }

        template <int castlingSide>
        ForceInline BoardState makeCastling(const BoardState& board) noexcept {
            constexpr bool side = CASTLING_SIDE[castlingSide];
            const int casPerms = board.casPerms & NO_CASTLE[side];

            Zobrist zobrist = zobrist::castle<castlingSide>(casPerms, board.casPerms, board.eP, board.zobrist);

            const U64 kM = board.kM ^ kingSwitch<castlingSide>();
            const U64 rM = board.rM ^ rookSwitch<castlingSide>();

            const U64 occM = board.occM ^ bothSwitch<castlingSide>();
            const U64 occB = board.occB ^ bothSwitch<castlingSide>();

            const U64 checks = getRookAttacks(board.kES, occB) & rM;

            constexpr int from = CASTLING_KING_SOURCE_SQUARE[castlingSide];
            constexpr int to = CASTLING_KING_TARGET_SQUARE[castlingSide];

            return BoardState(from, to, board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, board.pM, board.nM, board.bM, rM, board.qM, kM, board.kES, to, board.kEA, KING_ATTACKS[to], board.occE, occM, occB, checks, casPerms, noSquare, 0, 0, !side, false, false, true, zobrist);
        }
    };

    inline static BoardState dummy = BoardState(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0 });
}