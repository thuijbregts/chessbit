#include "MoveGenerator.h"
#include <cstdio>
#include <vector>

namespace movegen {
    U64 pinMasks[7][65][64];
    U64 potentialPinnerMasks[64][64];
    U64 validAttacksMasks[5949][64];
    /*U64 pawnAttackZones[2][64][8192];
    U64 pawnZones[2][64];*/
    U64 knightAttackZones[64];
    U64 bishopAttackZones[64];
    U64 rookAttackZones[64];
    int pawnChecks[2][64][64];
    int knightChecks[64][64];

    U64 occupanciesSaved[5949][3];
    int castlingPermissionsSaved[5949];
    int enPassantSaved[5949];
    int checkSquaresSaved[5949][2];
    int checkPiecesSaved[5949][2];
    int checkIndex;

    void makeMove(MoveInfo& move) {
        movesPlayed[moveCount] = move;
        occupanciesSaved[moveCount][white] = occupancies[white];
        occupanciesSaved[moveCount][black] = occupancies[black];
        occupanciesSaved[moveCount][both] = occupancies[both];
        castlingPermissionsSaved[moveCount] = castlingPermissions;
        enPassantSaved[moveCount] = enPassant;
        checkSquaresSaved[moveCount][0] = checkSquares[0];
        checkSquaresSaved[moveCount][1] = checkSquares[1];
        checkPiecesSaved[moveCount][0] = checkPieces[0];
        checkPiecesSaved[moveCount][1] = checkPieces[1];

        moveCount++;

        U64 bD, rD;
        findPotentialDiscovers(side, occupancies[both], pieces[side][b], pieces[side][r], pieces[side][q], bD, rD);

        enPassant = move.enPassant;

        U64& pM = pieces[side][p];
        U64& nM = pieces[side][n];
        U64& bM = pieces[side][b];
        U64& rM = pieces[side][r];
        U64& qM = pieces[side][q];
        U64& kM = pieces[side][k];

        U64& pE = pieces[!side][p];
        U64& nE = pieces[!side][n];
        U64& bE = pieces[!side][b];
        U64& rE = pieces[!side][r];
        U64& qE = pieces[!side][q];
        U64& kE = pieces[!side][k];

        switch (move.type) {
            case Pawn: {
                makePawn(side, move.from, move.to, castlingPermissions, bD, rD, occupancies[side], occupancies[!side], occupancies[both], pM, pE, nE, bE, rE, qE, kE);
                boardPieces[move.from] = noPiece;
                boardPieces[move.to] = move.movedPiece;
                break;
            }
            case Knight: {
                makeKnight(side, move.from, move.to, castlingPermissions, bD, rD, occupancies[side], occupancies[!side], occupancies[both], nM, pE, nE, bE, rE, qE);
                boardPieces[move.from] = noPiece;
                boardPieces[move.to] = move.movedPiece;
                break;
            }
            case Bishop: {
                makeBishop(side, move.from, move.to, castlingPermissions, bD, rD, occupancies[side], occupancies[!side], occupancies[both], bM, pE, nE, bE, rE, qE, kE);
                boardPieces[move.from] = noPiece;
                boardPieces[move.to] = move.movedPiece;
                break;
            }
            case Rook: {
                makeRook(side, move.from, move.to, castlingPermissions, bD, rD, occupancies[side], occupancies[!side], occupancies[both], rM, pE, nE, bE, rE, qE, kE);
                boardPieces[move.from] = noPiece;
                boardPieces[move.to] = move.movedPiece;
                break;
            }
            case Queen: {
                makeQueen(side, move.from, move.to, castlingPermissions, bD, rD, occupancies[side], occupancies[!side], occupancies[both], qM, pE, nE, bE, rE, qE, kE);
                boardPieces[move.from] = noPiece;
                boardPieces[move.to] = move.movedPiece;
                break;
            }
            case King: {
                makeKing(side, move.from, move.to, castlingPermissions, bD, rD, occupancies[side], occupancies[!side], occupancies[both], kM, pE, nE, bE, rE, qE);
                boardPieces[move.from] = noPiece;
                boardPieces[move.to] = move.movedPiece;
                break;
            }
            case PromotionKnight: {
                makePromotionKnight(side, move.from, move.to, castlingPermissions, bD, rD, occupancies[side], occupancies[!side], occupancies[both], pM, nM, nE, bE, rE, qE);
                boardPieces[move.from] = noPiece;
                boardPieces[move.to] = move.promotedPiece;
                break;
            }
            case PromotionBishop: {
                makePromotionBishop(side, move.from, move.to, castlingPermissions, bD, rD, occupancies[side], occupancies[!side], occupancies[both], pM, bM, nE, bE, rE, qE, kE);
                boardPieces[move.from] = noPiece;
                boardPieces[move.to] = move.promotedPiece;
                break;
            }
            case PromotionRook: {
                makePromotionRook(side, move.from, move.to, castlingPermissions, bD, rD, occupancies[side], occupancies[!side], occupancies[both], pM, rM, nE, bE, rE, qE, kE);
                boardPieces[move.from] = noPiece;
                boardPieces[move.to] = move.promotedPiece;
                break;
            }
            case PromotionQueen: {
                makePromotionQueen(side, move.from, move.to, castlingPermissions, bD, rD, occupancies[side], occupancies[!side], occupancies[both], pM, qM, nE, bE, rE, qE, kE);
                boardPieces[move.from] = noPiece;
                boardPieces[move.to] = move.promotedPiece;
                break;
            }
            case EnPassant: {
                makePawnEnPassant(side, move.from, move.to, bD, occupancies[side], occupancies[!side], occupancies[both], pM, bM, rM, qM, pE, kE);
                boardPieces[move.from] = noPiece;
                boardPieces[move.to] = move.movedPiece;
                int ePawnSquare = move.to + PAWN_PUSH[!side];
                boardPieces[ePawnSquare] = noPiece;
                break;
            }
            case Castling: {
                makeCastling(side, move.from, move.to, move.rookFrom, move.rookTo, castlingPermissions, occupancies[side], occupancies[!side], occupancies[both], kM, rM, kE);
                boardPieces[move.from] = noPiece;
                boardPieces[move.to] = move.movedPiece;
                boardPieces[move.rookFrom] = noPiece;
                boardPieces[move.rookTo] = r;
            }
        }

        side = !side;
    }

    void unmakeMove(MoveInfo& move) {
        side = !side;
        moveCount--;

        occupancies[white] = occupanciesSaved[moveCount][white];
        occupancies[black] = occupanciesSaved[moveCount][black];
        occupancies[both] = occupanciesSaved[moveCount][both];
        castlingPermissions = castlingPermissionsSaved[moveCount];
        checkSquares[0] = checkSquaresSaved[moveCount][0];
        checkSquares[1] = checkSquaresSaved[moveCount][1];
        checkPieces[0] = checkPiecesSaved[moveCount][0];
        checkPieces[1] = checkPiecesSaved[moveCount][1];

        enPassant = enPassantSaved[moveCount];

        switch (move.type) {
            case Pawn:
            case Knight:
            case Bishop:
            case Rook:
            case Queen: unmake(side, move.from, move.to, move.movedPiece, move.deadPiece); break;
            case King: unmakeKing(side, move.from, move.to, move.deadPiece); break;
            case PromotionKnight:
            case PromotionBishop:
            case PromotionRook:
            case PromotionQueen: unmakePromotion(side, move.from, move.to, move.deadPiece, move.promotedPiece); break;
            case EnPassant: unmakePawnEnPassant(side, move.from, move.to); break;
            case Castling: unmakeCastling(side, move.from, move.to, move.rookFrom, move.rookTo); break;
        }
    }

    ForceInline void initializeMove() {
        checkIndex = 0;
        checkSquares[0] = noSquare;
        checkSquares[1] = noSquare;
        checkPieces[0] = noPiece;
        checkPieces[1] = noPiece;
    }

    ForceInline void make(bool side, int from, int to, int& casPerm, U64 bDis, U64 rDis, U64& occM, U64& occE, U64& occB, U64& movP, U64& pE, U64& nE, U64& bE, U64& rE, U64& qE) {
        initializeMove();
        //printf("%s%s\n", SQUARE_NAMES[from], SQUARE_NAMES[to]);
        MoveBit(movP, from, to);
        MoveBit(occM, from, to);

        U64 toBit = (1ULL << to);
        if (occE & toBit) {
            pE &= ~toBit;
            nE &= ~toBit;
            bE &= ~toBit;
            rE &= ~toBit;
            qE &= ~toBit;
            PopBit(occE, to);
            casPerm &= NO_CASTLE_ROOK[!side][to];
        }

        occB = occM | occE;

        discoverCheck(side, from, bDis, rDis, occB);
    }

    ForceInline void makePawn(bool side, int from, int to, int& casPerm, U64 bDis, U64 rDis, U64& occM, U64& occE, U64& occB, U64& pM, U64& pE, U64& nE, U64& bE, U64& rE, U64& qE, U64 kE) {
        make(side, from, to, casPerm, bDis, rDis,occM, occE, occB, pM, pE, nE, bE, rE, qE);

        //faster
        if (PAWN_CAPTURES[side][to] & kE) {
            checkSquares[checkIndex] = to;
            checkPieces[checkIndex] = p;
        }
        //checkSquares[checkIndex] = pawnChecks[side][to][king[!side]];
    }

    ForceInline void makePawnEnPassant(bool side, int from, int to, U64 bDis, U64& occM, U64& occE, U64& occB, U64& pM, U64 bM, U64 rM, U64 qM, U64& pE, U64 kE) {
        initializeMove();
        //printf("%s%s\n", SQUARE_NAMES[from], SQUARE_NAMES[to]);
        MoveBit(pM, from, to);
        MoveBit(occM, from, to);

        int ePawnSquare = to + PAWN_PUSH[!side];
        PopBit(pE, ePawnSquare);
        PopBit(occE, ePawnSquare);

        occB = occM | occE;

        discoverCheckPassant(side, from, ePawnSquare, bDis, occM, occB, bM, rM, qM);

        if (PAWN_CAPTURES[side][to] & kE) {
            checkSquares[checkIndex] = to;
            checkPieces[checkIndex] = p;
        }
    }

    ForceInline void makePromotion(bool side, int from, int to, int& casPerm, U64 bDis, U64 rDis, U64& occM, U64& occE, U64& occB, U64& pM, U64& promo, U64& nE, U64& bE, U64& rE, U64& qE) {
        initializeMove();
        //printf("%s%s\n", SQUARE_NAMES[from], SQUARE_NAMES[to]);
        PopBit(pM, from);
        SetBit(promo, to);
        MoveBit(occM, from, to);

        U64 toBit = (1ULL << to);
        if (occE & toBit) {
            nE &= ~toBit;
            bE &= ~toBit;
            rE &= ~toBit;
            qE &= ~toBit;
            PopBit(occE, to);
            casPerm &= NO_CASTLE_ROOK[!side][to];
        }

        occB = occM | occE;

        discoverCheck(side, from, bDis, rDis, occB);
    }

    ForceInline void makePromotionKnight(bool side, int from, int to, int& casPerm, U64 bDis, U64 rDis, U64& occM, U64& occE, U64& occB, U64& pM, U64& nM, U64& nE, U64& bE, U64& rE, U64& qE) {
        makePromotion(side, from, to, casPerm, bDis, rDis, occM, occE, occB, pM, nM, nE, bE, rE, qE);

        /*if (KNIGHT_ATTACKS[to] & kE) {
            checkSquares[checkIndex] = to;
        }*/
        //faster
        checkSquares[checkIndex] = knightChecks[to][king[!side]];
        checkPieces[checkIndex] = n;
    }

    ForceInline void makePromotionBishop(bool side, int from, int to, int& casPerm, U64 bDis, U64 rDis, U64& occM, U64& occE, U64& occB, U64& pM, U64& bM, U64& nE, U64& bE, U64& rE, U64& qE, U64 kE) {
        makePromotion(side, from, to, casPerm, bDis, rDis, occM, occE, occB, pM, bM, nE, bE, rE, qE);

        if (getBishopAttacks(to, occB) & kE) {
            checkSquares[checkIndex] = to;
            checkPieces[checkIndex] = b;
        }
    }

    ForceInline void makePromotionRook(bool side, int from, int to, int& casPerm, U64 bDis, U64 rDis, U64& occM, U64& occE, U64& occB, U64& pM, U64& rM, U64& nE, U64& bE, U64& rE, U64& qE, U64 kE) {
        makePromotion(side, from, to, casPerm, bDis, rDis, occM, occE, occB, pM, rM, nE, bE, rE, qE);

        if (getRookAttacks(to, occB) & kE) {
            checkSquares[checkIndex] = to;
            checkPieces[checkIndex] = r;
        }
    }

    ForceInline void makePromotionQueen(bool side, int from, int to, int& casPerm, U64 bDis, U64 rDis, U64& occM, U64& occE, U64& occB, U64& pM, U64& qM, U64& nE, U64& bE, U64& rE, U64& qE, U64 kE) {
        makePromotion(side, from, to, casPerm, bDis, rDis, occM, occE, occB, pM, qM, nE, bE, rE, qE);

        if (getQueenAttacks(to, occB) & kE) {
            checkSquares[checkIndex] = to;
            checkPieces[checkIndex] = q;
        }
    }

    ForceInline void makeKnight(bool side, int from, int to, int& casPerm, U64 bDis, U64 rDis, U64& occM, U64& occE, U64& occB, U64& nM, U64& pE, U64& nE, U64& bE, U64& rE, U64& qE) {
        make(side, from, to, casPerm, bDis, rDis, occM, occE, occB, nM, pE, nE, bE, rE, qE);

        /*if (KNIGHT_ATTACKS[to] & kE) {
            checkSquares[checkIndex] = to;
        }*/
        checkSquares[checkIndex] = knightChecks[to][king[!side]];
        checkPieces[checkIndex] = n;
    }

    ForceInline void makeBishop(bool side, int from, int to, int& casPerm, U64 bDis, U64 rDis, U64& occM, U64& occE, U64& occB, U64& bM, U64& pE, U64& nE, U64& bE, U64& rE, U64& qE, U64 kE) {
        make(side, from, to, casPerm, bDis, rDis, occM, occE, occB, bM, pE, nE, bE, rE, qE);

        if (getBishopAttacks(to, occB) & kE) {
            checkSquares[checkIndex] = to;
            checkPieces[checkIndex] = b;
        }
    }

    ForceInline void makeRook(bool side, int from, int to, int& casPerm, U64 bDis, U64 rDis, U64& occM, U64& occE, U64& occB, U64& rM, U64& pE, U64& nE, U64& bE, U64& rE, U64& qE, U64 kE) {
        make(side, from, to, casPerm, bDis, rDis, occM, occE, occB, rM, pE, nE, bE, rE, qE);

        casPerm &= NO_CASTLE_ROOK[side][from];

        if (getRookAttacks(to, occB) & kE) {
            checkSquares[checkIndex] = to;
            checkPieces[checkIndex] = r;
        }
    }

    ForceInline void makeQueen(bool side, int from, int to, int& casPerm, U64 bDis, U64 rDis, U64& occM, U64& occE, U64& occB, U64& qM, U64& pE, U64& nE, U64& bE, U64& rE, U64& qE, U64 kE) {
        make(side, from, to, casPerm, bDis, rDis, occM, occE, occB, qM, pE, nE, bE, rE, qE);

        if (getQueenAttacks(to, occB) & kE) {
            checkSquares[checkIndex] = to;
            checkPieces[checkIndex] = q;
        }
    }

    ForceInline void makeKing(bool side, int from, int to, int& casPerm, U64 bDis, U64 rDis, U64& occM, U64& occE, U64& occB, U64& kM, U64& pE, U64& nE, U64& bE, U64& rE, U64& qE) {
        make(side, from, to, casPerm, bDis, rDis, occM, occE, occB, kM, pE, nE, bE, rE, qE);

        casPerm &= NO_CASTLE[side];
        king[side] = to;
    }

    ForceInline void makeCastling(bool side, int from, int to, int rookFrom, int rookTo, int& casPerm, U64& occM, U64& occE, U64& occB, U64& kM, U64& rM, U64 kE) {
        initializeMove();
        //printf("%s%s\n", SQUARE_NAMES[from], SQUARE_NAMES[to]);
        //move king
        MoveBit(kM, from, to);
        MoveBit(occM, from, to);
        king[side] = to;

        //move rook
        MoveBit(rM, rookFrom, rookTo);
        MoveBit(occM, rookFrom, rookTo);

        casPerm &= NO_CASTLE[side];

        occB = occM | occE;

        if (getRookAttacks(rookTo, occB) & kE) {
            checkSquares[checkIndex] = rookTo;
            checkPieces[checkIndex] = r;
        }
    }

    ForceInline void unmake(bool side, int from, int to, int movedPiece, int deadPiece) {
        MoveBit(pieces[side][movedPiece], to, from);

        boardPieces[from] = movedPiece;

        SetBit(pieces[!side][deadPiece], to);
        boardPieces[to] = deadPiece;
    }

    ForceInline void unmakeKing(bool side, int from, int to, int deadPiece) {
        unmake(side, from, to, k, deadPiece);

        king[side] = from;
    }

    ForceInline void unmakePawnEnPassant(bool side, int from, int to) {
        MoveBit(pieces[side][p], to, from);
        boardPieces[to] = noPiece;
        boardPieces[from] = p;

        int ePawnSquare = to + PAWN_PUSH[!side];
        SetBit(pieces[!side][p], ePawnSquare);
        boardPieces[ePawnSquare] = p;
    }

    ForceInline void unmakePromotion(bool side, int from, int to, int deadPiece, int promotedPiece) {
        PopBit(pieces[side][promotedPiece], to);
        SetBit(pieces[side][p], from);
        boardPieces[from] = p;

        SetBit(pieces[!side][deadPiece], to);
        boardPieces[to] = deadPiece;
    }

    ForceInline void unmakeCastling(bool side, int from, int to, int rookFrom, int rookTo) {
        //move king
        MoveBit(pieces[side][k], to, from);
        boardPieces[to] = noPiece;
        boardPieces[from] = k;
        king[side] = from;

        //move rook
        MoveBit(pieces[side][r], rookTo, rookFrom);
        boardPieces[rookTo] = noPiece;
        boardPieces[rookFrom] = r;
    }

    void init() {
        initPinMasks();
        initValidAttacksMasks();
        initPotentielPinnerMasks();
        initAttackZones();
        initChecks();
    }

    void initPinMasks() {
        int currentIndex, endIndex, bitShift;
        U64 pinMask;

        for (int piece = p; piece <= noPiece; piece++) {
            for (int i = 0; i < 64; i++) {
                for (int j = 0; j < 64; j++) {
                    pinMask = 0ULL;
                    bitShift = 0;
                    //bit shifts for horizontal: 1
                    if (RANKS[i] == RANKS[j]) {
                        bitShift = (piece == r || piece == q) ? 1 : 0;
                    }
                    //bit shifts for vertical: 8
                    else if (FILES[i] == FILES[j]) {
                        bitShift = (piece == r || piece == q) ? 8 : 0;
                    }
                    //bit shifts for diagonals: 7, 9
                    else {
                        int x0 = RANKS[i];
                        int x1 = RANKS[j];
                        int y0 = FILES[i];
                        int y1 = FILES[j];

                        if (std::abs(x0 - x1) == std::abs(y0 - y1)) {
                            if ((i - j) % 9 == 0) {
                                bitShift = (piece == b || piece == q) ? 9 : 0;
                            }
                            else {
                                bitShift = (piece == b || piece == q) ? 7 : 0;
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
                    }

                    pinMasks[piece][i][j] = pinMask;
                }
            }
            for (int i = 0; i < 64; i++) {
                pinMasks[piece][64][i] = 0ULL;
            }
        }
    }

    void initValidAttacksMasks() {
        for (int i = 0; i < 5949; i++) {
            for (int j = 0; j < 64; j++) {
                validAttacksMasks[i][j] = FULL_BOARD;
            }
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

    void initAttackZones() {
        initPawnZones();
        for (int square = 0; square < 64; square++) {
            U64 attacks = getKingAttacks(square);
            Bitloop(attacks) {
                int attackSquare = SquareOf(attacks);

                knightAttackZones[square] |= getKnightAttacks(attackSquare);

                bishopAttackZones[square] |= BISHOP_XRAYS[attackSquare];

                rookAttackZones[square] |= ROOK_XRAYS[attackSquare];
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
                    if (RANKS[currentSquare] <= RANKS[square] && RANKS[currentSquare] < 8) SetBit(whiteZone, currentSquare - 2);
                    if (RANKS[currentSquare] >= RANKS[square] && RANKS[currentSquare] > 1) SetBit(blackZone, currentSquare - 2);
                }
                if (currentSquare - 1 >= 0 && FILES[square - 1] < 8) {
                    if (i != 1 && RANKS[currentSquare] <= RANKS[square] && RANKS[currentSquare] < 8) SetBit(whiteZone, currentSquare - 1);
                    if (i != -1 && RANKS[currentSquare] >= RANKS[square] && RANKS[currentSquare] > 1) SetBit(blackZone, currentSquare - 1);
                }
                if (currentSquare + 1 < 64 && FILES[currentSquare + 1] > 1) {
                    if (i != 1 && RANKS[currentSquare] <= RANKS[square] && RANKS[currentSquare] < 8) SetBit(whiteZone, currentSquare + 1);
                    if (i != -1 && RANKS[currentSquare] >= RANKS[square] && RANKS[currentSquare] > 1) SetBit(blackZone, currentSquare + 1);
                }
                if (currentSquare + 2 < 64 && FILES[currentSquare + 2] > 2) {
                    if (RANKS[currentSquare] <= RANKS[square] && RANKS[currentSquare] < 8) SetBit(whiteZone, currentSquare + 2);
                    if (RANKS[currentSquare] >= RANKS[square] && RANKS[currentSquare] > 1) SetBit(blackZone, currentSquare + 2);
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
                    int attackSquare = SquareOf(attacksLoop);
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
                    int attackSquare = SquareOf(attacksLoop);
                    //pawnAttackZones[black][square][i] |= PAWN_CAPTURES[black][attackSquare];
                }
                //pawnAttackZones[black][square][i] = ~pawnAttackZones[black][square][i];
            }
        }
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

    void generateMoves(MoveArray& moves) {
        int sourceSquare, targetSquare, deadPiece;
        U64 bitboard, attacks, pinMask;

        int checkSquare = checkSquares[0];
        int checkPiece = boardPieces[checkSquare];
        int checkSquare2 = checkSquares[1];

        if (checkSquare != noSquare) {

            /*

                KING MOVES

            */
            int kingSquare = king[side];

            //inverted pinMasks, because the king cannot move in the attack ray of the check pieces
            attacks = getKingAttacks(kingSquare) & ~occupancies[side] & ~pinMasks[boardPieces[checkSquare]][checkSquare][kingSquare] & ~pinMasks[boardPieces[checkSquare2]][checkSquare2][kingSquare];
            filterKingAttacks(side, occupancies[both], kingSquare, attacks, pieces[!side][p], pieces[!side][n], pieces[!side][b], pieces[!side][r], pieces[!side][q]);
            Bitloop(attacks)
            {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];
                moves.king(kingSquare, targetSquare, deadPiece);
            }

            //if there is no second check, we need to check for other pieces
            if (checkSquare2 == noSquare) {
                //remove the bit of the check piece for performance, because it cannot possibly pin a piece
                PopBit(pieces[!side][checkPiece], checkSquare);
                U64 allPins = findPins(0, side, occupancies[side], occupancies[both], pieces[!side][b], pieces[!side][r], pieces[!side][q]);
                SetBit(pieces[!side][checkPiece], checkSquare);

                U64 checkBit = (1ULL << checkSquare);

                //if check piece is pawn or knight, only possible moves are capture of the check piece (+ king moves)
                if (CAPTURE_ONLY[checkPiece]) {
                    /*

                       PAWN MOVES

                    */
                    targetSquare = checkSquare;
                    deadPiece = boardPieces[targetSquare];

                    //en passant
                    bitboard = pieces[side][p] & PASSANT_CAPTURES[enPassant] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        if (checkSquare == enPassant + PAWN_PUSH[!side]) {
                            moves.enPassant(sourceSquare, enPassant);
                        }
                    }

                    bitboard = pieces[side][p] & PROMO_RANKS[side] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        attacks = PAWN_CAPTURES[side][sourceSquare] & checkBit;
                        if (attacks)
                        {
                            moves.promotion(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][p] & ~PROMO_RANKS[side] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        attacks = PAWN_CAPTURES[side][sourceSquare] & checkBit;
                        if (attacks)
                        {
                            moves.pawn(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    //en passant
                    bitboard = pieces[side][p] & PASSANT_CAPTURES[enPassant] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        pinMask = validAttacksMasks[0][sourceSquare];
                        attacks = (1ULL << enPassant) & pinMask;
                        if (attacks && checkSquare == enPassant + PAWN_PUSH[!side]) {
                            moves.enPassant(sourceSquare, enPassant);
                        }
                    }

                    bitboard = pieces[side][p] & PROMO_RANKS[side] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        pinMask = validAttacksMasks[0][sourceSquare];

                        attacks = PAWN_CAPTURES[side][sourceSquare] & checkBit & pinMask;
                        if (attacks)
                        {
                            moves.promotion(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][p] & ~PROMO_RANKS[side] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        pinMask = validAttacksMasks[0][sourceSquare];

                        attacks = PAWN_CAPTURES[side][sourceSquare] & checkBit & pinMask;
                        if (attacks)
                        {
                            moves.pawn(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    /*

                       KNIGHT MOVES

                    */
                    bitboard = pieces[side][n] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        attacks = getKnightAttacks(sourceSquare) & checkBit;
                        if (attacks) {
                            moves.knight(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][n] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);
                        pinMask = validAttacksMasks[0][sourceSquare];

                        attacks = getKnightAttacks(sourceSquare) & checkBit & pinMask;
                        if (attacks) {
                            moves.knight(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    /*

                       BISHOP MOVES

                    */
                    bitboard = pieces[side][b] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        attacks = getBishopAttacks(sourceSquare, occupancies[both]) & checkBit;
                        if (attacks) {
                            moves.bishop(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][b] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);
                        pinMask = validAttacksMasks[0][sourceSquare];

                        attacks = getBishopAttacks(sourceSquare, occupancies[both]) & checkBit & pinMask;
                        if (attacks) {
                            moves.bishop(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    /*

                       ROOK MOVES

                    */
                    bitboard = pieces[side][r] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        attacks = getRookAttacks(sourceSquare, occupancies[both]) & checkBit;
                        if (attacks) {
                            moves.rook(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][r] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);
                        pinMask = validAttacksMasks[0][sourceSquare];

                        attacks = getRookAttacks(sourceSquare, occupancies[both]) & checkBit & pinMask;
                        if (attacks) {
                            moves.rook(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    /*

                       QUEEN MOVES

                    */
                    bitboard = pieces[side][q] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        attacks = getQueenAttacks(sourceSquare, occupancies[both]) & checkBit;
                        if (attacks) {
                            moves.queen(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][q] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);
                        pinMask = validAttacksMasks[0][sourceSquare];

                        attacks = getQueenAttacks(sourceSquare, occupancies[both]) & checkBit & pinMask;
                        if (attacks) {
                            moves.queen(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                }
                else {
                    //squares between the check piece (included) and the king
                    //those are the only squares that a piece can go to to block the check
                    U64 validSquares = (checkBit | pinMasks[boardPieces[checkSquare]][checkSquare][kingSquare]);

                    /*

                       PAWN MOVES

                    */
                    bitboard = pieces[side][p] & PROMO_RANKS[side] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        attacks = getPawnAttacks(sourceSquare, occupancies[both], occupancies[!side], side) & validSquares;
                        Bitloop(attacks) {
                            targetSquare = SquareOf(attacks);
                            deadPiece = boardPieces[targetSquare];

                            moves.promotion(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][p] & ~PROMO_RANKS[side] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        attacks = getPawnAttacks(sourceSquare, occupancies[both], occupancies[!side], side) & validSquares;
                        Bitloop(attacks) {
                            targetSquare = SquareOf(attacks);
                            deadPiece = boardPieces[targetSquare];

                            moves.pawn(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][p] & PROMO_RANKS[side] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);
                        pinMask = validAttacksMasks[0][sourceSquare];

                        attacks = getPawnAttacks(sourceSquare, occupancies[both], occupancies[!side], side) & pinMask & validSquares;
                        Bitloop(attacks) {
                            targetSquare = SquareOf(attacks);
                            deadPiece = boardPieces[targetSquare];

                            moves.promotion(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][p] & ~PROMO_RANKS[side] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);
                        pinMask = validAttacksMasks[0][sourceSquare];

                        attacks = getPawnAttacks(sourceSquare, occupancies[both], occupancies[!side], side) & pinMask & validSquares;
                        Bitloop(attacks) {
                            targetSquare = SquareOf(attacks);
                            deadPiece = boardPieces[targetSquare];

                            moves.pawn(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    /*

                       KNIGHT MOVES

                    */
                    bitboard = pieces[side][n] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        attacks = getKnightAttacks(sourceSquare) & validSquares;
                        Bitloop(attacks) {
                            targetSquare = SquareOf(attacks);
                            deadPiece = boardPieces[targetSquare];

                            moves.knight(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][n] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);
                        pinMask = validAttacksMasks[0][sourceSquare];

                        attacks = getKnightAttacks(sourceSquare) & validSquares & pinMask;
                        Bitloop(attacks) {
                            targetSquare = SquareOf(attacks);
                            deadPiece = boardPieces[targetSquare];

                            moves.knight(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    /*

                       BISHOP MOVES

                    */
                    bitboard = pieces[side][b] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        attacks = getBishopAttacks(sourceSquare, occupancies[both]) & validSquares;
                        Bitloop(attacks) {
                            targetSquare = SquareOf(attacks);
                            deadPiece = boardPieces[targetSquare];

                            moves.bishop(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][b] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);
                        pinMask = validAttacksMasks[0][sourceSquare];

                        attacks = getBishopAttacks(sourceSquare, occupancies[both]) & validSquares & pinMask;
                        Bitloop(attacks) {
                            targetSquare = SquareOf(attacks);
                            deadPiece = boardPieces[targetSquare];

                            moves.bishop(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    /*

                       ROOK MOVES

                    */
                    bitboard = pieces[side][r] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        attacks = getRookAttacks(sourceSquare, occupancies[both]) & validSquares;
                        Bitloop(attacks) {
                            targetSquare = SquareOf(attacks);
                            deadPiece = boardPieces[targetSquare];

                            moves.rook(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][r] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);
                        pinMask = validAttacksMasks[0][sourceSquare];

                        attacks = getRookAttacks(sourceSquare, occupancies[both]) & validSquares & pinMask;
                        Bitloop(attacks) {
                            targetSquare = SquareOf(attacks);
                            deadPiece = boardPieces[targetSquare];

                            moves.rook(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    /*

                       QUEEN MOVES

                    */
                    bitboard = pieces[side][q] & ~allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);

                        attacks = getQueenAttacks(sourceSquare, occupancies[both]) & validSquares;
                        Bitloop(attacks) {
                            targetSquare = SquareOf(attacks);
                            deadPiece = boardPieces[targetSquare];

                            moves.queen(sourceSquare, targetSquare, deadPiece);
                        }
                    }

                    bitboard = pieces[side][q] & allPins;
                    Bitloop(bitboard)
                    {
                        sourceSquare = SquareOf(bitboard);
                        pinMask = validAttacksMasks[0][sourceSquare];

                        attacks = getQueenAttacks(sourceSquare, occupancies[both]) & validSquares & pinMask;
                        Bitloop(attacks) {
                            targetSquare = SquareOf(attacks);
                            deadPiece = boardPieces[targetSquare];

                            moves.queen(sourceSquare, targetSquare, deadPiece);
                        }
                    }
                }
            }

            return;
        }

        U64 allPins = findPins(0, side, occupancies[side], occupancies[both], pieces[!side][b], pieces[!side][r], pieces[!side][q]);

        /*

           PAWN MOVES

        */
        //en passant
        bitboard = PASSANT_CAPTURES[enPassant] & pieces[side][p] & ~allPins;
        Bitloop(bitboard) {
            sourceSquare = SquareOf(bitboard);

            if ((1ULL << enPassant) & passantPinMask(side, enPassant, sourceSquare, occupancies[both], pieces[!side][r], pieces[!side][q])) {
                moves.enPassant(sourceSquare, enPassant);
            }
        }

        bitboard = pieces[side][p] & PROMO_RANKS[side] & ~allPins;
        Bitloop(bitboard)
        {
            sourceSquare = SquareOf(bitboard);

            attacks = getPawnAttacks(sourceSquare, occupancies[both], occupancies[!side], side);
            Bitloop(attacks) {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];

                moves.promotion(sourceSquare, targetSquare, deadPiece);
            }
        }

        bitboard = pieces[side][p] & ~PROMO_RANKS[side] & ~allPins;
        Bitloop(bitboard)
        {
            sourceSquare = SquareOf(bitboard);

            attacks = getPawnAttacks(sourceSquare, occupancies[both], occupancies[!side], side);
            Bitloop(attacks) {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];

                moves.pawn(sourceSquare, targetSquare, deadPiece);
            }
        }

        //en passant
        bitboard = PASSANT_CAPTURES[enPassant] & pieces[side][p] & allPins;
        Bitloop(bitboard) {
            sourceSquare = SquareOf(bitboard);
            pinMask = validAttacksMasks[0][sourceSquare];

            if ((1ULL << enPassant) & pinMask & passantPinMask(side, enPassant, sourceSquare, occupancies[both], pieces[!side][r], pieces[!side][q])) {
                moves.enPassant(sourceSquare, enPassant);
            }
        }

        bitboard = pieces[side][p] & PROMO_RANKS[side] & allPins;
        Bitloop(bitboard)
        {
            sourceSquare = SquareOf(bitboard);
            pinMask = validAttacksMasks[0][sourceSquare];

            attacks = getPawnAttacks(sourceSquare, occupancies[both], occupancies[!side], side) & pinMask;
            Bitloop(attacks) {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];

                moves.promotion(sourceSquare, targetSquare, deadPiece);
            }
        }

        bitboard = pieces[side][p] & ~PROMO_RANKS[side] & allPins;
        Bitloop(bitboard)
        {
            sourceSquare = SquareOf(bitboard);
            pinMask = validAttacksMasks[0][sourceSquare];

            attacks = getPawnAttacks(sourceSquare, occupancies[both], occupancies[!side], side) & pinMask;
            Bitloop(attacks) {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];

                moves.pawn(sourceSquare, targetSquare, deadPiece);
            }
        }

        /*

           KNIGHT MOVES

        */
        bitboard = pieces[side][n] & ~allPins;
        Bitloop(bitboard)
        {
            sourceSquare = SquareOf(bitboard);

            attacks = getKnightAttacks(sourceSquare) & ~occupancies[side];
            Bitloop(attacks)
            {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];

                moves.knight(sourceSquare, targetSquare, deadPiece);
            }
        }

        bitboard = pieces[side][n] & allPins;
        Bitloop(bitboard)
        {
            sourceSquare = SquareOf(bitboard);
            pinMask = validAttacksMasks[0][sourceSquare];

            attacks = getKnightAttacks(sourceSquare) & ~occupancies[side] & pinMask;
            Bitloop(attacks)
            {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];

                moves.knight(sourceSquare, targetSquare, deadPiece);
            }
        }

        /*

           BISHOP MOVES

        */
        bitboard = pieces[side][b] & ~allPins;
        Bitloop(bitboard)
        {
            sourceSquare = SquareOf(bitboard);

            attacks = getBishopAttacks(sourceSquare, occupancies[both]) & ~occupancies[side];
            Bitloop(attacks)
            {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];

                moves.bishop(sourceSquare, targetSquare, deadPiece);
            }
        }

        bitboard = pieces[side][b] & allPins;
        Bitloop(bitboard)
        {
            sourceSquare = SquareOf(bitboard);
            pinMask = validAttacksMasks[0][sourceSquare];

            attacks = getBishopAttacks(sourceSquare, occupancies[both]) & ~occupancies[side] & pinMask;
            Bitloop(attacks)
            {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];

                moves.bishop(sourceSquare, targetSquare, deadPiece);
            }
        }

        /*

           ROOK MOVES

        */
        bitboard = pieces[side][r] & ~allPins;
        Bitloop(bitboard)
        {
            sourceSquare = SquareOf(bitboard);

            attacks = getRookAttacks(sourceSquare, occupancies[both]) & ~occupancies[side];
            Bitloop(attacks)
            {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];

                moves.rook(sourceSquare, targetSquare, deadPiece);
            }
        }

        bitboard = pieces[side][r] & allPins;
        Bitloop(bitboard)
        {
            sourceSquare = SquareOf(bitboard);
            pinMask = validAttacksMasks[0][sourceSquare];

            attacks = getRookAttacks(sourceSquare, occupancies[both]) & ~occupancies[side] & pinMask;
            Bitloop(attacks)
            {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];

                moves.rook(sourceSquare, targetSquare, deadPiece);
            }
        }

        /*

           QUEEN MOVES

        */
        bitboard = pieces[side][q] & ~allPins;
        Bitloop(bitboard)
        {
            sourceSquare = SquareOf(bitboard);

            attacks = getQueenAttacks(sourceSquare, occupancies[both]) & ~occupancies[side];
            Bitloop(attacks)
            {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];

                moves.queen(sourceSquare, targetSquare, deadPiece);
            }
        }

        bitboard = pieces[side][q] & allPins;
        Bitloop(bitboard)
        {
            sourceSquare = SquareOf(bitboard);
            pinMask = validAttacksMasks[0][sourceSquare];

            attacks = getQueenAttacks(sourceSquare, occupancies[both]) & ~occupancies[side] & pinMask;
            Bitloop(attacks)
            {
                targetSquare = SquareOf(attacks);
                deadPiece = boardPieces[targetSquare];

                moves.queen(sourceSquare, targetSquare, deadPiece);
            }
        }

        /*

           KING MOVES

        */
        sourceSquare = king[side];

        attacks = getKingAttacks(sourceSquare) & ~occupancies[side];
        filterKingAttacks(side, occupancies[both], sourceSquare, attacks, pieces[!side][p], pieces[!side][n], pieces[!side][b], pieces[!side][r], pieces[!side][q]);
        Bitloop(attacks)
        {
            targetSquare = SquareOf(attacks);
            deadPiece = boardPieces[targetSquare];

            moves.king(sourceSquare, targetSquare, deadPiece);
        }

        //castling moves
        int castlingSide = CASTLING_SIDE_K[side];
        if (castle(side, castlingSide, castlingPermissions, occupancies[!side], occupancies[both], pieces[!side][n], pieces[!side][b], pieces[!side][r], pieces[!side][q])) {
            moves.castling(CASTLING_KING_TARGET_SQUARE[castlingSide]);
        }

        castlingSide = CASTLING_SIDE_Q[side];
        if (castle(side, castlingSide, castlingPermissions, occupancies[!side], occupancies[both], pieces[!side][n], pieces[!side][b], pieces[!side][r], pieces[!side][q])) {
            moves.castling(CASTLING_KING_TARGET_SQUARE[castlingSide]);
        }
    }

    ForceInline void filterKingAttacks(bool side, U64 occB, int kingSquare, U64& kingAttacks, U64 pE, U64 nE, U64 bE, U64 rE, U64 qE) {
        if (!kingAttacks) {
            return;
        }

        //kingAttacks &= pawnAttackZones[!side][kingSquare][_pext_u64(pE, pawnZones[!side][kingSquare])];
        kingAttacks &= getPawnKingAttacks(!side, kingSquare, pE);

        kingAttacks &= ~getKingAttacks(king[!side]);

        //remove king to avoid collisions, as it should not be considered when checking for threats
        PopBit(occB, kingSquare);

        U64 bitboard;
        bitboard = nE & knightAttackZones[kingSquare];
        Bitloop(bitboard) {
            kingAttacks &= ~getKnightAttacks(SquareOf(bitboard));
        }

        bitboard = (bE | qE) & bishopAttackZones[kingSquare];
        Bitloop(bitboard) {
            kingAttacks &= ~getBishopAttacks(SquareOf(bitboard), occB);
        }

        bitboard = (rE | qE) & rookAttackZones[kingSquare];
        Bitloop(bitboard) {
            kingAttacks &= ~getRookAttacks(SquareOf(bitboard), occB);
        }
    }

    ForceInline bool castle(bool side, int castlingSide, int casPerm, U64 occE, U64 occB, U64 nE, U64 bE, U64 rE, U64 qE) {
        if (!(casPerm & CASTLING[castlingSide])) {
            return false;
        }

        if (CASTLING_OCCUPIED_SQUARES[castlingSide] & occB) {
            return false;
        }

        if (occE & CASTLING_FORBIDDEN_SQUARES[castlingSide]
            | nE & CASTLING_FORBIDDEN_KNIGHT_SQUARES[castlingSide]
            | (occE ^ nE) & CASTLING_FORBIDDEN_SQUARE_EXCEPT_KNIGHT[castlingSide]) {
            return false;
        }

        /*if ((occE ^ nE) & CASTLING_FORBIDDEN_SQUARE_EXCEPT_KNIGHT[castlingSide]) {
            return false;
        }*/

        if ((occE ^ rE) & CASTLING_FORBIDDEN_SQUARE_EXCEPT_ROOK[castlingSide]) {
            return false;
        }

        /*if (nE & CASTLING_FORBIDDEN_KNIGHT_SQUARES[castlingSide]) {
            return false;
        }*/

        //faster than getBishopAttacks x getRookAttacks (need to check 4 squares vs 2 here)
        U64 bishopAttacks = getBishopCastleAttacks(castlingSide, occB) & (bE | qE);
        U64 rookAttacks = getRookCastleAttacks(castlingSide, occB) & (rE | qE);

        return !(bishopAttacks | rookAttacks);
    }

    ForceInline U64 passantPinMask(bool side, int enPassant, int pawnSquare, U64 occB, U64 rE, U64 qE) {
        int kingSquare = king[side];
        if (RANKS[pawnSquare] != RANKS[kingSquare]) {
            return FULL_BOARD;
        }

        int enemyPawn = enPassant + PAWN_PUSH[!side];
        PopBit(occB, enemyPawn);
        PopBit(occB, pawnSquare);

        return PASSANT_PIN_RESULT[SquareOf(getRookAttacks(kingSquare, occB) & potentialPinnerMasks[pawnSquare][kingSquare] & (rE | qE))];
    }

    ForceInline U64 passantPinMask1(int enPassant, U64 occB, U64 pM, U64 rE, U64 qE) {
        int kingSquare = king[side];
        U64 pawnSquares = PAWN_CAPTURES[!side][enPassant] & pM;

        if (Bitcount(RANK_BIT[kingSquare] & pawnSquares) != 1) {
            return SQUARE_BITS[enPassant];
        }

        int enemyPawn = enPassant + PAWN_PUSH[!side];
        occB ^= pawnSquares;
        PopBit(occB, enemyPawn);

        return PASSANT_PIN_RESULT[SquareOf(getRookAttacks(kingSquare, occB) & potentialPinnerMasks[SquareOf(pawnSquares)][kingSquare] & (rE | qE))];
    }

    ForceInline U64 findPins1(int depth, bool side, U64 occM, U64 occB, U64 bE, U64 rE, U64 qE) {
        U64 pins = 0ULL;
        U64 pinMask, pin;
        int square;
        int kingSquare = king[side];

        U64 pinBoard = (getBishopPins(kingSquare, occB) & (bE | qE))
                    | (getRookPins(kingSquare, occB) & (rE | qE));

        Bitloop(pinBoard) {
            square = SquareOf(pinBoard);

            pinMask = pinMasks[q][square][kingSquare];
            pin = pinMask & occM;
            validAttacksMasks[depth][SquareOf(pin)] = pinMask | SQUARE_BITS[square];
            pins |= pin;
        }
        return pins;
    }

    //faster slightly
    ForceInline U64 findPins(int depth, bool side, U64 occM, U64 occB, U64 bE, U64 rE, U64 qE) {
        U64 pins = 0ULL;
        int kingSquare = king[side];

        iteratePieces(depth, side, bE & BISHOP_XRAYS[kingSquare], b, kingSquare, pins, occM, occB);
        iteratePieces(depth, side, rE & ROOK_XRAYS[kingSquare], r, kingSquare, pins, occM, occB);
        iteratePieces(depth, side, qE & QUEEN_XRAYS[kingSquare], q, kingSquare, pins, occM, occB);

        return pins;
    }

    ForceInline void iteratePieces(int depth, bool side, U64 pieces, int pieceType, int kingSquare, U64& pins, U64 occM, U64 occB) {
        int sliderSquare;
        U64 pinMask, pinnedPieces;

        Bitloop(pieces)
        {
            sliderSquare = SquareOf(pieces);

            pinMask = pinMasks[pieceType][sliderSquare][kingSquare];
            pinnedPieces = pinMask & occB;

            if (Bitcount(pinnedPieces) == 1 && (pinMask & occM)) {
                validAttacksMasks[depth][SquareOf(pinnedPieces)] = pinMask | SQUARE_BITS[sliderSquare];
                pins |= pinnedPieces;
            }
        }
    }

    ForceInline void findPotentialDiscovers(bool side, U64 occB, U64 bM, U64 rM, U64 qM, U64& bDis, U64& rDis) {
        int kingSquare = king[!side];

        bDis = getBishopPins(kingSquare, occB) & (bM | qM);
        rDis = getRookPins(kingSquare, occB) & (rM | qM);
    }

    //TODO see if can improve
    ForceInline void discoverCheck(bool side, int square, U64 bDis, U64 rDis, U64 occB) {
        int enemyKingSquare = king[!side];

        U64 pinner = potentialPinnerMasks[square][enemyKingSquare] & (bDis|rDis);
        if (pinner) {
            int pinnerSquare = SquareOf(pinner);
            if (!(pinMasks[q][pinnerSquare][enemyKingSquare] & occB)) {
                checkSquares[checkIndex] = pinnerSquare;
                checkPieces[checkIndex] = q; //checkPiece is only used for the pinMask between king& checkPiece, therefore "q" works
                checkIndex++;
            }
        }
    }

    ForceInline void discoverCheckPassant(bool side, int square, int ePawnSquare, U64 bDis, U64 occM, U64 occB, U64 bM, U64 rM, U64 qM) {
        int enemyKingSquare = king[!side];

        U64 pinnerMask = potentialPinnerMasks[square][enemyKingSquare] & (((bM|qM) & BISHOP_XRAYS[enemyKingSquare]) | ((rM | qM) & ROOK_XRAYS[enemyKingSquare]));
        if (pinnerMask) {
            int potentialPinnerSquare = square > enemyKingSquare ? SquareOf(pinnerMask) : Ms1b(pinnerMask);    
            U64 pinMask = pinMasks[q][potentialPinnerSquare][enemyKingSquare];
            if (!(pinMask & occB)) {
                checkSquares[checkIndex] = potentialPinnerSquare;
                checkPieces[checkIndex] = q; //checkPiece is only used for the pinMask between king & checkPiece, therefore "q" works
                checkIndex++;
            }
        }

        //the ePawnSquare can only discover in diagonals, so we use bishop masks
        U64 pinner = potentialPinnerMasks[ePawnSquare][enemyKingSquare] & bDis;
        if (pinner) {
            int pinnerSquare = SquareOf(pinner);
            if (!(pinMasks[b][pinnerSquare][enemyKingSquare] & occB)) {
                checkSquares[checkIndex] = pinnerSquare;
                checkPieces[checkIndex] = q; //checkPiece is only used for the pinMask between king & checkPiece, therefore "q" works
                checkIndex++;
            }
        }

        //BISHOP_XRAYS because the ePawnSquare can only discover in diagonals
        //pinnerMask = potentialPinnerMasks[ePawnSquare][enemyKingSquare] & occM & BISHOP_XRAYS[ePawnSquare];

        //if (pinnerMask) {
        //    int potentialPinnerSquare = ePawnSquare > enemyKingSquare ? SquareOf(pinnerMask) : Ms1b(pinnerMask);
        //    //TODO REPLACE p WITH CORRECT PIECE
        //    U64 pinMask = pinMasks[p][potentialPinnerSquare][enemyKingSquare];
        //    if (pinMask && !(pinMask & occB)) {
        //        checkSquares[checkIndex] = potentialPinnerSquare;
        //        checkPieces[checkIndex] = b; //check is either bishop or queen, but checkPiece is only used for the pinMask between king & checkPiece, therefore "b" works
        //    }
        //}
    }
}