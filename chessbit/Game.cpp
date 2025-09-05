#pragma warning(disable:4146)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <iostream>
#include "MoveGenerator.h"

using std::string;

namespace game {
    MoveInfo PAWN_MOVES[7][64][64];
    MoveInfo KNIGHT_MOVES[7][64][64];
    MoveInfo BISHOP_MOVES[7][64][64];
    MoveInfo ROOK_MOVES[7][64][64];
    MoveInfo QUEEN_MOVES[7][64][64];
    MoveInfo KING_MOVES[7][64][64];
    MoveInfo PROMO_KNIGHT_MOVES[7][64][64];
    MoveInfo PROMO_BISHOP_MOVES[7][64][64];
    MoveInfo PROMO_ROOK_MOVES[7][64][64];
    MoveInfo PROMO_QUEEN_MOVES[7][64][64];

    MoveInfo EN_PASSANT_MOVES[64][64];
    MoveInfo CASTLING_MOVES[64];

    /*U64 bishopPins[5248];
    U64 rookPins[102400];
    U64 queenAttacks[6946816];*/

    /*U64 castleAttacksBishop[2][2][16384];
    U64 castleAttacksRook[2][2][16384];*/

    int moveCount = 0;
    MoveInfo movesPlayed[5949];

    U64 pieces[2][7];
    U64 occupancies[3];
    int boardPieces[65];
    bool side;
    int enPassant;
    int castlingPermissions;
    U64 checks;

    U64 pinMasks[65][64];
    U64 pinRays[64][64];
    U64 potentialPinnerMasks[64][64];
    //U64 validAttacksMasks[5949][64];
    /*U64 pawnAttackZones[2][64][8192];
    U64 pawnZones[2][64];*/
    U64 knightAttackZones[64];
    U64 bishopAttackZones[2][4096];
    U64 rookAttackZones[2][1024];
    int pawnChecks[2][64][64];
    int knightChecks[64][64];

    U64 pawnAttacks[2][4096];

    void init() {
        initMovesInfo();
        initSliderAttacks();
        //initCastleAttacks();
        //initPinMasks();
        //initPotentielPinnerMasks();
        //initAttackZones();
        //initChecks();
    }

    void printBoard(U64 bitboard) {
        for (int rank = 0; rank < 8; rank++)
        {
            for (int file = 0; file < 8; file++)
            {
                int square = rank * 8 + file;
                if (!file)
                    printf("  %d ", 8 - rank);

                printf(" %d", GetBit(bitboard, square) ? 1 : 0);
            }
            printf("\n");
        }
        printf("\n     a b c d e f g h\n\n");
        printf("     bitboard: 0x%llx\n\n", bitboard);
    }

    void printBoard() {
        printf("\n");

        for (int rank = 0; rank < 8; rank++)
        {
            for (int file = 0; file < 8; file++)
            {
                int square = rank * 8 + file;

                if (!file)
                    printf("  %d ", 8 - rank);

                int piece = -1;
                int side;

                for (int bb_piece = p; bb_piece <= k; bb_piece++)
                {
                    // if there is a piece on current square
                    if (GetBit(pieces[white][bb_piece], square)) {
                        piece = bb_piece;
                        side = white;
                    }
                    else if (GetBit(pieces[black][bb_piece], square)) {
                        piece = bb_piece;
                        side = black;
                    }
                }

                printf(" %c", (piece == -1) ? '.' : ASCII_PIECES[side][piece]);
            }

            printf("\n");
        }

        printf("\n     a b c d e f g h\n\n");

        printf("     Side:     %s\n", !side ? "white" : "black");

        printf("     En Passant:  %s\n", (enPassant != noSquare) ? SQUARE_NAMES[enPassant] : "no");

        printf("     Castling:  %c%c%c%c\n\n", (castlingPermissions & wk) ? 'K' : '-',
            (castlingPermissions & wq) ? 'Q' : '-',
            (castlingPermissions & bk) ? 'k' : '-',
            (castlingPermissions & bq) ? 'q' : '-');
    }

    void printPieces() {
        for (int rank = 0; rank < 8; rank++)
        {
            for (int file = 0; file < 8; file++)
            {
                int square = rank * 8 + file;
                if (!file)
                    printf("  %d ", 8 - rank);

                printf(" %d", boardPieces[square]);
            }
            printf("\n");
        }
        printf("\n     a b c d e f g h\n\n");
    }

    void initMovesInfo() {
        int deadPiece, from, to;

        //QUIET MOVES
        for (from = 0; from < 64; from++) {
            for (to = 0; to < 64; to++) {
                initMoveInfo(PAWN_MOVES[noPiece][from][to], movegen::Pawn, from, to, p, noSquare, noPiece, noPiece, 0, 0, 0);
                initMoveInfo(KNIGHT_MOVES[noPiece][from][to], movegen::Knight, from, to, n, noSquare, noPiece, noPiece, 0, 0, 0);
                initMoveInfo(BISHOP_MOVES[noPiece][from][to], movegen::Bishop, from, to, b, noSquare, noPiece, noPiece, 0, 0, 0);
                initMoveInfo(ROOK_MOVES[noPiece][from][to], movegen::Rook, from, to, r, noSquare, noPiece, noPiece, 0, 0, 0);
                initMoveInfo(QUEEN_MOVES[noPiece][from][to], movegen::Queen, from, to, q, noSquare, noPiece, noPiece, 0, 0, 0);
                initMoveInfo(KING_MOVES[noPiece][from][to], movegen::King, from, to, k, noSquare, noPiece, noPiece, 0, 0, 0);
                initMoveInfo(PROMO_KNIGHT_MOVES[noPiece][from][to], movegen::PromotionKnight, from, to, p, noSquare, noPiece, n, 0, 0, 0);
                initMoveInfo(PROMO_BISHOP_MOVES[noPiece][from][to], movegen::PromotionBishop, from, to, p, noSquare, noPiece, b, 0, 0, 0);
                initMoveInfo(PROMO_ROOK_MOVES[noPiece][from][to], movegen::PromotionRook, from, to, p, noSquare, noPiece, r, 0, 0, 0);
                initMoveInfo(PROMO_QUEEN_MOVES[noPiece][from][to], movegen::PromotionQueen, from, to, p, noSquare, noPiece, q, 0, 0, 0);
            }
        }

        //CAPTURE MOVES
        for (deadPiece = p; deadPiece <= q; deadPiece++) {
            for (from = 0; from < 64; from++) {
                for (to = 0; to < 64; to++) {
                    initMoveInfo(PAWN_MOVES[deadPiece][from][to], movegen::PawnCapture, from, to, p, noSquare, deadPiece, noPiece, 0, 0, (MVV_LVA[deadPiece] + 6) - (MVV_LVA[p] / 100));
                    initMoveInfo(KNIGHT_MOVES[deadPiece][from][to], movegen::KnightCapture, from, to, n, noSquare, deadPiece, noPiece, 0, 0, (MVV_LVA[deadPiece] + 6) - (MVV_LVA[n] / 100));
                    initMoveInfo(BISHOP_MOVES[deadPiece][from][to], movegen::BishopCapture, from, to, b, noSquare, deadPiece, noPiece, 0, 0, (MVV_LVA[deadPiece] + 6) - (MVV_LVA[b] / 100));
                    initMoveInfo(ROOK_MOVES[deadPiece][from][to], movegen::RookCapture, from, to, r, noSquare, deadPiece, noPiece, 0, 0, (MVV_LVA[deadPiece] + 6) - (MVV_LVA[r] / 100));
                    initMoveInfo(QUEEN_MOVES[deadPiece][from][to], movegen::QueenCapture, from, to, q, noSquare, deadPiece, noPiece, 0, 0, (MVV_LVA[deadPiece] + 6) - (MVV_LVA[q] / 100));
                    initMoveInfo(KING_MOVES[deadPiece][from][to], movegen::KingCapture, from, to, k, noSquare, deadPiece, noPiece, 0, 0, (MVV_LVA[deadPiece] + 6) - (MVV_LVA[k] / 100));
                    initMoveInfo(PROMO_KNIGHT_MOVES[deadPiece][from][to], movegen::PromotionKnightCapture, from, to, p, noSquare, deadPiece, n, 0, 0, (MVV_LVA[deadPiece] + 6) - (MVV_LVA[p] / 100));
                    initMoveInfo(PROMO_BISHOP_MOVES[deadPiece][from][to], movegen::PromotionBishopCapture, from, to, p, noSquare, deadPiece, b, 0, 0, (MVV_LVA[deadPiece] + 6) - (MVV_LVA[p] / 100));
                    initMoveInfo(PROMO_ROOK_MOVES[deadPiece][from][to], movegen::PromotionRookCapture, from, to, p, noSquare, deadPiece, r, 0, 0, (MVV_LVA[deadPiece] + 6) - (MVV_LVA[p] / 100));
                    initMoveInfo(PROMO_QUEEN_MOVES[deadPiece][from][to], movegen::PromotionQueenCapture, from, to, p, noSquare, deadPiece, q, 0, 0, (MVV_LVA[deadPiece] + 6) - (MVV_LVA[p] / 100));
                }
            }
        }

        //DOUBLE PAWN MOVES
        for (from = a2; from <= h2; from++) {
            to = from + (PAWN_PUSH[white] * 2);
            int enPassant = from + PAWN_PUSH[white];
            initMoveInfo(PAWN_MOVES[noPiece][from][to], movegen::Pawn, from, to, p, enPassant, noPiece, noPiece, 0, 0, 0);
        }

        for (from = a7; from <= h7; from++) {
            to = from + (PAWN_PUSH[black] * 2);
            int enPassant = from + PAWN_PUSH[black];
            initMoveInfo(PAWN_MOVES[noPiece][from][to], movegen::Pawn, from, to, p, enPassant, noPiece, noPiece, 0, 0, 0);
        }

        //EN PASSANT MOVES
        for (from = a5; from <= h5; from++) {
            if (from != a5) initMoveInfo(EN_PASSANT_MOVES[from][from - 9], movegen::EnPassant, from, from - 9, p, noSquare, p, noPiece, 0, 0, 105);
            if (from != h5) initMoveInfo(EN_PASSANT_MOVES[from][from - 7], movegen::EnPassant, from, from - 7, p, noSquare, p, noPiece, 0, 0, 105);
        }
        for (from = a4; from <= h4; from++) {
            if (from != a4) initMoveInfo(EN_PASSANT_MOVES[from][from + 7], movegen::EnPassant, from, from + 7, p, noSquare, p, noPiece, 0, 0, 105);
            if (from != h4) initMoveInfo(EN_PASSANT_MOVES[from][from + 9], movegen::EnPassant, from, from + 9, p, noSquare, p, noPiece, 0, 0, 105);
        }

        //CASTLING MOVES
        initMoveInfo(CASTLING_MOVES[g1], movegen::Castling, e1, g1, k, noSquare, noPiece, noPiece, h1, f1, 0);
        initMoveInfo(CASTLING_MOVES[c1], movegen::Castling, e1, c1, k, noSquare, noPiece, noPiece, a1, d1, 0);
        initMoveInfo(CASTLING_MOVES[g8], movegen::Castling, e8, g8, k, noSquare, noPiece, noPiece, h8, f8, 0);
        initMoveInfo(CASTLING_MOVES[c8], movegen::Castling, e8, c8, k, noSquare, noPiece, noPiece, a8, d8, 0);
    }

    void initMoveInfo(MoveInfo& move, int type, int from, int to, int movedPiece, int enPassant, int deadPiece, int promotedPiece, int sourceRook, int targetRook, int mvvLva) {
        move.type = type;
        move.from = from;
        move.to = to;
        move.movedPiece = movedPiece;
        move.enPassant = enPassant;
        move.deadPiece = deadPiece;
        move.promotedPiece = promotedPiece;
        move.rookFrom = sourceRook;
        move.rookTo = targetRook;
        move.mvv_lva = mvvLva;
    }

    U64 setOccupancy(int index, int bitsInMask, U64 mask) {
        U64 occupancy = 0ULL;

        for (int count = 0; count < bitsInMask; count++)
        {
            int square = SquareOf(mask);
            PopBit(mask, square);
            if (index & (1 << count)) {
                occupancy |= (1ULL << square);
            }
        }

        return occupancy;
    }

    U64 getBishopXRays(int square) {
        U64 attacks = 0ULL;
        int f, r;
        int tr = square / 8;
        int tf = square % 8;

        for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) attacks |= (1ULL << (r * 8 + f));
        for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--) attacks |= (1ULL << (r * 8 + f));
        for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) attacks |= (1ULL << (r * 8 + f));
        for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) attacks |= (1ULL << (r * 8 + f));

        return attacks;
    }

    int getBishopPinOffset(int square) {
        int offset = 0;

        if (square > 0) {
            //offset = BISHOP_PIN_OFFSETS[square - 1] + pow(2, BISHOP_PINS_OCCUPANCY_BITS[square - 1]);
        }

        return offset;
    }

    U64 getRookXRays(int square) {
        U64 attacks = 0ULL;
        int f, r;
        int tr = square / 8;
        int tf = square % 8;

        for (r = tr + 1; r <= 7; r++) attacks |= (1ULL << (r * 8 + tf));
        for (r = tr - 1; r >= 0; r--) attacks |= (1ULL << (r * 8 + tf));
        for (f = tf + 1; f <= 7; f++) attacks |= (1ULL << (tr * 8 + f));
        for (f = tf - 1; f >= 0; f--) attacks |= (1ULL << (tr * 8 + f));

        return attacks;
    }

    int getRookPinOffset(int square) {
        int offset = 0;

        if (square > 0) {
            //offset = ROOK_PIN_OFFSETS[square - 1] + pow(2, ROOK_PINS_OCCUPANCY_BITS);
        }

        return offset;
    }

    U64 bishopPinsWithOccupancy(int square, U64 occupancy) {
        U64 pins = 0ULL;

        int f, r;
        int tr = square / 8;
        int tf = square % 8;

        int bitFound = 0;
        for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++)
        {
            if ((occupancy & (1ULL << (r * 8 + f))) || EDGES[(r * 8 + f)]) {
                bitFound++;
                if (bitFound == 2) {
                    pins |= (1ULL << (r * 8 + f));
                    break;
                }
            }
        }
        bitFound = 0;
        for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--)
        {
            if ((occupancy & (1ULL << (r * 8 + f))) || EDGES[(r * 8 + f)]) {
                bitFound++;
                if (bitFound == 2) {
                    pins |= (1ULL << (r * 8 + f));
                    break;
                }
            }
        }
        bitFound = 0;
        for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++)
        {
            if ((occupancy & (1ULL << (r * 8 + f))) || EDGES[(r * 8 + f)]) {
                bitFound++;
                if (bitFound == 2) {
                    pins |= (1ULL << (r * 8 + f));
                    break;
                }
            }
        }
        bitFound = 0;
        for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--)
        {
            if ((occupancy & (1ULL << (r * 8 + f))) || EDGES[(r * 8 + f)]) {
                bitFound++;
                if (bitFound == 2) {
                    pins |= (1ULL << (r * 8 + f));
                    break;
                }
            }
        }

        return pins;
    }

    U64 rookPinsWithOccupancy(int square, U64 occupancy) {
        U64 attacks = 0ULL;

        int f, r;
        int tr = square / 8;
        int tf = square % 8;

        int bitFound = 0;
        for (r = tr + 1; r <= 7; r++)
        {
            if ((occupancy & (1ULL << (r * 8 + tf))) || RANKS[(r * 8 + tf)] == 1) {
                bitFound++;
                if (bitFound == 2) {
                    attacks |= (1ULL << (r * 8 + tf));
                    break;
                }
            }
        }

        bitFound = 0;
        for (r = tr - 1; r >= 0; r--)
        {
            if ((occupancy & (1ULL << (r * 8 + tf))) || RANKS[(r * 8 + tf)] == 8) {
                bitFound++;
                if (bitFound == 2) {
                    attacks |= (1ULL << (r * 8 + tf));
                    break;
                }
            }
        }
        bitFound = 0;
        for (f = tf + 1; f <= 7; f++)
        {
            if ((occupancy & (1ULL << (tr * 8 + f))) || FILES[(tr * 8 + f)] == 8) {
                bitFound++;
                if (bitFound == 2) {
                    attacks |= (1ULL << (tr * 8 + f));
                    break;
                }
            }
        }
        bitFound = 0;
        for (f = tf - 1; f >= 0; f--)
        {
            if ((occupancy & (1ULL << (tr * 8 + f))) || FILES[(tr * 8 + f)] == 1) {
                bitFound++;
                if (bitFound == 2) {
                    attacks |= (1ULL << (tr * 8 + f));
                    break;
                }
            }
        }

        return attacks;
    }

    U64 queenAttacksWithOccupancy(int square, U64 occupancy) {
        U64 attacks = 0ULL;

        int f, r;
        int tr = square / 8;
        int tf = square % 8;

        for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++)
        {
            attacks |= (1ULL << (r * 8 + f));
            if (occupancy & (1ULL << (r * 8 + f))) {
                break;
            }
        }
        for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--)
        {
            attacks |= (1ULL << (r * 8 + f));
            if (occupancy & (1ULL << (r * 8 + f))) {
                break;
            }
        }
        for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++)
        {
            attacks |= (1ULL << (r * 8 + f));
            if (occupancy & (1ULL << (r * 8 + f))) {
                break;
            }
        }
        for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--)
        {
            attacks |= (1ULL << (r * 8 + f));
            if (occupancy & (1ULL << (r * 8 + f))) {
                break;
            }
        }

        for (r = tr + 1; r <= 7; r++)
        {
            attacks |= (1ULL << (r * 8 + tf));
            if (occupancy & (1ULL << (r * 8 + tf))) {
                break;
            }
        }

        for (r = tr - 1; r >= 0; r--)
        {
            attacks |= (1ULL << (r * 8 + tf));
            if (occupancy & (1ULL << (r * 8 + tf))) {
                break;
            }
        }
        for (f = tf + 1; f <= 7; f++)
        {
            attacks |= (1ULL << (tr * 8 + f));
            if (occupancy & (1ULL << (tr * 8 + f))) {
                break;
            }
        }
        for (f = tf - 1; f >= 0; f--)
        {
            attacks |= (1ULL << (tr * 8 + f));
            if (occupancy & (1ULL << (tr * 8 + f))) {
                break;
            }
        }

        return attacks;
    }

    void initSliderAttacks() {
        initPawnAttacks(white);
        initPawnAttacks(black);
        for (int square = 0; square < 64; square++) {
            initBishopPins(square);
            initRookPins(square);
            //initQueenAttacks(square);
        }
    }

    void initPawnAttacks(bool side) {
        U64 mask = PAWN_KING_MASK_CASTLE[side];
        //int offset = PAWN_OFFSETS[side * 64 + square];
        int bitCount = Bitcount(mask);
        int occupancyVariations = 1 << bitCount;

        for (int count = 0; count < occupancyVariations; count++) {
            U64 occupancy = setOccupancy(count, bitCount, mask);
            pawnAttacks[side][_pext_u64(occupancy, mask)] = pawnAttacksWithOccupancy(side, occupancy);
        }
    }

    U64 pawnAttacksWithOccupancy(bool side, U64 occupancy) {
        U64 attacks = 0ULL;
        Bitloop(occupancy) {
            int square = SquareOf(occupancy);

            attacks |= PAWN_CAPTURES[!side][square];
        }
        return ~attacks;
    }

    void initBishopPins(int square) {
        U64 mask = BISHOP_MASKS[square];
        //int offset = BISHOP_OFFSETS[square];
        int bitCount = Bitcount(mask);
        int occupancyVariations = 1 << bitCount;

        for (int count = 0; count < occupancyVariations; count++) {
            //U64 occupancy = setOccupancy(count, bitCount, mask);
            //bishopPins[offset + _pext_u64(occupancy, mask)] = bishopPinsWithOccupancy(square, occupancy);
        }
    }

    void initRookPins(int square) {
        U64 mask = ROOK_MASKS[square];
        //int offset = ROOK_OFFSETS[square];
        int bitCount = Bitcount(mask);
        int occupancyVariations = 1 << bitCount;

        for (int count = 0; count < occupancyVariations; count++) {
            //U64 occupancy = setOccupancy(count, bitCount, mask);
            //rookPins[offset + _pext_u64(occupancy, mask)] = rookPinsWithOccupancy(square, occupancy);
        }
    }

    void initQueenAttacks(int square) {
        U64 mask = QUEEN_MASKS[square];
        //int offset = QUEEN_OFFSETS[square];
        int bitCount = Bitcount(mask);
        int occupancyVariations = 1 << bitCount;

        for (int count = 0; count < occupancyVariations; count++) {
            //U64 occupancy = setOccupancy(count, bitCount, mask);
            //queenAttacks[offset + _pext_u64(occupancy, mask)] = queenAttacksWithOccupancy(square, occupancy);
        }
    }

    U64 castleBishopWithOccupancy(int square, U64 occupancy) {
        U64 attacks = 0ULL;

        int f, r;
        int tr = square / 8;
        int tf = square % 8;

        for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++)
        {
            if (occupancy & (1ULL << (r * 8 + f))) {
                attacks |= (1ULL << (r * 8 + f));
                break;
            }
        }

        for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--)
        {
            if (occupancy & (1ULL << (r * 8 + f))) {
                attacks |= (1ULL << (r * 8 + f));
                break;
            }
        }

        for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++)
        {
            if (occupancy & (1ULL << (r * 8 + f))) {
                attacks |= (1ULL << (r * 8 + f));
                break;
            }
        }

        for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--)
        {
            if (occupancy & (1ULL << (r * 8 + f))) {
                attacks |= (1ULL << (r * 8 + f));
                break;
            }
        }

        return attacks;
    }

    U64 castleRookWithOccupancy(int square, U64 occupancy) {
        U64 attacks = 0ULL;

        int f, r;
        int tr = square / 8;
        int tf = square % 8;

        for (r = tr + 1; r <= 7; r++)
        {
            if (occupancy & (1ULL << (r * 8 + tf))) {
                attacks |= (1ULL << (r * 8 + tf));
                break;
            }
        }

        for (r = tr - 1; r >= 0; r--)
        {
            if (occupancy & (1ULL << (r * 8 + tf))) {
                attacks |= (1ULL << (r * 8 + tf));
                break;
            }
        }

        for (f = tf + 1; f <= 7; f++)
        {
            if (occupancy & (1ULL << (tr * 8 + f))) {
                attacks |= (1ULL << (tr * 8 + f));
                break;
            }
        }

        for (f = tf - 1; f >= 0; f--)
        {
            if (occupancy & (1ULL << (tr * 8 + f))) {
                attacks |= (1ULL << (tr * 8 + f));
                break;
            }
        }

        return attacks;
    }

    void initCastleAttacks() {
        initCastleAttacksBishop(white, CASTLE_K);
        initCastleAttacksBishop(white, CASTLE_Q);
        initCastleAttacksBishop(black, CASTLE_K);
        initCastleAttacksBishop(black, CASTLE_Q);

        initCastleAttacksRook(white, CASTLE_K);
        initCastleAttacksRook(white, CASTLE_Q);
        initCastleAttacksRook(black, CASTLE_K);
        initCastleAttacksRook(black, CASTLE_Q);
    }

    void initCastleAttacksBishop(int side, int castlingSide) {
        U64 mask = CASTLE_MASKS_BISHOP[side * 2 + castlingSide];
        int bitCount = Bitcount(mask);
        int occupancyVariations = 1 << bitCount;

        //int square1 = CASTLING_ATTACK_SQUARES[side * 2 + castlingSide][0];
        //int square2 = CASTLING_ATTACK_SQUARES[side * 2 + castlingSide][1];

        for (int count = 0; count < occupancyVariations; count++) {
            //U64 occupancy = setOccupancy(count, bitCount, mask);
            //castleAttacksBishop[side][castlingSide][_pext_u64(occupancy, mask)] = castleBishopWithOccupancy(square1, occupancy);
            //castleAttacksBishop[side][castlingSide][_pext_u64(occupancy, mask)] |= castleBishopWithOccupancy(square2, occupancy);
        }
    }

    void initCastleAttacksRook(int side, int castlingSide) {
        U64 mask = CASTLE_MASKS_ROOK[side * 2 + castlingSide];
        int bitCount = Bitcount(mask);
        int occupancyVariations = 1 << bitCount;

        //int square1 = CASTLING_ATTACK_SQUARES[side * 2 + castlingSide][0];
        //int square2 = CASTLING_ATTACK_SQUARES[side * 2 + castlingSide][1];

        for (int count = 0; count < occupancyVariations; count++) {
            //U64 occupancy = setOccupancy(count, bitCount, mask);
            //castleAttacksRook[side][castlingSide][_pext_u64(occupancy, mask)] = castleRookWithOccupancy(square1, occupancy);
            //castleAttacksRook[side][castlingSide][_pext_u64(occupancy, mask)] |= castleRookWithOccupancy(square2, occupancy);
        }
    }

    static inline void initPinMasks() {
        int currentIndex, endIndex, bitShift;
        U64 pinMask, pinRay;
        for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 64; j++) {
                pinMask = 0ULL;
                pinRay = 0ULL;
                bitShift = 0;
                //bit shifts for horizontal: 1
                if (RANKS[i] == RANKS[j]) {
                    bitShift = 1;
                }
                //bit shifts for vertical: 8
                else if (FILES[i] == FILES[j]) {
                    bitShift = 8;
                }
                //bit shifts for diagonals: 7, 9
                else {
                    int x0 = RANKS[i];
                    int x1 = RANKS[j];
                    int y0 = FILES[i];
                    int y1 = FILES[j];

                    if (std::abs(x0 - x1) == std::abs(y0 - y1)) {
                        if ((i - j) % 9 == 0) {
                            bitShift = 9;
                        }
                        else {
                            bitShift = 7;
                        }
                    }
                }

                if (bitShift) {
                    if (i < j) {
                        currentIndex = i + bitShift;
                        endIndex = j - bitShift;
                    }
                    else {
                        currentIndex = j + bitShift;
                        endIndex = i - bitShift;
                    }
                    while (currentIndex <= endIndex) {
                        SetBit(pinMask, currentIndex);
                        currentIndex += bitShift;
                    }

                    if (bitShift == 9) {
                        SetBit(pinRay, i);
                        currentIndex = i;
                        while (currentIndex >= 0) {
                            SetBit(pinRay, currentIndex);
                            if (FILES[currentIndex] == 1 || RANKS[currentIndex] == 8) {
                                break;
                            }
                            currentIndex -= bitShift;
                        }
                        currentIndex = i;
                        while (currentIndex < 64) {
                            SetBit(pinRay, currentIndex);
                            if (FILES[currentIndex] == 8 || RANKS[currentIndex] == 1) {
                                break;
                            }
                            currentIndex += bitShift;
                        }
                    }
                    else if (bitShift == 7) {
                        SetBit(pinRay, i);
                        currentIndex = i;
                        while (currentIndex >= 0) {
                            SetBit(pinRay, currentIndex);
                            if (FILES[currentIndex] == 8 || RANKS[currentIndex] == 8) {
                                break;
                            }
                            currentIndex -= bitShift;
                        }
                        currentIndex = i;
                        while (currentIndex < 64) {
                            SetBit(pinRay, currentIndex);
                            if (FILES[currentIndex] == 1 || RANKS[currentIndex] == 1) {
                                break;
                            }
                            currentIndex += bitShift;
                        }
                    }
                    else if (bitShift == 1) {
                        SetBit(pinRay, i);
                        currentIndex = i;
                        while (currentIndex >= 0) {
                            SetBit(pinRay, currentIndex);
                            if (FILES[currentIndex] == 1) {
                                break;
                            }
                            currentIndex -= bitShift;
                        }
                        currentIndex = i;
                        while (currentIndex < 64) {
                            SetBit(pinRay, currentIndex);
                            if (FILES[currentIndex] == 8) {
                                break;
                            }
                            currentIndex += bitShift;
                        }
                    }
                    else {
                        SetBit(pinRay, i);
                        currentIndex = i;
                        while (currentIndex >= 0) {
                            SetBit(pinRay, currentIndex);
                            if (RANKS[currentIndex] == 8) {
                                break;
                            }
                            currentIndex -= bitShift;
                        }
                        currentIndex = i;
                        while (currentIndex < 64) {
                            SetBit(pinRay, currentIndex);
                            if (RANKS[currentIndex] == 1) {
                                break;
                            }
                            currentIndex += bitShift;
                        }
                    }
                }

                pinMasks[i][j] = pinMask;
                pinRays[i][j] = pinRay;
            }
        }
        for (int i = 0; i < 64; i++) {
            pinMasks[64][i] = 0ULL;
        }
    }

    void initPotentielPinnerMasks() {
        int currentIndex, bitShift;
        U64 mask;

        for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 64; j++) {
                mask = 0ULL;

                bitShift = 0;
                //bit shifts for horizontal: 1
                if (RANKS[i] == RANKS[j]) {
                    if (FILES[i] > 1 && FILES[i] < 8) {
                        bitShift = 1;
                    }
                }
                //bit shifts for vertical: 8
                else if (FILES[i] == FILES[j]) {
                    if (RANKS[i] > 1 && RANKS[i] < 8) {
                        bitShift = 8;
                    }
                }
                //bit shifts for diagonals: 7, 9
                else if (!EDGES[i]) {
                    int x0 = RANKS[i];
                    int x1 = RANKS[j];
                    int y0 = FILES[i];
                    int y1 = FILES[j];

                    if (std::abs(x0 - x1) == std::abs(y0 - y1)) {
                        if ((i - j) % 9 == 0) {
                            bitShift = 9;
                        }
                        else {
                            bitShift = 7;
                        }
                    }
                }

                if (bitShift) {
                    if (i < j) {
                        bitShift = -bitShift;
                    }
                    currentIndex = i + bitShift;

                    if (bitShift % 9 == 0 || bitShift % 7 == 0) {
                        while (!EDGES[currentIndex]) {
                            SetBit(mask, currentIndex);
                            currentIndex += bitShift;
                        }
                    }
                    else if (bitShift % 8 == 0) {
                        while (INNER_RANKS[currentIndex]) {
                            SetBit(mask, currentIndex);
                            currentIndex += bitShift;
                        }
                    }
                    else {
                        while (INNER_FILES[currentIndex]) {
                            SetBit(mask, currentIndex);
                            currentIndex += bitShift;
                        }
                    }
                    SetBit(mask, currentIndex);
                }

                potentialPinnerMasks[i][j] = mask;
            }
        }
    }

    void initPawnZones() {
        for (int square = 0; square < 64; square++) {
            U64 whiteZone = 0ULL, blackZone = 0ULL;

            for (int i = -2; i <= 2; i++) {
                int currentSquare = square + (i * 8);
                if (currentSquare < 0 || currentSquare > 63) {
                    continue;
                }
                if (RANKS[currentSquare] <= RANKS[square] && RANKS[currentSquare] < 8) SetBit(whiteZone, currentSquare);
                if (RANKS[currentSquare] >= RANKS[square] && RANKS[currentSquare] > 1) SetBit(blackZone, currentSquare);
                if (currentSquare - 2 >= 0 && FILES[currentSquare - 2] < 7) {
                    if (RANKS[currentSquare] <= RANKS[square] && RANKS[currentSquare] < 8) SetBit(whiteZone, (currentSquare - 2));
                    if (RANKS[currentSquare] >= RANKS[square] && RANKS[currentSquare] > 1) SetBit(blackZone, (currentSquare - 2));
                }
                if (currentSquare - 1 >= 0 && FILES[square - 1] < 8) {
                    if (i != 1 && RANKS[currentSquare] <= RANKS[square] && RANKS[currentSquare] < 8) SetBit(whiteZone, (currentSquare - 1));
                    if (i != -1 && RANKS[currentSquare] >= RANKS[square] && RANKS[currentSquare] > 1) SetBit(blackZone, (currentSquare - 1));
                }
                if (currentSquare + 1 < 64 && FILES[currentSquare + 1] > 1) {
                    if (i != 1 && RANKS[currentSquare] <= RANKS[square] && RANKS[currentSquare] < 8) SetBit(whiteZone, (currentSquare + 1));
                    if (i != -1 && RANKS[currentSquare] >= RANKS[square] && RANKS[currentSquare] > 1) SetBit(blackZone, (currentSquare + 1));
                }
                if (currentSquare + 2 < 64 && FILES[currentSquare + 2] > 2) {
                    if (RANKS[currentSquare] <= RANKS[square] && RANKS[currentSquare] < 8) SetBit(whiteZone, (currentSquare + 2));
                    if (RANKS[currentSquare] >= RANKS[square] && RANKS[currentSquare] > 1) SetBit(blackZone, (currentSquare + 2));
                }
            }
            //pawnZones[white][square] = whiteZone;
            //pawnZones[black][square] = blackZone;

            std::vector<int> bitPositionsWhite;
            std::vector<int> bitPositionsBlack;
            for (int i = 0; i < 64; ++i) {
                if (whiteZone & (1ULL << i)) {
                    bitPositionsWhite.push_back(i);
                }
                if (blackZone & (1ULL << i)) {
                    bitPositionsBlack.push_back(i);
                }
            }

            int combinationsWhite = pow(2, Bitcount(whiteZone));
            int combinationsBlack = pow(2, Bitcount(blackZone));
            for (int i = 0; i < combinationsWhite; i++) {
                U64 attacks = whiteZone;

                for (int pos : bitPositionsWhite) {
                    attacks &= ~(1ULL << pos);
                }

                for (int j = 0; j < Bitcount(whiteZone); ++j) {
                    if (i & (1 << j)) {
                        attacks |= (1ULL << bitPositionsWhite[j]);
                    }
                }

                U64 attacksLoop = attacks;
                Bitloop(attacksLoop) {
                    //int attackSquare = SquareOf(attacksLoop);
                    //pawnAttackZones[white][square][i] |= PAWN_CAPTURES[white][attackSquare];
                }
                //pawnAttackZones[white][square][i] = ~pawnAttackZones[white][square][i];
            }
            for (int i = 0; i < combinationsBlack; i++) {
                U64 attacks = blackZone;

                for (int pos : bitPositionsBlack) {
                    attacks &= ~(1ULL << pos);
                }

                for (int j = 0; j < Bitcount(blackZone); ++j) {
                    if (i & (1 << j)) {
                        attacks |= (1ULL << bitPositionsBlack[j]);
                    }
                }

                U64 attacksLoop = attacks;
                Bitloop(attacksLoop) {
                    //int attackSquare = SquareOf(attacksLoop);
                    //pawnAttackZones[black][square][i] |= PAWN_CAPTURES[black][attackSquare];
                }
                //pawnAttackZones[black][square][i] = ~pawnAttackZones[black][square][i];
            }
        }
    }

    void initAttackZones() {
        //initPawnZones();
        for (int square = 0; square < 64; square++) {
            U64 attacks = getKingAttacks(square);
            if (square == e1) {
                initBishopZone(square, white, attacks);
                initRookZone(square, white, attacks);
            }
            if (square == e8) {
                initBishopZone(square, black, attacks);
                initRookZone(square, black, attacks);
            }
            Bitloop(attacks) {
                int attackSquare = SquareOf(attacks);

                knightAttackZones[square] |= getKnightAttacks(attackSquare);
            }
        }
    }

    void initBishopZone(int square, bool side, U64 mask) {
        mask = BISHOP_ATTACK_ZONE_CASTLE_MASK[side];
        int bitCount = Bitcount(mask);
        int occupancyVariations = 1 << bitCount;

        for (int count = 0; count < occupancyVariations; count++) {
            U64 occupancy = setOccupancy(count, bitCount, mask);
            bishopAttackZones[side][_pext_u64(occupancy, mask)] = bishopZoneWithOccupancy(square, side, occupancy);
        }
    }

    void initRookZone(int square, bool side, U64 mask) {
        mask = ROOK_ATTACK_ZONE_CASTLE_MASK[side];
        int bitCount = Bitcount(mask);
        int occupancyVariations = 1 << bitCount;

        for (int count = 0; count < occupancyVariations; count++) {
            U64 occupancy = setOccupancy(count, bitCount, mask);
            rookAttackZones[side][_pext_u64(occupancy, mask)] = rookZoneWithOccupancy(square, side, occupancy);
        }
    }

    U64 bishopZoneWithOccupancy(int square, bool side, U64 occupancy) {
        U64 attacks = 0ULL;

        U64 emptySquares = ~occupancy & KING_ATTACKS[square];

        int f, r;
        int sq;
        Bitloop(emptySquares) {
            sq = SquareOf(emptySquares);

            SetBit(attacks, sq);

            
            int tr = sq / 8;
            int tf = sq % 8;

            for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++)
            {
                if (occupancy & (1ULL << (r * 8 + f))) {
                    break;
                }
                attacks |= (1ULL << (r * 8 + f));
            }

            for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--)
            {
                if (occupancy & (1ULL << (r * 8 + f))) {
                    break;
                }
                attacks |= (1ULL << (r * 8 + f));
            }

            for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++)
            {
                if (occupancy & (1ULL << (r * 8 + f))) {
                    break;
                }
                attacks |= (1ULL << (r * 8 + f));
            }

            for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--)
            {
                if (occupancy & (1ULL << (r * 8 + f))) {
                    break;
                }
                attacks |= (1ULL << (r * 8 + f));
            }
        }

        if (side == black) {
            for (int i = 2; i < 4; i++) {
                if (occupancy & CASTLING_PASSING_SQUARES[i]) {
                    continue;
                }
                for (int j = 0; j < 2; j++) {
                    sq = CASTLING_ATTACK_SQUARES[i][j];
                    int tr = sq / 8;
                    int tf = sq % 8;

                    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++)
                    {
                        if (occupancy & (1ULL << (r * 8 + f))) {
                            break;
                        }
                        attacks |= (1ULL << (r * 8 + f));
                    }

                    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--)
                    {
                        if (occupancy & (1ULL << (r * 8 + f))) {
                            break;
                        }
                        attacks |= (1ULL << (r * 8 + f));
                    }
                }
            }
        }
        else {
            for (int i = 0; i < 2; i++) {
                if (occupancy & CASTLING_PASSING_SQUARES[i]) {
                    continue;
                }
                for (int j = 0; j < 2; j++) {
                    sq = CASTLING_ATTACK_SQUARES[i][j];
                    int tr = sq / 8;
                    int tf = sq % 8;

                    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++)
                    {
                        if (occupancy & (1ULL << (r * 8 + f))) {
                            break;
                        }
                        attacks |= (1ULL << (r * 8 + f));
                    }

                    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--)
                    {
                        if (occupancy & (1ULL << (r * 8 + f))) {
                            break;
                        }
                        attacks |= (1ULL << (r * 8 + f));
                    }
                }
            }
        }
        return attacks;
    }

    U64 rookZoneWithOccupancy(int square, bool side, U64 occupancy) {
        U64 attacks = 0ULL;

        U64 emptySquares = ~occupancy & KING_ATTACKS[square];

        int f, r, tr, tf;
        int sq;
        Bitloop(emptySquares) {
            sq = SquareOf(emptySquares);

            SetBit(attacks, sq);

            tr = sq / 8;
            tf = sq % 8;

            for (r = tr + 1; r <= 7; r++)
            {
                if (occupancy & (1ULL << (r * 8 + tf))) {
                    break;
                }
                attacks |= (1ULL << (r * 8 + tf));
            }

            for (r = tr - 1; r >= 0; r--)
            {
                if (occupancy & (1ULL << (r * 8 + tf))) {
                    break;
                }
                attacks |= (1ULL << (r * 8 + tf));
            }

            for (f = tf + 1; f <= 7; f++)
            {
                if (occupancy & (1ULL << (tr * 8 + f))) {
                    break;
                }
                attacks |= (1ULL << (tr * 8 + f));
            }

            for (f = tf - 1; f >= 0; f--)
            {
                if (occupancy & (1ULL << (tr * 8 + f))) {
                    break;
                }
                attacks |= (1ULL << (tr * 8 + f));
            }
        }

        if (side == black) {
            if (!(occupancy & CASTLING_PASSING_SQUARES[2])) {
                sq = CASTLING_ATTACK_SQUARES[2][1];

                tr = sq / 8;
                tf = sq % 8;

                for (r = tr + 1; r <= 7; r++)
                {
                    if (occupancy & (1ULL << (r * 8 + tf))) {
                        break;
                    }
                    attacks |= (1ULL << (r * 8 + tf));
                }
            }
            
            if (!(occupancy & CASTLING_PASSING_SQUARES[3])) {
                sq = CASTLING_ATTACK_SQUARES[3][0];
                tr = sq / 8;
                tf = sq % 8;

                for (r = tr + 1; r <= 7; r++)
                {
                    if (occupancy & (1ULL << (r * 8 + tf))) {
                        break;
                    }
                    attacks |= (1ULL << (r * 8 + tf));
                }
            }
            
        }
        else {
            if (!(occupancy & CASTLING_PASSING_SQUARES[0])) {
                sq = CASTLING_ATTACK_SQUARES[0][1];

                tr = sq / 8;
                tf = sq % 8;

                for (r = tr - 1; r >= 0; r--)
                {
                    if (occupancy & (1ULL << (r * 8 + tf))) {
                        break;
                    }
                    attacks |= (1ULL << (r * 8 + tf));
                }
            }

            if (!(occupancy & CASTLING_PASSING_SQUARES[1])) {
                sq = CASTLING_ATTACK_SQUARES[1][0];
                tr = sq / 8;
                tf = sq % 8;

                for (r = tr - 1; r >= 0; r--)
                {
                    if (occupancy & (1ULL << (r * 8 + tf))) {
                        break;
                    }
                    attacks |= (1ULL << (r * 8 + tf));
                }
            }
        }

        return attacks;
    }

    void initChecks() {
        for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 64; j++) {
                if (PAWN_CAPTURES[white][i] & SQUARE_BITS[j]) pawnChecks[white][i][j] = i;
                else pawnChecks[white][i][j] = noSquare;

                if (PAWN_CAPTURES[black][i] & SQUARE_BITS[j]) pawnChecks[black][i][j] = i;
                else pawnChecks[black][i][j] = noSquare;

                if (KNIGHT_ATTACKS[i] & SQUARE_BITS[j]) knightChecks[i][j] = i;
                else knightChecks[i][j] = noSquare;
            }
        }
    }

    void setFen(const char* fen) {
        memset(pieces, 0ULL, sizeof(pieces));
        memset(occupancies, 0ULL, sizeof(occupancies));

        moveCount = 0;
        side = 0;
        enPassant = noSquare;
        castlingPermissions = 0;

        for (int i = 0; i < 65; i++) {
            boardPieces[i] = noPiece;
        }

        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
                int square = rank * 8 + file;

                if (*fen >= 'a' && *fen <= 'z')
                {
                    int piece = getPieceForCharacter(*fen);
                    SetBit(pieces[black][piece], square);
                    boardPieces[square] = piece;
                    fen++;
                }
                else if (*fen >= 'A' && *fen <= 'Z')
                {
                    int piece = getPieceForCharacter(*fen);
                    SetBit(pieces[white][piece], square);
                    boardPieces[square] = piece;
                    fen++;
                }

                if (*fen >= '0' && *fen <= '9')
                {
                    int offset = *fen - '0';
                    int piece = -1;

                    for (int bb_piece = p; bb_piece <= k; bb_piece++)
                    {
                        if (GetBit(pieces[white][bb_piece], square) || GetBit(pieces[black][bb_piece], square)) {
                            piece = bb_piece;
                        }
                    }

                    if (piece == -1) {
                        file--;
                    }

                    file += offset;

                    fen++;
                }

                if (*fen == '/') {
                    fen++;
                }
            }
        }

        // got to parsing side to move
        fen++;

        side = (*fen == 'w') ? white : black;

        // go to parsing castling rights
        fen += 2;

        while (*fen != ' ')
        {
            switch (*fen)
            {
            case 'K': castlingPermissions |= wk; break;
            case 'Q': castlingPermissions |= wq; break;
            case 'k': castlingPermissions |= bk; break;
            case 'q': castlingPermissions |= bq; break;
            case '-': break;
            }

            fen++;
        }

        // go to parsing en passant square
        fen++;

        if (*fen != '-')
        {
            int file = fen[0] - 'a';
            int rank = 8 - (fen[1] - '0');

            enPassant = rank * 8 + file;
        }
        else {
            enPassant = noSquare;
        }

        for (int piece = p; piece <= k; piece++) {
            occupancies[white] |= pieces[white][piece];
            occupancies[black] |= pieces[black][piece];
        }

        occupancies[both] |= occupancies[white];
        occupancies[both] |= occupancies[black];

        //look for checks for side to move
        U64 kingBit = pieces[side][k];

        checks = 0ULL;

        U64 bitboard = pieces[!side][p];
        Bitloop(bitboard) {
            int sourceSquare = SquareOf(bitboard);

            if (PAWN_CAPTURES[!side][sourceSquare] & kingBit) {
                checks |= (1ULL << sourceSquare);
            }
        }

        bitboard = pieces[!side][n];
        Bitloop(bitboard) {
            int sourceSquare = SquareOf(bitboard);

            if (KNIGHT_ATTACKS[sourceSquare] & kingBit) {
                checks |= (1ULL << sourceSquare);
            }
        }

        bitboard = pieces[!side][b];
        Bitloop(bitboard) {
            int sourceSquare = SquareOf(bitboard);

            if (getBishopAttacks(sourceSquare, occupancies[both]) & kingBit) {
                checks |= (1ULL << sourceSquare);
            }
        }

        bitboard = pieces[!side][r];
        Bitloop(bitboard) {
            int sourceSquare = SquareOf(bitboard);

            if (getRookAttacks(sourceSquare, occupancies[both]) & kingBit) {
                checks |= (1ULL << sourceSquare);
            }
        }

        bitboard = pieces[!side][q];
        Bitloop(bitboard) {
            int sourceSquare = SquareOf(bitboard);

            if (getQueenAttacks(sourceSquare, occupancies[both]) & kingBit) {
                checks |= (1ULL << sourceSquare);
            }
        }
    }

    string getFen() {
        string fen;

        int empty;
        int piece;
        for (int rank = 0; rank < 8; rank++)
        {
            empty = 0;
            for (int file = 0; file < 8; file++)
            {
                int square = rank * 8 + file;

                piece = -1;
                int side;

                for (int bb_piece = p; bb_piece <= k; bb_piece++)
                {
                    if (GetBit(pieces[white][bb_piece], square)) {
                        piece = bb_piece;
                        side = white;
                    }
                    else if (GetBit(pieces[black][bb_piece], square)) {
                        piece = bb_piece;
                        side = black;
                    }
                }

                if (piece != -1) {
                    if (empty > 0) {
                        fen += std::to_string(empty);
                    }
                    fen += ASCII_PIECES[side][piece];
                    empty = 0;
                }
                else {
                    empty++;
                }
            }
            if (piece == -1) fen += std::to_string(empty);

            if (rank < 7) fen += "/";
        }

        fen += " ";

        fen += (!side ? "w" : "b");

        fen += " ";

        string castling = (castlingPermissions & wk) ? "K" : "";
        castling += (castlingPermissions & wq) ? "Q" : "";
        castling += (castlingPermissions & bk) ? "k" : "";
        castling += (castlingPermissions & bq) ? "q" : "";

        fen += (castlingPermissions > 0) ? castling : "-";

        fen += " ";

        fen += (enPassant != noSquare) ? SQUARE_NAMES[enPassant] : "-";

        //TODO halfclock moves + total moves

        return fen;
    }
}