#ifndef GAME_H
#define GAME_H

#include "MoveGenerator.h"
#include <string>

using namespace defs;
using namespace movegen;
using std::string;

namespace game {
    extern int moveCount;
    inline static MoveInfo movesPlayed[5949];

    extern bstate::BoardState board;

    void printBoard(U64 bitboard);
    void printBoard(BoardState& board);

    void makeMove(MoveInfo& move);
    void unmakeMove();

    void setFen(const char* fen);
    string getFen(); 
}

#endif