#pragma once

#include "BoardState.h"

namespace moveinfo {
    enum Type { Normal, EnPassant, Castle, Promotion };

    struct MoveInfo {
        Type type;

        int from;
        int to;

        int promo;

        bool capture;

        bstate::BoardState board;

        constexpr MoveInfo() : type(Normal), from(0), to(0), promo(0), capture(0), board(bstate::dummy) { }

        constexpr MoveInfo(int from, int to, int promo, bool capture, const bstate::BoardState& board) :
            type(Promotion), from(from), to(to), promo(promo), capture(capture), board(board) { }

        constexpr MoveInfo(int from, int to, bool capture, const bstate::BoardState& board) :
            type(Normal), from(from), to(to), promo(defs::noPiece), capture(capture), board(board) { }

        constexpr MoveInfo(Type type, int from, int to, bool capture, const bstate::BoardState& board) :
            type(type), from(from), to(to), promo(defs::noPiece), capture(capture), board(board) {
        }
    };
}