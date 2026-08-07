#ifndef GAME_H
#define GAME_H

#include "BoardState.h"
#include <string>

using namespace defs;
using namespace bstate;
using std::string;

namespace game {
    extern int moveCount;
    inline static BoardState movesPlayed[5949];

    extern bstate::BoardState board;

    void printBoard(U64 bitboard);
    void printBoard(BoardState& board);

    void makeMove(const BoardState& move);
    void unmakeMove();

    void setFen(const char* fen);
    string getFen();
}

#endif