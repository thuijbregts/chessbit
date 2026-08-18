#include "Cui.h"
#include "Uci.h"
#include "Engine.h"
#include "Nnue.h"
#include "Datagen.h"

int main() {
    game::setFen(StartPosition);
    engine::initLmr();
    nnue::loadWeights("quantised.bin");

    /*datagen::Config cfg;
    datagen::run(cfg, game::board);*/

    Cui cui;
}