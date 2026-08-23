#include "Cui.h"
#include "Uci.h"
#include "Engine.h"
#include "Nnue.h"
#include "Datagen.h"

int main() {
    game::setFen(StartPosition);
    engine::initLmr();
    nnue::loadWeights("nnue.bin");

    /*nnue::initRoot(game::board);
    printf("%d\n",  nnue::evaluate<white>(0, game::board));
    nnue::initRoot(game::board);
    printf("%d\n", nnue::evaluate<black>(0, game::board));*/

    /*datagen::Config cfg;
    datagen::run(cfg, game::board);*/

    Cui cui;
}