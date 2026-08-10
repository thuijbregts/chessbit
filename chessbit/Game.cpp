#pragma warning(disable:4146)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <iostream>
#include "Game.h"
#include "Zobrist.h"

using std::string;

namespace game {
    int moveCount;
    bstate::BoardState board = bstate::dummy;

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

    void printBoard(BoardState& board) {
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

                if (GetBit(board.pM, square)) { piece = p; side = board.side; }
                if (GetBit(board.nM, square)) { piece = n; side = board.side; }
                if (GetBit(board.bM, square)) { piece = b; side = board.side; }
                if (GetBit(board.rM, square)) { piece = r; side = board.side; }
                if (GetBit(board.qM, square)) { piece = q; side = board.side; }
                if (GetBit(board.kM, square)) { piece = k; side = board.side; }

                if (GetBit(board.pE, square)) { piece = p; side = !board.side; }
                if (GetBit(board.nE, square)) { piece = n; side = !board.side; }
                if (GetBit(board.bE, square)) { piece = b; side = !board.side; }
                if (GetBit(board.rE, square)) { piece = r; side = !board.side; }
                if (GetBit(board.qE, square)) { piece = q; side = !board.side; }
                if (GetBit(board.kE, square)) { piece = k; side = !board.side; }

                printf(" %c", (piece == -1) ? '.' : ASCII_PIECES[side][piece]);
            }

            printf("\n");
        }

        printf("\n     a b c d e f g h\n\n");

        printf("     Side:     %s\n", !board.side ? "white" : "black");

        printf("     En Passant:  %s\n", (board.eP != noSquare) ? SQUARE_NAMES[board.eP] : "no");

        printf("     Castling:  %c%c%c%c\n\n", 
            (board.casPerms & wk) ? 'K' : '-',
            (board.casPerms & wq) ? 'Q' : '-',
            (board.casPerms & bk) ? 'k' : '-',
            (board.casPerms & bq) ? 'q' : '-');
    }

    void makeMove(const BoardState& move) {
        moveCount++;
        movesPlayed[moveCount] = move;
        board = move;
    }

    void unmakeMove() {
        moveCount--;
        board = movesPlayed[moveCount];
    }

    void setFen(const char* fen) {
        U64 pieces[2][6];
        U64 occupancies[3];

        memset(pieces, 0ULL, sizeof(pieces));
        memset(occupancies, 0ULL, sizeof(occupancies));

        moveCount = 0;

        bool side = 0;
        int enPassant = noSquare;
        int castlingPermissions = 0;

        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
                int square = rank * 8 + file;

                if (*fen >= 'a' && *fen <= 'z')
                {
                    int piece = getPieceForCharacter(*fen);
                    SetBit(pieces[black][piece], square);
                    fen++;
                }
                else if (*fen >= 'A' && *fen <= 'Z')
                {
                    int piece = getPieceForCharacter(*fen);
                    SetBit(pieces[white][piece], square);
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

        U64 checks = 0ULL;

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

        int kMS = SquareOf(pieces[side][k]);
        int kES = SquareOf(pieces[!side][k]);

        Zobrist zobrist = zobrist::init(pieces, side, castlingPermissions, enPassant);

        board = bstate::BoardState(0, 0, pieces[side][p], pieces[side][n], pieces[side][b], pieces[side][r], pieces[side][q], pieces[side][k],
                                    pieces[!side][p], pieces[!side][n], pieces[!side][b], pieces[!side][r], pieces[!side][q], pieces[!side][k],
                                    kMS, kES, getKingAttacks(kMS), getKingAttacks(kES),
                                    occupancies[side], occupancies[!side], occupancies[both],
                                    checks, castlingPermissions, enPassant, 0, 0, side, false, false, false, zobrist);

        movesPlayed[0] = board;
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

                if (GetBit(board.pM, square)) { piece = p; side = board.side; }
                if (GetBit(board.nM, square)) { piece = n; side = board.side; }
                if (GetBit(board.bM, square)) { piece = b; side = board.side; }
                if (GetBit(board.rM, square)) { piece = r; side = board.side; }
                if (GetBit(board.qM, square)) { piece = q; side = board.side; }
                if (GetBit(board.kM, square)) { piece = k; side = board.side; }

                if (GetBit(board.pE, square)) { piece = p; side = !board.side; }
                if (GetBit(board.nE, square)) { piece = n; side = !board.side; }
                if (GetBit(board.bE, square)) { piece = b; side = !board.side; }
                if (GetBit(board.rE, square)) { piece = r; side = !board.side; }
                if (GetBit(board.qE, square)) { piece = q; side = !board.side; }
                if (GetBit(board.kE, square)) { piece = k; side = !board.side; }

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

        fen += (!board.side ? "w" : "b");

        fen += " ";

        string castling = (board.casPerms & wk) ? "K" : "";
        castling += (board.casPerms & wq) ? "Q" : "";
        castling += (board.casPerms & bk) ? "k" : "";
        castling += (board.casPerms & bq) ? "q" : "";

        fen += (board.casPerms > 0) ? castling : "-";

        fen += " ";

        fen += (board.eP != noSquare) ? SQUARE_NAMES[board.eP] : "-";

        //TODO halfclock moves + total moves

        return fen;
    }
}