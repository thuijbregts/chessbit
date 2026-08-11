#pragma once

#include "Definitions.h"

using namespace defs;

namespace eval {
    ForceInline int init(U64(&pieces)[2][6], bool side, int& mg, int& eg, int8_t& phase) {
        U64 pM = pieces[side][p], nM = pieces[side][n], bM = pieces[side][b];
        U64 rM = pieces[side][r], qM = pieces[side][q], kM = pieces[side][k];
        Bitloop(pM) { int s = SquareOf(pM); mg += PAWN_MG[side][s];   eg += PAWN_EG[side][s]; }
        Bitloop(nM) { int s = SquareOf(nM); mg += KNIGHT_MG[side][s]; eg += KNIGHT_EG[side][s]; phase += 1; }
        Bitloop(bM) { int s = SquareOf(bM); mg += BISHOP_MG[side][s]; eg += BISHOP_EG[side][s]; phase += 1; }
        Bitloop(rM) { int s = SquareOf(rM); mg += ROOK_MG[side][s];   eg += ROOK_EG[side][s];   phase += 2; }
        Bitloop(qM) { int s = SquareOf(qM); mg += QUEEN_MG[side][s];  eg += QUEEN_EG[side][s];  phase += 4; }
        mg += KING_MG[side][SquareOf(kM)];
        eg += KING_EG[side][SquareOf(kM)];

        U64 pE = pieces[!side][p], nE = pieces[!side][n], bE = pieces[!side][b];
        U64 rE = pieces[!side][r], qE = pieces[!side][q], kE = pieces[!side][k];
        Bitloop(pE) { int s = SquareOf(pE); mg -= PAWN_MG[!side][s];   eg -= PAWN_EG[!side][s]; }
        Bitloop(nE) { int s = SquareOf(nE); mg -= KNIGHT_MG[!side][s]; eg -= KNIGHT_EG[!side][s]; phase += 1; }
        Bitloop(bE) { int s = SquareOf(bE); mg -= BISHOP_MG[!side][s]; eg -= BISHOP_EG[!side][s]; phase += 1; }
        Bitloop(rE) { int s = SquareOf(rE); mg -= ROOK_MG[!side][s];   eg -= ROOK_EG[!side][s];   phase += 2; }
        Bitloop(qE) { int s = SquareOf(qE); mg -= QUEEN_MG[!side][s];  eg -= QUEEN_EG[!side][s];  phase += 4; }
        mg -= KING_MG[!side][SquareOf(kE)];
        eg -= KING_EG[!side][SquareOf(kE)];

        phase = phase < 24 ? phase : 24;
        return (mg * phase + eg * (24 - phase)) / 24;
    }

    template <Piece piece, Piece victim, bool side>
    ForceInline int update(int from, int to, int& mg, int& eg, int8_t& phase) noexcept {
        if constexpr (piece == Piece::Pawn)     { mg += PAWN_MG[side][to] - PAWN_MG[side][from];        eg += PAWN_EG[side][to] - PAWN_EG[side][from]; }
        if constexpr (piece == Piece::Knight)   { mg += KNIGHT_MG[side][to] - KNIGHT_MG[side][from];    eg += KNIGHT_EG[side][to] - KNIGHT_EG[side][from]; }
        if constexpr (piece == Piece::Bishop)   { mg += BISHOP_MG[side][to] - BISHOP_MG[side][from];    eg += BISHOP_EG[side][to] - BISHOP_EG[side][from]; }
        if constexpr (piece == Piece::Rook)     { mg += ROOK_MG[side][to] - ROOK_MG[side][from];        eg += ROOK_EG[side][to] - ROOK_EG[side][from]; }
        if constexpr (piece == Piece::Queen)    { mg += QUEEN_MG[side][to] - QUEEN_MG[side][from];      eg += QUEEN_EG[side][to] - QUEEN_EG[side][from]; }
        if constexpr (piece == Piece::King)     { mg += KING_MG[side][to] - KING_MG[side][from];        eg += KING_EG[side][to] - KING_EG[side][from]; }

        if constexpr (victim != Piece::King) {
            if constexpr (victim == Piece::Pawn)    { mg += PAWN_MG[!side][to];    eg += PAWN_EG[!side][to]; }
            if constexpr (victim == Piece::Knight)  { mg += KNIGHT_MG[!side][to];  eg += KNIGHT_EG[!side][to]; phase--; }
            if constexpr (victim == Piece::Bishop)  { mg += BISHOP_MG[!side][to];  eg += BISHOP_EG[!side][to]; phase--; }
            if constexpr (victim == Piece::Rook)    { mg += ROOK_MG[!side][to];    eg += ROOK_EG[!side][to]; phase -= 2; }
            if constexpr (victim == Piece::Queen)   { mg += QUEEN_MG[!side][to];   eg += QUEEN_EG[!side][to]; phase -= 4; }
        }

        phase = phase < 24 ? phase : 24;
        return (mg * phase + eg * (24 - phase)) / 24;
    }

    template <Piece piece, Piece victim, bool side>
    ForceInline int updatePromotion(int from, int to, int& mg, int& eg, int8_t& phase) noexcept {
        mg -= PAWN_MG[side][from];
        eg -= PAWN_EG[side][from];

        if constexpr (piece == Piece::Knight)   { mg += KNIGHT_MG[side][to];    eg += KNIGHT_EG[side][to]; phase++; }
        if constexpr (piece == Piece::Bishop)   { mg += BISHOP_MG[side][to];    eg += BISHOP_EG[side][to]; phase++; }
        if constexpr (piece == Piece::Rook)     { mg += ROOK_MG[side][to];      eg += ROOK_EG[side][to]; phase += 2; }
        if constexpr (piece == Piece::Queen)    { mg += QUEEN_MG[side][to];     eg += QUEEN_EG[side][to]; phase += 4; }

        if constexpr (victim != Piece::King) {
            if constexpr (victim == Piece::Pawn)    { mg += PAWN_MG[!side][to];    eg += PAWN_EG[!side][to]; }
            if constexpr (victim == Piece::Knight)  { mg += KNIGHT_MG[!side][to];  eg += KNIGHT_EG[!side][to]; phase--; }
            if constexpr (victim == Piece::Bishop)  { mg += BISHOP_MG[!side][to];  eg += BISHOP_EG[!side][to]; phase--; }
            if constexpr (victim == Piece::Rook)    { mg += ROOK_MG[!side][to];    eg += ROOK_EG[!side][to]; phase -= 2; }
            if constexpr (victim == Piece::Queen)   { mg += QUEEN_MG[!side][to];   eg += QUEEN_EG[!side][to]; phase -= 4; }
        }

        phase = phase < 24 ? phase : 24;
        return (mg * phase + eg * (24 - phase)) / 24;
    }

    template <bool side>
    ForceInline int updateDoublePush(int from, int to, int& mg, int& eg, int8_t phase) noexcept {
        mg += PAWN_MG[side][to] - PAWN_MG[side][from];
        eg += PAWN_EG[side][to] - PAWN_EG[side][from];

        phase = phase < 24 ? phase : 24;
        return (mg * phase + eg * (24 - phase)) / 24;
    }

    template <bool side>
    ForceInline int updateEnPassant(int from, int to, int ePS, int& mg, int& eg, int8_t phase) noexcept {
        mg += PAWN_MG[side][to] - PAWN_MG[side][from];
        eg += PAWN_EG[side][to] - PAWN_EG[side][from];

        mg += PAWN_MG[!side][ePS];
        eg += PAWN_EG[!side][ePS];

        phase = phase < 24 ? phase : 24;
        return (mg * phase + eg * (24 - phase)) / 24;
    }

    template <int castlingSide>
    ForceInline int updateCastling(int& mg, int& eg, int8_t phase) noexcept {
        constexpr bool side = CASTLING_SIDE[castlingSide];
        constexpr int fromK = CASTLING_KING_SOURCE_SQUARE[castlingSide];
        constexpr int toK = CASTLING_KING_TARGET_SQUARE[castlingSide];
        constexpr int fromR = CASTLING_ROOK_SOURCE_SQUARE[castlingSide];
        constexpr int toR = CASTLING_ROOK_TARGET_SQUARE[castlingSide];

        mg += KING_MG[side][toK] - KING_MG[side][fromK];
        eg += KING_EG[side][toK] - KING_EG[side][fromK];

        mg += ROOK_MG[side][toR] - ROOK_MG[side][fromR];
        eg += ROOK_EG[side][toR] - ROOK_EG[side][fromR];

        phase = phase < 24 ? phase : 24;
        return (mg * phase + eg * (24 - phase)) / 24;
    }
}