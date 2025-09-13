#pragma once

#include "Definitions.h"

namespace movegen {
    struct BoardState;
    extern BoardState dummy;
}

namespace moveinfo {
    struct MoveInfo {
        int from;
        int to;

        int promo;

        bool capture;

        movegen::BoardState* board;

        constexpr MoveInfo() : from(0), to(0), promo(0), capture(0), board(&movegen::dummy) { }

        constexpr MoveInfo(int from, int to, int promo, bool capture, movegen::BoardState& board) :
            from(from), to(to), promo(promo), capture(capture), board(&board)
        {

        }

        constexpr MoveInfo(int from, int to, bool capture, movegen::BoardState& board) :
            from(from), to(to), promo(defs::noPiece), capture(capture), board(&board)
        {

        }
    };
}