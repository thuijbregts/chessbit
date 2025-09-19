#ifndef GAME_H
#define GAME_H

#include "BoardState.h"
#include "MoveInfo.h"
#include <string>

using namespace defs;
using namespace bstate;
using namespace moveinfo;
using std::string;

namespace game {
    extern int count;
    extern MoveInfo* moves[5949];

    void printBoard(U64 bitboard);
    void printBoard(BoardState& board);

    void makeMove(MoveInfo& move);
    void unmakeMove();

    void setFen(const char* fen);
    string getFen(); 
}

#endif