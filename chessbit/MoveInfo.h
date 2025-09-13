#pragma once

#include "BoardState.h"

namespace moveinfo {
    struct MoveInfo {
        int from;
        int to;

        int promo;

        bool capture;

        bstate::BoardState board;

        constexpr MoveInfo() : from(0), to(0), promo(0), capture(0), board(bstate::dummy) { }

        constexpr MoveInfo(int from, int to, int promo, bool capture, bstate::BoardState& board) :
            from(from), to(to), promo(promo), capture(capture), board(board)
        {

        }

        constexpr MoveInfo(int from, int to, bool capture, bstate::BoardState& board) :
            from(from), to(to), promo(defs::noPiece), capture(capture), board(board)
        {

        }
    };
}