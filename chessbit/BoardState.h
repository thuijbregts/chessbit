#pragma once

#include <cstdint>
#include "Definitions.h"
#include "Eval.h"
#include "Zobrist.h"
#include "Nnue.h"

using namespace defs;
using namespace eval;
using namespace zobrist;
using namespace nnue;

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
        DirtyPiece dirty;

        int score;
        int mg;
        int eg;
        int8_t phase;

        int8_t toPrev;
        int8_t from;
        int8_t to;
        int8_t kMS;
        int8_t kES;
        int8_t casPerms;
        int8_t eP;
        int8_t vctm;
        int8_t atkrPrev;
        int8_t atkr;
        int8_t promoted;
        uint8_t   halfClock;
        bool   side;
        bool   cap;
        bool   promo;
        bool   king;

        constexpr BoardState() = default;

        constexpr BoardState(
            int8_t toPrev, int8_t from, int8_t to,
            U64 pM, U64 nM, U64 bM, U64 rM, U64 qM, U64 kM,
            U64 pE, U64 nE, U64 bE, U64 rE, U64 qE, U64 kE,
            int8_t kMS, int8_t kES, U64 kMA, U64 kEA,
            U64 occM, U64 occE, U64 occB,
            U64 checks, int8_t casPerms, int8_t eP, int8_t vctm, int8_t atkrPrev, int8_t atkr, int8_t promoted,
            bool side, bool cap, bool promo, bool king, int score, int mg, int eg, int8_t phase, uint8_t halfClock, Zobrist zobrist) noexcept :
            pM(pM), nM(nM), bM(bM), rM(rM), qM(qM), kM(kM),
            pE(pE), nE(nE), bE(bE), rE(rE), qE(qE), kE(kE),
            kMA(kMA), kEA(kEA),
            occM(occM), occE(occE), occB(occB),
            checks(checks), zobrist(zobrist),
            score(score), mg(mg), eg(eg), phase(phase),
            toPrev(toPrev), from(from), to(to),
            kMS(kMS), kES(kES),
            casPerms(casPerms), eP(eP), vctm(vctm), atkrPrev(atkrPrev), atkr(atkr), promoted(promoted),
            halfClock(halfClock), side(side), cap(cap), promo(promo), king(king)
        {

        }

        template <Piece piece, bool side, bool capture, uint8_t kMoved>
        ForceInline BoardState make(int from, int to, const BoardState& board, U64 discovers) noexcept {
            constexpr bool kMMoved = kMoved & KING_MOVED[side];
            constexpr bool kEMoved = kMoved & KING_MOVED[!side];

            const U64 t = (1ULL << to);
            const U64 move = (1ULL << from) | t;

            U64 pM = board.pM;
            U64 nM = board.nM;
            U64 bM = board.bM;
            U64 rM = board.rM;
            U64 qM = board.qM;
            U64 kM = board.kM;

            U64 pE = board.pE;
            U64 nE = board.nE;
            U64 bE = board.bE;
            U64 rE = board.rE;
            U64 qE = board.qE;

            const U64 occM = board.occM ^ move;
            U64 occE = board.occE;
            U64 occB;
            if constexpr (capture)  occB = board.occB ^ (1ULL << from);
            else                    occB = board.occB ^ move;

            uint8_t halfClock = board.halfClock;
            if constexpr (piece == Piece::Pawn || capture) halfClock = 0;
            else halfClock++;

            int casPerms = board.casPerms;
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
                if constexpr (!kMMoved) casPerms &= NO_CASTLE_ROOK[from];
            }
            if constexpr (Piece::Queen == piece) {
                qM ^= move;
                if ((QUEEN_XRAYS[board.kES] & t) && !(PIN_MASKS[board.kES][to] & occB)) [[unlikely]] checks |= t;
            }
            if constexpr (Piece::King == piece) {
                kM ^= move;
                if constexpr (!kMMoved) casPerms &= NO_CASTLE[side];
            }

            DirtyPiece dp;
            if constexpr (piece != Piece::King) {
                dp.push(false, halfKAIndex<piece, side, side>(from, board.kMS, board.kES));
                dp.push(true, halfKAIndex<piece, side, side>(to, board.kMS, board.kES));
            }
            else {
                dp.refresh[side] = true;
                dp.push(false, halfKAIndex<piece, side, side>(from, board.kMS, board.kES));
                dp.push(true, halfKAIndex<piece, side, side>(to, board.kMS, board.kES));
            }

            int score, mg = board.mg, eg = board.eg;
            int8_t phase = board.phase, v = NO_CAPTURE;
            Zobrist zobrist;

            if constexpr (capture) {
                occE ^= t;

                if (pE & t) {
                    pE ^= t;
                    score = eval::update<piece, Piece::Pawn, side>(from, to, mg, eg, phase);
                    zobrist = zobrist::basic<piece, side, Piece::Pawn>(from, to, casPerms, board.casPerms, board.eP, board.zobrist);
                    dp.push(false, halfKAIndex<Piece::Pawn, side, !side>(to, board.kMS, board.kES));
                    v = p;
                }
                else if (nE & t) {
                    nE ^= t;
                    score = eval::update<piece, Piece::Knight, side>(from, to, mg, eg, phase);
                    zobrist = zobrist::basic<piece, side, Piece::Knight>(from, to, casPerms, board.casPerms, board.eP, board.zobrist);
                    dp.push(false, halfKAIndex<Piece::Knight, side, !side>(to, board.kMS, board.kES));
                    v = n;
                }
                else if (bE & t) {
                    bE ^= t;
                    score = eval::update<piece, Piece::Bishop, side>(from, to, mg, eg, phase);
                    zobrist = zobrist::basic<piece, side, Piece::Bishop>(from, to, casPerms, board.casPerms, board.eP, board.zobrist);
                    dp.push(false, halfKAIndex<Piece::Bishop, side, !side>(to, board.kMS, board.kES));
                    v = b;
                }
                else if (rE & t) {
                    if constexpr (!kEMoved) casPerms &= NO_CASTLE_ROOK[to];

                    rE ^= t;
                    score = eval::update<piece, Piece::Rook, side>(from, to, mg, eg, phase);
                    zobrist = zobrist::basic<piece, side, Piece::Rook>(from, to, casPerms, board.casPerms, board.eP, board.zobrist);
                    dp.push(false, halfKAIndex<Piece::Rook, side, !side>(to, board.kMS, board.kES));
                    v = r;
                }
                else {
                    qE ^= t;
                    score = eval::update<piece, Piece::Queen, side>(from, to, mg, eg, phase);
                    zobrist = zobrist::basic<piece, side, Piece::Queen>(from, to, casPerms, board.casPerms, board.eP, board.zobrist);
                    dp.push(false, halfKAIndex<Piece::Queen, side, !side>(to, board.kMS, board.kES));
                    v = q;
                }
            }
            else {
                score = eval::update<piece, Piece::King, side>(from, to, mg, eg, phase);
                zobrist = zobrist::basic<piece, side, Piece::King>(from, to, casPerms, board.casPerms, board.eP, board.zobrist);
            }

            if constexpr (Piece::Queen != piece) {
                if (discovers) [[unlikely]] {
                    U64 disc = discovers & DISCOVER_RAYS[board.kES][from];
                    if (disc && !(DISCOVER_RAYS[board.kES][to] & disc)) [[unlikely]] checks |= disc;
                }
            }

            if constexpr (Piece::King == piece) {
                BoardState child(board.to, from, to, pE, nE, bE, rE, qE, board.kE, pM, nM, bM, rM, qM, kM, board.kES, to, board.kEA, getKingAttacks(to), occE, occM, occB, checks, casPerms, noSquare, v, board.atkr, k, noPiece, !side, capture, false, true, -score, -mg, -eg, phase, halfClock, zobrist);
                child.dirty = dp;
                return child;
            }
            else {
                BoardState child(board.to, from, to, pE, nE, bE, rE, qE, board.kE, pM, nM, bM, rM, qM, kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare, v, board.atkr, static_cast<int>(piece), noPiece, !side, capture, false, false, -score, -mg, -eg, phase, halfClock, zobrist);
                child.dirty = dp;
                return child;
            }
        }

        template <Piece piece, bool side, bool capture, uint8_t kMoved>
        ForceInline BoardState makePromotion(int from, int to, const BoardState& board, U64 discovers) noexcept {
            constexpr bool kEMoved = kMoved & KING_MOVED[!side];

            DirtyPiece dp;
            dp.push(false, halfKAIndex<Piece::Pawn, side, side>(from, board.kMS, board.kES));
            dp.push(true, halfKAIndex<piece, side, side>(to, board.kMS, board.kES));

            const U64 f = (1ULL << from);
            const U64 t = (1ULL << to);

            const U64 pM = board.pM ^ f;
            U64 nM = board.nM;
            U64 bM = board.bM;
            U64 rM = board.rM;
            U64 qM = board.qM;

            U64 nE = board.nE;
            U64 bE = board.bE;
            U64 rE = board.rE;
            U64 qE = board.qE;

            const U64 occM = board.occM ^ (f | t);
            U64 occE = board.occE;
            U64 occB;
            if constexpr (capture)  occB = board.occB ^ f;
            else                    occB = board.occB ^ (f | t);

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

            int casPerms = board.casPerms, score, mg = board.mg, eg = board.eg, v = NO_CAPTURE;
            int8_t phase = board.phase;
            Zobrist zobrist;

            if constexpr (capture) {
                occE ^= t;

                if (nE & t) {
                    nE ^= t;
                    score = eval::updatePromotion<piece, Piece::Knight, side>(from, to, mg, eg, phase);
                    zobrist = zobrist::promotion<piece, side, Piece::Knight>(from, to, casPerms, board.casPerms, board.eP, board.zobrist);
                    dp.push(false, halfKAIndex<Piece::Knight, side, !side>(to, board.kMS, board.kES));
                    v = n;
                }
                else if (bE & t) {
                    bE ^= t;
                    score = eval::updatePromotion<piece, Piece::Bishop, side>(from, to, mg, eg, phase);
                    zobrist = zobrist::promotion<piece, side, Piece::Bishop>(from, to, casPerms, board.casPerms, board.eP, board.zobrist);
                    dp.push(false, halfKAIndex<Piece::Bishop, side, !side>(to, board.kMS, board.kES));
                    v = b;
                }
                else if (rE & t) {
                    if constexpr (!kEMoved)  casPerms &= NO_CASTLE_ROOK[to];

                    rE ^= t;
                    score = eval::updatePromotion<piece, Piece::Rook, side>(from, to, mg, eg, phase);
                    zobrist = zobrist::promotion<piece, side, Piece::Rook>(from, to, casPerms, board.casPerms, board.eP, board.zobrist);
                    dp.push(false, halfKAIndex<Piece::Rook, side, !side>(to, board.kMS, board.kES));
                    v = r;
                }
                else {
                    qE ^= t;
                    score = eval::updatePromotion<piece, Piece::Queen, side>(from, to, mg, eg, phase);
                    zobrist = zobrist::promotion<piece, side, Piece::Queen>(from, to, casPerms, board.casPerms, board.eP, board.zobrist);
                    dp.push(false, halfKAIndex<Piece::Queen, side, !side>(to, board.kMS, board.kES));
                    v = q;
                }
            }
            else {
                score = eval::updatePromotion<piece, Piece::King, side>(from, to, mg, eg, phase);
                zobrist = zobrist::promotion<piece, side, Piece::King>(from, to, casPerms, board.casPerms, board.eP, board.zobrist);
            }

            if (discovers) [[unlikely]] checks |= discovers & DISCOVER_RAYS[board.kES][from];

            BoardState child(board.to, from, to, board.pE, nE, bE, rE, qE, board.kE, pM, nM, bM, rM, qM, board.kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare, v, board.atkr, p, static_cast<int>(piece), !side, capture, true, false, -score, -mg, -eg, phase, 0, zobrist);
            child.dirty = dp;
            return child;
        }

        template <bool side>
        ForceInline BoardState makeDoublePush(int from, int to, const BoardState& board, U64 discovers) noexcept {
            DirtyPiece dp;
            dp.push(false, halfKAIndex<Piece::Pawn, side, side>(from, board.kMS, board.kES));
            dp.push(true, halfKAIndex<Piece::Pawn, side, side>(to, board.kMS, board.kES));

            int8_t eP = from + PAWN_PUSH[side];
            if (!(PAWN_CAPTURES[side][eP] & board.pE)) eP = noSquare;

            int mg = board.mg, eg = board.eg;
            int score = eval::updateDoublePush<side>(from, to, mg, eg, board.phase);
            const Zobrist zobrist = zobrist::doublePush<side>(from, to, eP, board.eP, board.zobrist);

            const U64 t = (1ULL << to);
            const U64 move = (1ULL << from) | t;

            const U64 pM = board.pM ^ move;
            const U64 occM = board.occM ^ move;

            const U64 occB = board.occB ^ move;

            U64 checks;
            if (discovers) [[unlikely]] {
                U64 disc = discovers & DISCOVER_RAYS[board.kES][from] & ~FILE_BIT[from];
                if (disc) [[unlikely]] checks = disc;
                else                    checks = (PAWN_CAPTURES[!side][board.kES] & pM);
            }
            else                        checks = (PAWN_CAPTURES[!side][board.kES] & pM);

            BoardState child(board.to, from, to, board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, board.nM, board.bM, board.rM, board.qM, board.kM, board.kES, board.kMS, board.kEA, board.kMA, board.occE, occM, occB, checks, board.casPerms, eP, NO_CAPTURE, board.atkr, p, noPiece, !side, false, false, false, -score, -mg, -eg, board.phase, 0, zobrist);
            child.dirty = dp;
            return child;
        }

        template <bool side>
        ForceInline BoardState makeEnPassant(int from, int to, const BoardState& board) noexcept {
            const int ePS = (to + PAWN_PUSH[!side]);

            DirtyPiece dp;
            dp.push(false, halfKAIndex<Piece::Pawn, side, side>(from, board.kMS, board.kES));
            dp.push(true, halfKAIndex<Piece::Pawn, side, side>(to, board.kMS, board.kES));
            dp.push(false, halfKAIndex<Piece::Pawn, side, !side>(ePS, board.kMS, board.kES));

            int mg = board.mg, eg = board.eg;
            int score = eval::updateEnPassant<side>(from, to, ePS, mg, eg, board.phase);
            const Zobrist zobrist = zobrist::enPassant<side>(from, to, ePS, board.eP, board.zobrist);

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

            BoardState child(board.to, from, to, pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, board.nM, board.bM, board.rM, board.qM, board.kM, board.kES, board.kMS, board.kEA, board.kMA, occE, occM, occB, checks, board.casPerms, noSquare, p, board.atkr, p, noPiece, !side, true, false, false, -score, -mg, -eg, board.phase, 0, zobrist);
            child.dirty = dp;
            return child;
        }

        template <int castlingSide>
        ForceInline BoardState makeCastling(const BoardState& board) noexcept {
            constexpr bool side = CASTLING_SIDE[castlingSide];
            const int casPerms = board.casPerms & NO_CASTLE[side];

            int mg = board.mg, eg = board.eg;
            int score = eval::updateCastling<castlingSide>(mg, eg, board.phase);
            const Zobrist zobrist = zobrist::castle<castlingSide>(casPerms, board.casPerms, board.eP, board.zobrist);

            const U64 kM = board.kM ^ kingSwitch<castlingSide>();
            const U64 rM = board.rM ^ rookSwitch<castlingSide>();

            const U64 occM = board.occM ^ bothSwitch<castlingSide>();
            const U64 occB = board.occB ^ bothSwitch<castlingSide>();

            const U64 checks = getRookAttacks(board.kES, occB) & rM;

            constexpr int from = CASTLING_KING_SOURCE_SQUARE[castlingSide];
            constexpr int to = CASTLING_KING_TARGET_SQUARE[castlingSide];
            constexpr int rFrom = CASTLING_ROOK_SOURCE_SQUARE[castlingSide];
            constexpr int rTo = CASTLING_ROOK_TARGET_SQUARE[castlingSide];

            DirtyPiece dp;
            dp.refresh[side] = true;
            dp.push(false, halfKAIndex<Piece::King, side, side>(from, board.kMS, board.kES));
            dp.push(true, halfKAIndex<Piece::King, side, side>(to, board.kMS, board.kES));
            dp.push(false, halfKAIndex<Piece::Rook, side, side>(rFrom, board.kMS, board.kES));
            dp.push(true, halfKAIndex<Piece::Rook, side, side>(rTo, board.kMS, board.kES));

            BoardState child(board.to, from, to, board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, board.pM, board.nM, board.bM, rM, board.qM, kM, board.kES, to, board.kEA, KING_ATTACKS[to], board.occE, occM, occB, checks, casPerms, noSquare, NO_CAPTURE, board.atkr, k, noPiece, !side, false, false, true, -score, -mg, -eg, board.phase, board.halfClock + 1, zobrist);
            child.dirty = dp;
            return child;
        }

        template <bool side>
        ForceInline BoardState makeNull(const BoardState& board) noexcept {
            const Zobrist zobrist = zobrist::null(board.eP, board.zobrist);

            return BoardState(0, 0, 0, board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, board.pM, board.nM, board.bM, board.rM, board.qM, board.kM, board.kES, board.kMS, board.kEA, board.kMA, board.occE, board.occM, board.occB, 0, board.casPerms, noSquare, 0, 0, 0, noPiece, !side, false, false, false, -board.score, -board.mg, -board.eg, board.phase, board.halfClock, zobrist);
        }

        ForceInline bool hasEnoughMaterial(const BoardState& board) noexcept {
            return board.occM & ~(board.pM | board.kM);
        }
    };

    inline static BoardState dummy = BoardState(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0 });
}