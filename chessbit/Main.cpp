#include "Cui.h"
#include "Uci.h"
#include "TranspositionTable.h"

int main() {
    game::setFen(StartPosition);

    std::string first;
    std::getline(std::cin, first);
    while (!first.empty() && (first.back() == '\r' || first.back() == ' ')) first.pop_back();

    if (first == "uci") uci::loop();
    else                Cui cui;
}