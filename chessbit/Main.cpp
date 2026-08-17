#include "Cui.h"
#include "Uci.h"
#include "Engine.h"
#include "Nnue.h"

int main() {
    game::setFen(StartPosition);
    engine::initLmr();
    nnue::initRandom();

    Cui cui;
}