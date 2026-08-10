#pragma once

#include "Definitions.h"

using namespace defs;

namespace eval {
    ForceInline int score(int mg, int eg, int8_t phase) {
        phase = phase < 24 ? phase : 24;
        return (mg * phase + eg * (24 - phase)) / 24;
    }

    ForceInline void init(U64(&pieces)[2][6], bool side, int& mg, int& eg, int8_t& phase) {
        bool me = (side == white) ? white : black;

        U64 pM = pieces[me][p], nM = pieces[me][n], bM = pieces[me][b];
        U64 rM = pieces[me][r], qM = pieces[me][q], kM = pieces[me][k];
        Bitloop(pM) { int s = SquareOf(pM); mg += VAL_MG[p] + PAWN_MG[s];   eg += VAL_EG[p] + PAWN_EG[s]; }
        Bitloop(nM) { int s = SquareOf(nM); mg += VAL_MG[n] + KNIGHT_MG[s]; eg += VAL_EG[n] + KNIGHT_EG[s]; phase += 1; }
        Bitloop(bM) { int s = SquareOf(bM); mg += VAL_MG[b] + BISHOP_MG[s]; eg += VAL_EG[b] + BISHOP_EG[s]; phase += 1; }
        Bitloop(rM) { int s = SquareOf(rM); mg += VAL_MG[r] + ROOK_MG[s];   eg += VAL_EG[r] + ROOK_EG[s];   phase += 2; }
        Bitloop(qM) { int s = SquareOf(qM); mg += VAL_MG[q] + QUEEN_MG[s];  eg += VAL_EG[q] + QUEEN_EG[s];  phase += 4; }
        mg += KING_MG[SquareOf(kM)];
        eg += KING_EG[SquareOf(kM)];

        U64 pE = pieces[!me][p], nE = pieces[!me][n], bE = pieces[!me][b];
        U64 rE = pieces[!me][r], qE = pieces[!me][q], kE = pieces[!me][k];
        Bitloop(pE) { int s = SquareOf(pE) ^ 56; mg -= VAL_MG[p] + PAWN_MG[s];   eg -= VAL_EG[p] + PAWN_EG[s]; }
        Bitloop(nE) { int s = SquareOf(nE) ^ 56; mg -= VAL_MG[n] + KNIGHT_MG[s]; eg -= VAL_EG[n] + KNIGHT_EG[s]; phase += 1; }
        Bitloop(bE) { int s = SquareOf(bE) ^ 56; mg -= VAL_MG[b] + BISHOP_MG[s]; eg -= VAL_EG[b] + BISHOP_EG[s]; phase += 1; }
        Bitloop(rE) { int s = SquareOf(rE) ^ 56; mg -= VAL_MG[r] + ROOK_MG[s];   eg -= VAL_EG[r] + ROOK_EG[s];   phase += 2; }
        Bitloop(qE) { int s = SquareOf(qE) ^ 56; mg -= VAL_MG[q] + QUEEN_MG[s];  eg -= VAL_EG[q] + QUEEN_EG[s];  phase += 4; }
        mg -= KING_MG[SquareOf(kE) ^ 56];
        eg -= KING_EG[SquareOf(kE) ^ 56];
    }

    template <Piece piece, Piece victim>
    ForceInline void update(int from, int to, int& mg, int& eg, int8_t& phase) noexcept {
        if constexpr (piece == Piece::Pawn)     { mg += PAWN_MG[to] - PAWN_MG[from];        eg += PAWN_EG[to] - PAWN_EG[from]; }
        if constexpr (piece == Piece::Knight)   { mg += KNIGHT_MG[to] - KNIGHT_MG[from];    eg += KNIGHT_EG[to] - KNIGHT_EG[from]; }
        if constexpr (piece == Piece::Bishop)   { mg += BISHOP_MG[to] - BISHOP_MG[from];    eg += BISHOP_EG[to] - BISHOP_EG[from]; }
        if constexpr (piece == Piece::Rook)     { mg += ROOK_MG[to] - ROOK_MG[from];        eg += ROOK_EG[to] - ROOK_EG[from]; }
        if constexpr (piece == Piece::Queen)    { mg += QUEEN_MG[to] - QUEEN_MG[from];      eg += QUEEN_EG[to] - QUEEN_EG[from]; }
        if constexpr (piece == Piece::King)     { mg += KING_MG[to] - KING_MG[from];        eg += KING_EG[to] - KING_EG[from]; }

        if constexpr (victim != Piece::King) {
            if constexpr (victim == Piece::Pawn)    { mg += VAL_MG[p] + PAWN_MG[to ^ 56];    eg += VAL_EG[p] + PAWN_EG[to ^ 56]; }
            if constexpr (victim == Piece::Knight)  { mg += VAL_MG[n] + KNIGHT_MG[to ^ 56];  eg += VAL_EG[n] + KNIGHT_EG[to ^ 56]; phase--; }
            if constexpr (victim == Piece::Bishop)  { mg += VAL_MG[b] + BISHOP_MG[to ^ 56];  eg += VAL_EG[b] + BISHOP_EG[to ^ 56]; phase--; }
            if constexpr (victim == Piece::Rook)    { mg += VAL_MG[r] + ROOK_MG[to ^ 56];    eg += VAL_EG[r] + ROOK_EG[to ^ 56]; phase -= 2; }
            if constexpr (victim == Piece::Queen)   { mg += VAL_MG[q] + QUEEN_MG[to ^ 56];   eg += VAL_EG[q] + QUEEN_EG[to ^ 56]; phase -= 4; }
        }
    }

    template <Piece piece, Piece victim>
    ForceInline void updatePromotion(int from, int to, int& mg, int& eg, int8_t& phase) noexcept {
        mg -= PAWN_MG[from];
        eg -= PAWN_EG[from];

        if constexpr (piece == Piece::Knight)   { mg += KNIGHT_MG[to];    eg += KNIGHT_EG[to]; phase++; }
        if constexpr (piece == Piece::Bishop)   { mg += BISHOP_MG[to];    eg += BISHOP_EG[to]; phase++; }
        if constexpr (piece == Piece::Rook)     { mg += ROOK_MG[to];      eg += ROOK_EG[to]; phase += 2; }
        if constexpr (piece == Piece::Queen)    { mg += QUEEN_MG[to];     eg += QUEEN_EG[to]; phase += 4; }

        if constexpr (victim != Piece::King) {
            if constexpr (victim == Piece::Pawn)    { mg += VAL_MG[p] + PAWN_MG[to ^ 56];    eg += VAL_EG[p] + PAWN_EG[to ^ 56]; }
            if constexpr (victim == Piece::Knight)  { mg += VAL_MG[n] + KNIGHT_MG[to ^ 56];  eg += VAL_EG[n] + KNIGHT_EG[to ^ 56]; phase--; }
            if constexpr (victim == Piece::Bishop)  { mg += VAL_MG[b] + BISHOP_MG[to ^ 56];  eg += VAL_EG[b] + BISHOP_EG[to ^ 56]; phase--; }
            if constexpr (victim == Piece::Rook)    { mg += VAL_MG[r] + ROOK_MG[to ^ 56];    eg += VAL_EG[r] + ROOK_EG[to ^ 56]; phase -= 2; }
            if constexpr (victim == Piece::Queen)   { mg += VAL_MG[q] + QUEEN_MG[to ^ 56];   eg += VAL_EG[q] + QUEEN_EG[to ^ 56]; phase -= 4; }
        }
    }

    ForceInline void updateDoublePush(int from, int to, int& mg, int& eg) noexcept {
        mg += PAWN_MG[to] - PAWN_MG[from];
        eg += PAWN_EG[to] - PAWN_EG[from];
    }

    ForceInline void updateEnPassant(int from, int to, int ePS, int& mg, int& eg) noexcept {
        mg += PAWN_MG[to] - PAWN_MG[from];
        eg += PAWN_EG[to] - PAWN_EG[from];

        mg += VAL_MG[p] + PAWN_MG[ePS ^ 56];
        eg += VAL_EG[p] + PAWN_EG[ePS ^ 56];
    }

    template <int castlingSide>
    ForceInline void updateCastling(int& mg, int& eg) noexcept {
        constexpr int fromK = CASTLING_KING_SOURCE_SQUARE[castlingSide];
        constexpr int toK = CASTLING_KING_TARGET_SQUARE[castlingSide];

        constexpr int fromR = CASTLING_ROOK_SOURCE_SQUARE[castlingSide];
        constexpr int toR = CASTLING_ROOK_TARGET_SQUARE[castlingSide];

        mg += KING_MG[toK] - KING_MG[fromK];
        eg += KING_EG[toK] - KING_EG[fromK];

        mg += ROOK_MG[toR] - ROOK_MG[fromR];
        eg += ROOK_EG[toR] - ROOK_EG[fromR];
    }
}