#include "Cui.h"
#include "TranspositionTable.h"

int main()
{
    tt::init(16384);

    game::setFen(StartPosition);

    Cui cui;
}