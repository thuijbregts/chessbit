#include "MoveGenerator.h"
#include "Cui.h"
#include <chrono>
#include <bitset>

#include <iostream>
#include <fstream>

using namespace std::chrono;

//void printArrayToFile(const char* filename) {
//    FILE* file = std::fopen(filename, "w");
//    if (!file) {
//        perror("Error opening file");
//        return;
//    }
//
//    std::fprintf(file, "static constexpr U64 PAWN_KING_ATTACKS_CASTLE[2][4096] = {\n");
//    for (int i = 0; i < 2; i++) {
//        std::fprintf(file, "{\n");
//        for (int j = 0; j < 4096; j++) {
//            if (j % 32 == 0 && j != 0) std::fprintf(file, "\n");
//            std::fprintf(file, "0x%llx,", game::pawnAttacks[i][j]);
//        }
//        std::fprintf(file, "},\n");
//    }
//    std::fprintf(file, "};\n\n");
//
//    std::fclose(file);
//}

int main()
{
    game::init();

    setFen(StartPosition);

    Cui cui;
}