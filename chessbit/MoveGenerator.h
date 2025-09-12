#ifndef MOVEGENERATOR_H
#define MOVEGENERATOR_H

#include "MoveArray.h"
#include <vector>

using namespace game;
using namespace defs;

struct MoveInfo {
    int type;

    int movedPiece;

    int from;
    int to;
    int enPassant;
    int deadPiece;

    int rookFrom;
    int rookTo;

    int promotedPiece;

    int mvv_lva;
};

namespace movegen {

    /****************************************************
    * Naming conventions
    *
    * pM -> kM	|	pawn to king, current side
    * pE -> kE	|	pawn to king, opposite side
    * occM|E|B	|	occupancies (current, opposite, both)
    * bDis|rDis	|	bishop & rook potential discovers
    * checkSquare|chP	|	check square & check piece
    *****************************************************/

    //union of pinMask and the slider piece square, where index is the square of the pinned piece
    static inline U64 validAttacksMasks[100][64];

    enum Type {
        Pawn, Knight, Bishop, Rook, Queen, King, EnPassant, Castling,
        PromotionKnight, PromotionBishop, PromotionRook, PromotionQueen,
        PawnCapture, KnightCapture, BishopCapture, RookCapture, QueenCapture, KingCapture,
        PromotionKnightCapture, PromotionBishopCapture, PromotionRookCapture, PromotionQueenCapture
    };

    //5949 is the maximum possible move count in one game
    extern U64 occupanciesSaved[5949][3];
    extern int castlingPermissionsSaved[5949];
    extern int enPassantSaved[5949];
    extern int checksSaved[5949];

    enum class Piece {
        Pawn, Knight, Bishop, Rook, Queen, King
    };

    struct BoardState {
        const U64 pM;
        const U64 nM;
        const U64 bM;
        const U64 rM;
        const U64 qM;
        const U64 kM;

        const U64 pE;
        const U64 nE;
        const U64 bE;
        const U64 rE;
        const U64 qE;
        const U64 kE;

        const U64 kMA;
        const U64 kEA;

        const U64 occM;
        const U64 occE;
        const U64 occB;

        const U64 checks;
        const int casPerms;
        const int enPassant;

        constexpr BoardState(
            U64 pM, U64 nM, U64 bM, U64 rM, U64 qM, U64 kM,
            U64 pE, U64 nE, U64 bE, U64 rE, U64 qE, U64 kE,
            U64 kMA, U64 kEA,
            U64 occM, U64 occE, U64 occB,
            U64 checks, int casPerms, int enPassant) :
            pM(pM), nM(nM), bM(bM), rM(rM), qM(qM), kM(kM),
            pE(pE), nE(nE), bE(bE), rE(rE), qE(qE), kE(kE),
            kMA(kMA), kEA(kEA),
            occM(occM),
            occE(occE),
            occB(occB),
            checks(checks),
            casPerms(casPerms),
            enPassant(enPassant)
        {

        }

        ForceInline U64 sliderChecks(U64 bM, U64 rM, U64 qM, U64 occB, int kES) {
            return (getBishopAttacks(kES, occB) & (bM | qM)) | (getRookAttacks(kES, occB) & (rM | qM));
        }

        template <Piece piece, bool side, bool capture>
        ForceInline BoardState make(int from, int to, const BoardState& board, int kES) {
            const U64 move = (1ULL << from) | (1ULL << to);

            U64 pM = board.pM;
            U64 nM = board.nM;
            U64 bM = board.bM;
            U64 rM = board.rM;
            U64 qM = board.qM;
            U64 kM = board.kM;

            int casPerms = board.casPerms;

            const U64 occM = board.occM ^ move;
            U64 occE = board.occE;

            U64 checks = 0ULL;

            if constexpr (Piece::Pawn == piece) {
                pM ^= move;
                checks |= PAWN_CAPTURES[!side][kES] & pM;
            }
            if constexpr (Piece::Knight == piece) {
                nM ^= move;
                checks |= KNIGHT_ATTACKS[kES] & nM;
            }
            if constexpr (Piece::Bishop == piece)   bM ^= move;
            if constexpr (Piece::Rook == piece) {
                rM ^= move;
                casPerms &= NO_CASTLE_ROOK[from];
            }
            if constexpr (Piece::Queen == piece)    qM ^= move;
            if constexpr (Piece::King == piece)     kM ^= move;

            if constexpr (capture) {
                occE ^= (1ULL << to);
                casPerms &= NO_CASTLE_ROOK[to];

                const U64 occB = occM | occE;
                checks |= sliderChecks(bM, rM, qM, occB, kES);
                if constexpr (Piece::King == piece)         return BoardState(board.pE & occE, board.nE & occE, board.bE & occE, board.rE & occE, board.qE & occE, board.kE, pM, nM, bM, rM, qM, kM, board.kEA, getKingAttacks(to), occE, occM, occB, checks, casPerms, noSquare);
                else                                        return BoardState(board.pE & occE, board.nE & occE, board.bE & occE, board.rE & occE, board.qE & occE, board.kE, pM, nM, bM, rM, qM, kM, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare);
            }
            else {
                const U64 occB = occM | occE;
                checks |= sliderChecks(bM, rM, qM, occB, kES);

                if constexpr (Piece::King == piece)         return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, nM, bM, rM, qM, kM, board.kEA, getKingAttacks(to), occE, occM, occB, checks, casPerms, noSquare);
                else                                        return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, nM, bM, rM, qM, kM, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare);
            }
        }

        template <Piece piece, bool capture>
        ForceInline BoardState makePromotion(int from, int to, const BoardState& board, int kES) {
            const U64 f = (1ULL << from);
            const U64 t = (1ULL << to);

            const U64 pM = board.pM ^ f;
            U64 nM = board.nM;
            U64 bM = board.bM;
            U64 rM = board.rM;
            U64 qM = board.qM;

            const U64 occM = board.occM ^ (f | t);
            U64 occE = board.occE;

            U64 checks = 0ULL;

            if constexpr (Piece::Knight == piece) {
                nM ^= t;
                checks |= KNIGHT_ATTACKS[kES] & nM;
            }
            if constexpr (Piece::Bishop == piece)   bM ^= t;
            if constexpr (Piece::Rook == piece)     rM ^= t;
            if constexpr (Piece::Queen == piece)    qM ^= t;

            if constexpr (capture) {
                occE ^= t;
                const int casPerms = board.casPerms & NO_CASTLE_ROOK[to];

                const U64 occB = occM | occE;
                checks |= sliderChecks(bM, rM, qM, occB, kES);
                return BoardState(board.pE & occE, board.nE & occE, board.bE & occE, board.rE & occE, board.qE & occE, board.kE, pM, nM, bM, rM, qM, board.kM, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare);
            }
            else {
                const U64 occB = occM | occE;
                checks |= sliderChecks(bM, rM, qM, occB, kES);
                return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, nM, bM, rM, qM, board.kM, board.kEA, board.kMA, occE, occM, occB, checks, board.casPerms, noSquare);
            }
        }

        template <bool side>
        ForceInline BoardState makeDoublePush(int from, int to, const BoardState& board, int kES) {
            const U64 move = (1ULL << from) | (1ULL << to);

            const U64 pM = board.pM ^ move;
            const U64 occM = board.occM ^ move;

            const U64 occB = occM | board.occE;

            const U64 checks = (PAWN_CAPTURES[!side][kES] & pM) | sliderChecks(board.bM, board.rM, board.qM, occB, kES);

            return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, board.nM, board.bM, board.rM, board.qM, board.kM, board.kEA, board.kMA, board.occE, occM, occB, checks, board.casPerms, from + PAWN_PUSH[side]);
        }

        template <bool side>
        ForceInline BoardState makeEnPassant(int from, int to, const BoardState& board, int kES) {
            const U64 move = (1ULL << from) | (1ULL << to);

            const U64 pM = board.pM ^ move;
            const U64 occM = board.occM ^ move;

            const U64 ePawnSquare = (1ULL << (to + PAWN_PUSH[!side]));
            const U64 pE = board.pE ^ ePawnSquare;
            const U64 occE = board.occE ^ ePawnSquare;

            const U64 occB = occM | occE;

            const U64 checks = (PAWN_CAPTURES[!side][kES] & pM) | sliderChecks(board.bM, board.rM, board.qM, occB, kES);

            return BoardState(pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, board.nM, board.bM, board.rM, board.qM, board.kM, board.kEA, board.kMA, occE, occM, occB, checks, board.casPerms, noSquare);
        }

        template <int castlingSide>
        ForceInline constexpr U64 rookSwitch() {
            if constexpr (castlingSide == 0) return 0xa000000000000000;
            if constexpr (castlingSide == 1) return 0x900000000000000;
            if constexpr (castlingSide == 2) return 0xa0;
            if constexpr (castlingSide == 3) return 0x9;
        }

        template <int castlingSide>
        ForceInline constexpr U64 kingSwitch() {
            if constexpr (castlingSide == 0) return 0x5000000000000000;
            if constexpr (castlingSide == 1) return 0x1400000000000000;
            if constexpr (castlingSide == 2) return 0x50;
            if constexpr (castlingSide == 3) return 0x14;
        }

        template <int castlingSide>
        ForceInline constexpr U64 bothSwitch() {
            if constexpr (castlingSide == 0) return 0xa000000000000000 | 0x5000000000000000;
            if constexpr (castlingSide == 1) return 0x900000000000000 | 0x1400000000000000;
            if constexpr (castlingSide == 2) return 0xa0 | 0x50;
            if constexpr (castlingSide == 3) return 0x9 | 0x14;
        }

        template <int castlingSide>
        ForceInline BoardState makeCastling(const BoardState& board, int kES) {
            const U64 kM = board.kM ^ kingSwitch<castlingSide>();
            const U64 rM = board.rM ^ rookSwitch<castlingSide>();

            const U64 occM = board.occM ^ bothSwitch<castlingSide>();
            const U64 occB = occM | board.occE;

            const U64 checks = getRookAttacks(kES, occB) & rM;

            const int to = CASTLING_KING_TARGET_SQUARE[castlingSide];

            return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, board.pM, board.nM, board.bM, rM, board.qM, kM, board.kEA, getKingAttacks(to), board.occE, occM, occB, checks, board.casPerms, noSquare);
        }
    };

    ForceInline U64 sliderChecks(U64 bM, U64 rM, U64 qM, U64 occB, int kES) {
        return (getBishopAttacks(kES, occB) & (bM | qM)) | (getRookAttacks(kES, occB) & (rM | qM));
    }

    template <Piece piece, bool side, bool capture>
    ForceInline void make(int from, int to, U64& occM, U64& occE, U64& occB, U64& movP, U64 bM, U64 rM, U64 qM, int kES, U64& checks) {
        const U64 move = (1ULL << from) | (1ULL << to);

        movP ^= move;
        occM ^= move;

        if constexpr (capture) occE ^= (1ULL << to);

        occB = occM | occE;

        if constexpr (Piece::Bishop == piece)       checks = sliderChecks(movP, rM, qM, occB, kES);
        else if constexpr (Piece::Rook == piece)    checks = sliderChecks(bM, movP, qM, occB, kES);
        else if constexpr (Piece::Queen == piece)   checks = sliderChecks(bM, rM, movP, occB, kES);
        else                                        checks = sliderChecks(bM, rM, qM, occB, kES);

        if constexpr (Piece::Pawn == piece)     checks |= PAWN_CAPTURES[!side][kES] & movP;
        if constexpr (Piece::Knight == piece)   checks |= KNIGHT_ATTACKS[kES] & movP;
    }

    template <Piece piece, bool side, bool capture>
    ForceInline void makePromotion(int from, int to, U64& occM, U64& occE, U64& occB, U64& pM, U64& promoP, U64 bM, U64 rM, U64 qM, int kES, U64& checks) {
        const U64 f = (1ULL << from);
        const U64 t = (1ULL << to);

        pM ^= f;
        promoP ^= t;
        occM ^= (f | t);

        if constexpr (capture) occE ^= t;

        occB = occM | occE;

        if constexpr (Piece::Bishop == piece)       checks = sliderChecks(promoP, rM, qM, occB, kES);
        else if constexpr (Piece::Rook == piece)    checks = sliderChecks(bM, promoP, qM, occB, kES);
        else if constexpr (Piece::Queen == piece)   checks = sliderChecks(bM, rM, promoP, occB, kES);
        else                                        checks = sliderChecks(bM, rM, qM, occB, kES);

        if constexpr (Piece::Knight == piece)   checks |= KNIGHT_ATTACKS[kES] & promoP;
    }

    template <bool side>
    ForceInline void makePawnEnPassant(int from, int to, U64& occM, U64& occE, U64& occB, U64& pM, U64 bM, U64 rM, U64 qM, U64& pE, int kES, U64& checks) {
        const U64 move = (1ULL << from) | (1ULL << to);

        pM ^= move;
        occM ^= move;

        U64 ePawnSquare = (1ULL << (to + PAWN_PUSH[!side]));
        pE ^= ePawnSquare;
        occE ^= ePawnSquare;

        occB = occM | occE;

        checks = PAWN_CAPTURES[!side][kES] & pM;
        checks |= sliderChecks(bM, rM, qM, occB, kES);
    }

    template <int castlingSide>
    ForceInline U64 rookSwitch() {
        if constexpr (castlingSide == 0) return 0xa000000000000000;
        if constexpr (castlingSide == 1) return 0x900000000000000;
        if constexpr (castlingSide == 2) return 0xa0;
        if constexpr (castlingSide == 3) return 0x9;
    }

    template <int castlingSide>
    ForceInline U64 kingSwitch() {
        if constexpr (castlingSide == 0) return 0x5000000000000000;
        if constexpr (castlingSide == 1) return 0x1400000000000000;
        if constexpr (castlingSide == 2) return 0x50;
        if constexpr (castlingSide == 3) return 0x14;
    }

    template <int castlingSide>
    ForceInline void makeCastling(U64& occM, U64& occE, U64& occB, U64& kM, U64& rM, int kES, U64& checks) {
        U64 kSwitch = kingSwitch<castlingSide>();
        U64 rSwitch = rookSwitch<castlingSide>();

        kM ^= kSwitch;
        rM ^= rSwitch;

        occM ^= (kSwitch | rSwitch);
        occB = occM | occE;

        checks = getRookAttacks(kES, occB) & rM;
    }

    ForceInline void unmake(bool side, int from, int to, int movedPiece, int deadPiece) {
        MoveBit(pieces[side][movedPiece], to, from);

        boardPieces[from] = movedPiece;

        SetBit(pieces[!side][deadPiece], to);
        boardPieces[to] = deadPiece;
    }

    ForceInline void unmakeKing(bool side, int from, int to, int deadPiece) {
        unmake(side, from, to, k, deadPiece);
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

        //move rook
        MoveBit(pieces[side][r], rookTo, rookFrom);
        boardPieces[rookTo] = noPiece;
        boardPieces[rookFrom] = r;
    }

    template <bool side>
    static inline void makeMove(MoveInfo& move) {
        movesPlayed[moveCount] = move;
        occupanciesSaved[moveCount][white] = occupancies[white];
        occupanciesSaved[moveCount][black] = occupancies[black];
        occupanciesSaved[moveCount][both] = occupancies[both];
        castlingPermissionsSaved[moveCount] = castlingPermissions;
        enPassantSaved[moveCount] = enPassant;
        checksSaved[moveCount] = checks;

        moveCount++;

        enPassant = move.enPassant;

        U64& pM = pieces[side][p];
        U64& nM = pieces[side][n];
        U64& bM = pieces[side][b];
        U64& rM = pieces[side][r];
        U64& qM = pieces[side][q];
        U64& kM = pieces[side][k];

        U64& pE = pieces[!side][p];
        U64& kE = pieces[!side][k];

        int kES = SquareOf(kE);

        switch (move.type) {
        case Pawn: make<Piece::Pawn, side, false>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], pM, bM, rM, qM, kES, checks); break;
        case PawnCapture: make<Piece::Pawn, side, true>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], pM, bM, rM, qM, kES, checks); break;
        case Knight: make<Piece::Knight, side, false>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], nM, bM, rM, qM, kES, checks); break;
        case KnightCapture: make<Piece::Knight, side, true>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], nM, bM, rM, qM, kES, checks); break;
        case Bishop: make<Piece::Bishop, side, false>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], bM, bM, rM, qM, kES, checks); break;
        case BishopCapture: make<Piece::Bishop, side, true>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], bM, bM, rM, qM, kES, checks); break;
        case Rook: {
            make<Piece::Rook, side, false>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], rM, bM, rM, qM, kES, checks);
            castlingPermissions &= NO_CASTLE_ROOK[move.from]; break;
        }
        case RookCapture: {
            make<Piece::Rook, side, true>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], rM, bM, rM, qM, kES, checks);
            castlingPermissions &= NO_CASTLE_ROOK[move.from]; break;
        }
        case Queen: make<Piece::Queen, side, false>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], qM, bM, rM, qM, kES, checks); break;
        case QueenCapture: make<Piece::Queen, side, true>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], qM, bM, rM, qM, kES, checks); break;
        case King: {
            make<Piece::King, side, false>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], kM, bM, rM, qM, kES, checks);
            castlingPermissions &= NO_CASTLE[side]; break;
        }
        case KingCapture: {
            make<Piece::King, side, true>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], kM, bM, rM, qM, kES, checks);
            castlingPermissions &= NO_CASTLE[side]; break;
        }
        case PromotionKnight: makePromotion<Piece::Knight, side, false>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], pM, nM, bM, rM, qM, kES, checks); break;
        case PromotionKnightCapture: makePromotion<Piece::Knight, side, true>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], pM, nM, bM, rM, qM, kES, checks); break;
        case PromotionBishop: makePromotion<Piece::Bishop, side, false>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], pM, bM, bM, rM, qM, kES, checks); break;
        case PromotionBishopCapture: makePromotion<Piece::Bishop, side, true>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], pM, bM, bM, rM, qM, kES, checks); break;
        case PromotionRook: makePromotion<Piece::Rook, side, false>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], pM, rM, bM, rM, qM, kES, checks); break;
        case PromotionRookCapture: makePromotion<Piece::Rook, side, true>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], pM, rM, bM, rM, qM, kES, checks); break;
        case PromotionQueen: makePromotion<Piece::Queen, side, false>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], pM, qM, bM, rM, qM, kES, checks); break;
        case PromotionQueenCapture: makePromotion<Piece::Rook, side, true>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], pM, qM, bM, rM, qM, kES, checks); break;
        case EnPassant: {
            makePawnEnPassant<side>(move.from, move.to, occupancies[side], occupancies[!side], occupancies[both], pM, bM, rM, qM, pE, kES, checks);
            int ePawnSquare = move.to + PAWN_PUSH[!side];
            boardPieces[ePawnSquare] = noPiece; break;
        }
        case Castling: {
            if (move.rookFrom == CASTLE_ROOK_KING[side]) {
                makeCastling<CASTLING_SIDE_K[side]>(occupancies[side], occupancies[!side], occupancies[both], kM, rM, kES, checks);
            }
            else {
                makeCastling<CASTLING_SIDE_Q[side]>(occupancies[side], occupancies[!side], occupancies[both], kM, rM, kES, checks);
            }
            castlingPermissions &= NO_CASTLE[side];
            boardPieces[move.rookFrom] = noPiece;
            boardPieces[move.rookTo] = r;
        }
        }

        if (move.promotedPiece != noPiece) {
            boardPieces[move.from] = noPiece;
            boardPieces[move.to] = move.promotedPiece;
        }
        else {
            boardPieces[move.from] = noPiece;
            boardPieces[move.to] = move.movedPiece;
        }

        if (move.deadPiece != noPiece && move.type != EnPassant) {
            castlingPermissions &= NO_CASTLE_ROOK[move.to];
            pieces[!side][move.deadPiece] ^= (1ULL << move.to);
        }

        game::side = !side;
    }

    static inline void unmakeMove(MoveInfo& move) {
        side = !side;
        moveCount--;

        occupancies[white] = occupanciesSaved[moveCount][white];
        occupancies[black] = occupanciesSaved[moveCount][black];
        occupancies[both] = occupanciesSaved[moveCount][both];
        castlingPermissions = castlingPermissionsSaved[moveCount];
        checks = checksSaved[moveCount];
        enPassant = enPassantSaved[moveCount];

        switch (move.type) {
        case Pawn:
        case PawnCapture:
        case Knight:
        case KnightCapture:
        case Bishop:
        case BishopCapture:
        case Rook:
        case RookCapture:
        case Queen:
        case QueenCapture: unmake(side, move.from, move.to, move.movedPiece, move.deadPiece); break;
        case King:
        case KingCapture: unmakeKing(side, move.from, move.to, move.deadPiece); break;
        case PromotionKnight:
        case PromotionKnightCapture:
        case PromotionBishop:
        case PromotionBishopCapture:
        case PromotionRook:
        case PromotionRookCapture:
        case PromotionQueen:
        case PromotionQueenCapture: unmakePromotion(side, move.from, move.to, move.deadPiece, move.promotedPiece); break;
        case EnPassant: unmakePawnEnPassant(side, move.from, move.to); break;
        case Castling: unmakeCastling(side, move.from, move.to, move.rookFrom, move.rookTo); break;
        }
    }

    template <bool side, bool wKMoved, bool bKMoved>
    ForceInline void filterKingAttacks(U64 occM, U64 occB, int kMS, U64& kingAttacks, U64& castleAttacks, U64 pE, U64 nE, U64 bE, U64 rE, U64 qE, U64 kEA, U64 mask) {
        U64 attacks = 0ULL;
        if constexpr ((side == white && !wKMoved) || (side == black && !bKMoved)) {
            attacks |= ~getPawnKingAttacksCastle<side>(pE);
        }
        else {
            attacks |= ~getPawnKingAttacks<!side>(kMS, pE);
        }

        attacks |= kEA;

        //remove king to avoid collisions, as it should not be considered when checking for threats
        PopBit(occB, kMS);

        U64 bitboard;
        if constexpr ((side == white && !wKMoved) || (side == black && !bKMoved)) {
            bitboard = nE & KNIGHT_ATTACK_ZONE_CASTLE[side];
        }
        else {
            bitboard = nE & KNIGHT_ATTACK_ZONES[kMS];
        }
        Bitloop(bitboard) {
            attacks |= getKnightAttacks(SquareOf(bitboard));
        }

        if constexpr ((side == white && !wKMoved) || (side == black && !bKMoved)) {
            bitboard = (bE | qE) & getBishopAttackZoneCastle<side>(occM);
        }
        else {
            bitboard = (bE | qE) & getBishopAttackZone(kMS, occM, mask);
        }
        Bitloop(bitboard) {
            attacks |= getBishopAttacks(SquareOf(bitboard), occB);
        }

        if constexpr ((side == white && !wKMoved) || (side == black && !bKMoved)) {
            bitboard = (rE | qE) & getRookAttackZoneCastle<side>(occM);
        }
        else {
            bitboard = (rE | qE) & getRookAttackZone(kMS, occM, mask);
        }
        Bitloop(bitboard) {
            attacks |= getRookAttacks(SquareOf(bitboard), occB);
        }

        kingAttacks &= ~attacks;

        if constexpr ((side == white && !wKMoved) || (side == black && !bKMoved)) {
            castleAttacks = attacks;
        }
    }

    template <int castlingSide>
    ForceInline bool castle(int casPerm, U64 occE, U64 occB, U64 nE, U64 bE, U64 rE, U64 qE, U64 attacks) {
        return !(!(casPerm & CASTLING[castlingSide]) | (CASTLING_OCCUPIED_SQUARES[castlingSide] & occB) | (attacks & CASTLING_PASSING_SQUARES[castlingSide]));
        /*if (!(casPerm & CASTLING[castlingSide])) {
            return false;
        }
        if ((CASTLING_OCCUPIED_SQUARES[castlingSide] & occB) | (attacks & CASTLING_PASSING_SQUARES[castlingSide])) {
            return false;
        }
        return true;*/
    }

    template <bool side>
    ForceInline U64 passantPinMask(int enPassant, int pawnSquare, U64 occB, U64 kM, U64 rE, U64 qE, int kMS) {
        if (!(EN_PASSANT_RANK[side] & kM)) {
            return FULL_BOARD;
        }

        int enemyPawn = enPassant + PAWN_PUSH[!side];
        PopBit(occB, enemyPawn);
        PopBit(occB, pawnSquare);

        return PASSANT_PIN_RESULT[SquareOf(getRookAttacks(kMS, occB) & (rE | qE))];
    }

    template <int depth>
    ForceInline U64 iteratePieces(U64 pieces, U64 occB, int kMS) {
        U64 pins = 0ULL;

        Bitloop(pieces)
        {
            int sliderSquare = SquareOf(pieces);

            U64 pinMask = PIN_MASKS[sliderSquare][kMS];
            U64 pinnedPieces = pinMask & occB;

            if (Bitcount(pinnedPieces) == 1) {
                U64 attacks = pinMask | SQUARE_BITS[sliderSquare];
                validAttacksMasks[depth][SquareOf(pinnedPieces)] = attacks ^ pinnedPieces;
                pins |= attacks;
            }
        }

        return pins;
    }

    template <int depth>
    ForceInline U64 findBishopPins(U64 occB, U64 bE, U64 qE, int kMS) {
        return iteratePieces<depth>(((bE | qE) & BISHOP_XRAYS[kMS]), occB, kMS);
    }

    template <int depth>
    ForceInline U64 findRookPins(U64 occB, U64 rE, U64 qE, int kMS) {
        return iteratePieces<depth>(((rE | qE) & ROOK_XRAYS[kMS]), occB, kMS);
    }

    template <bool side>
    ForceInline constexpr U64 pawnsAtkLeft(U64 pM) {
        if constexpr (side == white) return pM >> 9;
        return pM << 7;
    }

    template <bool side>
    ForceInline constexpr U64 pawnsAtkRight(U64 pM) {
        if constexpr (side == white) return pM >> 7;
        return pM << 9;
    }

    template <bool side>
    ForceInline constexpr U64 pawnsAtkForward(U64 pM) {
        if constexpr (side == white) return pM >> 8;
        return pM << 8;
    }

    template <bool side, bool wKMoved, bool bKMoved>
    static inline void generateMoves(MoveArray& moves) {
        int from, to, deadPiece;
        U64 bitboard, attacks, pinMask;
        U64 castleAttacks = 0ULL;

        int kMS = SquareOf(pieces[side][k]);
        int kES = SquareOf(pieces[!side][k]);

        if (checks) {

            /*

                KING MOVES

            */
            int checkSquare = SquareOf(checks);
            U64 mask = getKingAttacks(kMS);
            //inverted PIN_MASKS, because the king cannot move in the attack ray of the check pieces
            attacks = mask & ~occupancies[side] & ~PIN_MASKS[checkSquare][kMS] & ~PIN_MASKS[Ms1b(checks)][kMS];
            filterKingAttacks<side, true, true>(occupancies[side], occupancies[both], kMS, attacks, castleAttacks, pieces[!side][p], pieces[!side][n], pieces[!side][b], pieces[!side][r], pieces[!side][q], getKingAttacks(kES), mask);
            Bitloop(attacks)
            {
                to = SquareOf(attacks);
                deadPiece = boardPieces[to];
                moves.king(kMS, to, deadPiece);
            }

            //if there is no second check, we need to check for other pieces
            if (Bitcount(checks) == 1) {
                //remove the bit of the check piece for performance, because it cannot possibly pin a piece
                //PopBit(pieces[!side][checkPiece], checkSquare);
                U64 bPins = findBishopPins<0>(occupancies[both], pieces[!side][b], pieces[!side][q], kMS);
                U64 rPins = findRookPins<0>(occupancies[both], pieces[!side][r], pieces[!side][q], kMS);
                U64 allPins = bPins | rPins;
                //SetBit(pieces[!side][checkPiece], checkSquare);

                //if check piece is pawn or knight, only possible moves are capture of the check piece (+ king moves)
                if (checks & (pieces[!side][p] | pieces[!side][n])) {
                    /*

                       PAWN MOVES

                    */
                    to = checkSquare;
                    deadPiece = boardPieces[to];

                    //en passant
                    bitboard = pieces[side][p] & PASSANT_CAPTURES[enPassant] & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        if (checkSquare == enPassant + PAWN_PUSH[!side]) {
                            moves.enPassant(from, enPassant);
                        }
                    }

                    bitboard = pieces[side][p] & PROMO_RANKS[side] & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = PAWN_CAPTURES[side][from] & checks;
                        if (attacks)
                        {
                            moves.promotion(from, to, deadPiece);
                        }
                    }

                    bitboard = pieces[side][p] & ~PROMO_RANKS[side] & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = PAWN_CAPTURES[side][from] & checks;
                        if (attacks)
                        {
                            moves.pawn(from, to, deadPiece);
                        }
                    }

                    /*

                       KNIGHT MOVES

                    */
                    bitboard = pieces[side][n] & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getKnightAttacks(from) & checks;
                        if (attacks) {
                            moves.knight(from, to, deadPiece);
                        }
                    }

                    /*

                       BISHOP MOVES

                    */
                    bitboard = pieces[side][b] & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getBishopAttacks(from, occupancies[both]) & checks;
                        if (attacks) {
                            moves.bishop(from, to, deadPiece);
                        }
                    }

                    /*

                       ROOK MOVES

                    */
                    bitboard = pieces[side][r] & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getRookAttacks(from, occupancies[both]) & checks;
                        if (attacks) {
                            moves.rook(from, to, deadPiece);
                        }
                    }

                    /*

                       QUEEN MOVES

                    */
                    bitboard = pieces[side][q] & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getQueenAttacks(from, occupancies[both]) & checks;
                        if (attacks) {
                            moves.queen(from, to, deadPiece);
                        }
                    }
                }
                else {
                    //squares between the check piece (included) and the king
                    //those are the only squares that a piece can go to to block the check
                    U64 validSquares = (checks | PIN_MASKS[checkSquare][kMS]);

                    /*

                       PAWN MOVES

                    */
                    U64 pawns = pieces[side][p] & ~allPins;

                    U64 pawnsLeft = pawnsAtkLeft<side>(pawns & ~FIRST_COL) & occupancies[!side] & validSquares;
                    U64 pawnsRight = pawnsAtkRight<side>(pawns & ~LAST_COL) & occupancies[!side] & validSquares;
                    U64 pawnsFwd = pawnsAtkForward<side>(pawns) & ~occupancies[both];
                    U64 pawnsDouble = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~occupancies[both] & validSquares;
                    //mask after double push, to not ignore possible moves
                    pawnsFwd &= validSquares;

                    if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANKS[side]) {
                        U64 promosLeft = pawnsLeft & LAST_RANKS[side];
                        U64 promosRight = pawnsRight & LAST_RANKS[side];
                        U64 promosFwd = pawnsFwd & LAST_RANKS[side];

                        pawnsLeft ^= promosLeft;
                        pawnsRight ^= promosRight;
                        pawnsFwd ^= promosFwd;

                        Bitloop(promosLeft) { to = SquareOf(promosLeft);  from = to + PAWN_RIGHT[!side]; deadPiece = boardPieces[to]; moves.promotion(from, to, deadPiece); }
                        Bitloop(promosRight) { to = SquareOf(promosRight); from = to + PAWN_LEFT[!side]; deadPiece = boardPieces[to]; moves.promotion(from, to, deadPiece); }
                        Bitloop(promosFwd) { to = SquareOf(promosFwd);   from = to + PAWN_PUSH[!side];   deadPiece = boardPieces[to]; moves.promotion(from, to, deadPiece); }
                    }

                    Bitloop(pawnsLeft) {
                        to = SquareOf(pawnsLeft);
                        from = to + PAWN_RIGHT[!side];
                        deadPiece = boardPieces[to];
                        moves.pawn(from, to, deadPiece);
                    }

                    Bitloop(pawnsRight) {
                        to = SquareOf(pawnsRight);
                        from = to + PAWN_LEFT[!side];
                        deadPiece = boardPieces[to];
                        moves.pawn(from, to, deadPiece);
                    }

                    Bitloop(pawnsFwd) {
                        to = SquareOf(pawnsFwd);
                        from = to + PAWN_PUSH[!side];
                        moves.pawn(from, to, noPiece);
                    }

                    Bitloop(pawnsDouble) {
                        to = SquareOf(pawnsDouble);
                        from = to + PAWN_DOUBLE_PUSH[!side];
                        moves.pawn(from, to, noPiece);
                    }

                    /*

                       KNIGHT MOVES

                    */
                    bitboard = pieces[side][n] & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getKnightAttacks(from) & validSquares;
                        Bitloop(attacks) {
                            to = SquareOf(attacks);
                            deadPiece = boardPieces[to];

                            moves.knight(from, to, deadPiece);
                        }
                    }

                    /*

                       BISHOP MOVES

                    */
                    bitboard = pieces[side][b] & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getBishopAttacks(from, occupancies[both]) & validSquares;
                        Bitloop(attacks) {
                            to = SquareOf(attacks);
                            deadPiece = boardPieces[to];

                            moves.bishop(from, to, deadPiece);
                        }
                    }

                    /*

                       ROOK MOVES

                    */
                    bitboard = pieces[side][r] & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getRookAttacks(from, occupancies[both]) & validSquares;
                        Bitloop(attacks) {
                            to = SquareOf(attacks);
                            deadPiece = boardPieces[to];

                            moves.rook(from, to, deadPiece);
                        }
                    }

                    /*

                       QUEEN MOVES

                    */
                    bitboard = pieces[side][q] & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getQueenAttacks(from, occupancies[both]) & validSquares;
                        Bitloop(attacks) {
                            to = SquareOf(attacks);
                            deadPiece = boardPieces[to];

                            moves.queen(from, to, deadPiece);
                        }
                    }
                }
            }

            return;
        }

        U64 bPins = findBishopPins<0>(occupancies[both], pieces[!side][b], pieces[!side][q], kMS);
        U64 rPins = findRookPins<0>(occupancies[both], pieces[!side][r], pieces[!side][q], kMS);
        U64 allPins = bPins | rPins;

        /*

           PAWN MOVES

        */
        //en passant
        bitboard = PASSANT_CAPTURES[enPassant] & pieces[side][p] & ~allPins;
        Bitloop(bitboard) {
            from = SquareOf(bitboard);

            if ((1ULL << enPassant) & passantPinMask<side>(enPassant, from, occupancies[both], pieces[side][k], pieces[!side][r], pieces[!side][q], kMS)) {
                moves.enPassant(from, enPassant);
            }
        }

        bitboard = PASSANT_CAPTURES[enPassant] & pieces[side][p] & allPins;
        Bitloop(bitboard) {
            from = SquareOf(bitboard);
            pinMask = validAttacksMasks[0][from];

            if ((1ULL << enPassant) & pinMask & passantPinMask<side>(enPassant, from, occupancies[both], pieces[side][k], pieces[!side][r], pieces[!side][q], kMS)) {
                moves.enPassant(from, enPassant);
            }
        }

        U64 pawnsAtk = pieces[side][p] & ~rPins;
        U64 pawnsPush = pieces[side][p] & ~bPins;

        U64 pawnsLeft = (pawnsAtkLeft<side>(pawnsAtk & ~bPins & ~FIRST_COL) & occupancies[!side]) | (pawnsAtkLeft<side>(pawnsAtk & bPins & ~FIRST_COL) & occupancies[!side] & bPins);
        U64 pawnsRight = (pawnsAtkRight<side>(pawnsAtk & ~bPins & ~LAST_COL) & occupancies[!side]) | (pawnsAtkRight<side>(pawnsAtk & bPins & ~LAST_COL) & occupancies[!side] & bPins);
        U64 pawnsFwd = (pawnsAtkForward<side>(pawnsPush & ~rPins) & ~occupancies[both]) | (pawnsAtkForward<side>(pawnsPush & rPins) & ~occupancies[both] & rPins);
        U64 pawnsDouble = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~occupancies[both];

        if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANKS[side]) {
            U64 promosLeft = pawnsLeft & LAST_RANKS[side];
            U64 promosRight = pawnsRight & LAST_RANKS[side];
            U64 promosFwd = pawnsFwd & LAST_RANKS[side];

            pawnsLeft ^= promosLeft;
            pawnsRight ^= promosRight;
            pawnsFwd ^= promosFwd;

            Bitloop(promosLeft) { to = SquareOf(promosLeft);  from = to + PAWN_RIGHT[!side]; deadPiece = boardPieces[to]; moves.promotion(from, to, deadPiece); }
            Bitloop(promosRight) { to = SquareOf(promosRight); from = to + PAWN_LEFT[!side]; deadPiece = boardPieces[to]; moves.promotion(from, to, deadPiece); }
            Bitloop(promosFwd) { to = SquareOf(promosFwd);   from = to + PAWN_PUSH[!side];   deadPiece = boardPieces[to]; moves.promotion(from, to, deadPiece); }
        }

        Bitloop(pawnsLeft) {
            to = SquareOf(pawnsLeft);
            from = to + PAWN_RIGHT[!side];
            deadPiece = boardPieces[to];
            moves.pawn(from, to, deadPiece);
        }

        Bitloop(pawnsRight) {
            to = SquareOf(pawnsRight);
            from = to + PAWN_LEFT[!side];
            deadPiece = boardPieces[to];
            moves.pawn(from, to, deadPiece);
        }

        Bitloop(pawnsFwd) {
            to = SquareOf(pawnsFwd);
            from = to + PAWN_PUSH[!side];
            moves.pawn(from, to, noPiece);
        }

        Bitloop(pawnsDouble) {
            to = SquareOf(pawnsDouble);
            from = to + PAWN_DOUBLE_PUSH[!side];
            moves.pawn(from, to, noPiece);
        }

        /*

           KNIGHT MOVES

        */
        bitboard = pieces[side][n] & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getKnightAttacks(from) & ~occupancies[side];
            Bitloop(attacks)
            {
                to = SquareOf(attacks);
                deadPiece = boardPieces[to];

                moves.knight(from, to, deadPiece);
            }
        }

        /*

           BISHOP MOVES

        */
        bitboard = pieces[side][b] & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getBishopAttacks(from, occupancies[both]) & ~occupancies[side];
            Bitloop(attacks)
            {
                to = SquareOf(attacks);
                deadPiece = boardPieces[to];

                moves.bishop(from, to, deadPiece);
            }
        }

        bitboard = pieces[side][b] & allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);
            pinMask = validAttacksMasks[0][from];

            attacks = getBishopAttacks(from, occupancies[both]) & ~occupancies[side] & pinMask;
            Bitloop(attacks)
            {
                to = SquareOf(attacks);
                deadPiece = boardPieces[to];

                moves.bishop(from, to, deadPiece);
            }
        }

        /*

           ROOK MOVES

        */
        bitboard = pieces[side][r] & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getRookAttacks(from, occupancies[both]) & ~occupancies[side];
            Bitloop(attacks)
            {
                to = SquareOf(attacks);
                deadPiece = boardPieces[to];

                moves.rook(from, to, deadPiece);
            }
        }

        bitboard = pieces[side][r] & allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);
            pinMask = validAttacksMasks[0][from];

            attacks = getRookAttacks(from, occupancies[both]) & ~occupancies[side] & pinMask;
            Bitloop(attacks)
            {
                to = SquareOf(attacks);
                deadPiece = boardPieces[to];

                moves.rook(from, to, deadPiece);
            }
        }

        /*

           QUEEN MOVES

        */
        bitboard = pieces[side][q] & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getQueenAttacks(from, occupancies[both]) & ~occupancies[side];
            Bitloop(attacks)
            {
                to = SquareOf(attacks);
                deadPiece = boardPieces[to];

                moves.queen(from, to, deadPiece);
            }
        }

        bitboard = pieces[side][q] & allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);
            pinMask = validAttacksMasks[0][from];

            attacks = getQueenAttacks(from, occupancies[both]) & ~occupancies[side] & pinMask;
            Bitloop(attacks)
            {
                to = SquareOf(attacks);
                deadPiece = boardPieces[to];

                moves.queen(from, to, deadPiece);
            }
        }

        /*

           KING MOVES

        */
        U64 mask = getKingAttacks(kMS);
        attacks = mask & ~occupancies[side];
        filterKingAttacks<side, wKMoved, bKMoved>(occupancies[side], occupancies[both], kMS, attacks, castleAttacks, pieces[!side][p], pieces[!side][n], pieces[!side][b], pieces[!side][r], pieces[!side][q], getKingAttacks(kES), mask);
        Bitloop(attacks)
        {
            to = SquareOf(attacks);
            deadPiece = boardPieces[to];

            moves.king(kMS, to, deadPiece);
        }

        //castling moves
        if (side == white) {
            if (castle<CASTLING_SIDE_K[white]>(castlingPermissions, occupancies[!side], occupancies[both], pieces[!side][n], pieces[!side][b], pieces[!side][r], pieces[!side][q], castleAttacks)) {
                moves.castling(CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_K[white]]);
            }

            if (castle<CASTLING_SIDE_Q[white]>(castlingPermissions, occupancies[!side], occupancies[both], pieces[!side][n], pieces[!side][b], pieces[!side][r], pieces[!side][q], castleAttacks)) {
                moves.castling(CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_Q[white]]);
            }
        }
        else {
            if (castle<CASTLING_SIDE_K[black]>(castlingPermissions, occupancies[!side], occupancies[both], pieces[!side][n], pieces[!side][b], pieces[!side][r], pieces[!side][q], castleAttacks)) {
                moves.castling(CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_K[black]]);
            }

            if (castle<CASTLING_SIDE_Q[black]>(castlingPermissions, occupancies[!side], occupancies[both], pieces[!side][n], pieces[!side][b], pieces[!side][r], pieces[!side][q], castleAttacks)) {
                moves.castling(CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_Q[black]]);
            }
        }
    }

    template <int depth, bool side, bool wKMoved, bool bKMoved>
    struct PerftGenerator;
    
    inline constexpr BoardState empty = BoardState(0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    template <int depth, bool side, bool wKMoved, bool bKMoved, Piece piece, bool capture>
    ForceInline void enumMoves(U64& nodes, U64 moves, int from, const BoardState& board, int kES) {
        if (moves) {
            int to = SquareOf(moves);

            BoardState newBoard = board.make<piece, side, capture>(from, to, board, kES);
            if constexpr (piece == Piece::King) {
                if constexpr (side == white)    nodes += PerftGenerator<depth - 1, !side, true, bKMoved>::generateMoves(newBoard);
                else                            nodes += PerftGenerator<depth - 1, !side, wKMoved, true>::generateMoves(newBoard);
            }
            else {
                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                //PerftGenerator<depth - 1, !side, false, false>::generateMoves(empty);
            }

            enumMoves<depth, side, wKMoved, bKMoved, piece, capture>(nodes, _blsr_u64(moves), from, board, kES);
        }
    }

    template <int depth, bool side, bool wKMoved, bool bKMoved, Piece piece>
    ForceInline void makeMoves(U64& nodes, U64 attacks, int from, const BoardState& board, int kES) {
        int to;
        U64 moves = attacks & ~board.occE;
        //enumMoves<depth, side, wKMoved, bKMoved, piece, false>(nodes, moves, from, board, kES);
        Bitloop(moves) {
            to = SquareOf(moves);

            BoardState newBoard = board.make<piece, side, false>(from, to, board, kES);
            if constexpr (piece == Piece::King) {
                if constexpr (side == white)    nodes += PerftGenerator<depth - 1, !side, true, bKMoved>::generateMoves(newBoard);
                else                            nodes += PerftGenerator<depth - 1, !side, wKMoved, true>::generateMoves(newBoard);
            }
            else {
                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);                
                PerftGenerator<depth - 1, !side, false, false>::generateMoves(empty);
            }
            
        }

        moves = attacks & board.occE;
        //enumMoves<depth, side, wKMoved, bKMoved, piece, true>(nodes, moves, from, board, kES);
        Bitloop(moves) {
            to = SquareOf(moves);

            BoardState newBoard = board.make<piece, side, true>(from, to, board, kES);
            if constexpr (piece == Piece::King) {
                if constexpr (side == white)    nodes += PerftGenerator<depth - 1, !side, true, bKMoved>::generateMoves(newBoard);
                else                            nodes += PerftGenerator<depth - 1, !side, wKMoved, true>::generateMoves(newBoard);
            }
            else {
                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                PerftGenerator<depth - 1, !side, false, false>::generateMoves(empty);
            }
        }
    }

    template <int depth, bool side, bool wKMoved, bool bKMoved, bool capture>
    ForceInline void makePromotionMoves(U64& nodes, int from, int to, const BoardState& board, int kES) {
        BoardState newBoardN = board.makePromotion<Piece::Knight, capture>(from, to, board, kES);
        nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoardN);

        BoardState newBoardB = board.makePromotion<Piece::Bishop, capture>(from, to, board, kES);
        nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoardB);

        BoardState newBoardR = board.makePromotion<Piece::Rook, capture>(from, to, board, kES);
        nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoardR);

        BoardState newBoardQ = board.makePromotion<Piece::Queen, capture>(from, to, board, kES);
        nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoardQ);
    }

    template <int depth, bool side, bool wKMoved, bool bKMoved>
    ForceInline void makePromotionMoves(U64& nodes, U64 attacks, int from, const BoardState& board, int kES) {
        int to;
        U64 moves = attacks & ~board.occE;
        Bitloop(moves) {
            to = SquareOf(moves);

            makePromotionMoves<depth, side, wKMoved, bKMoved, false>(nodes, from, to, board, kES);
        }

        moves = attacks & board.occE;
        Bitloop(moves) {
            to = SquareOf(moves);

            makePromotionMoves<depth, side, wKMoved, bKMoved, true>(nodes, from, to, board, kES);
        }
    }

    template <int depth, bool side, bool wKMoved, bool bKMoved>
    struct PerftGenerator {
        static inline U64 generateMoves(const BoardState& board) {
            if (!board.occB) return 0;
            int from, to;
            U64 bitboard, attacks;

            U64 nodes = 0ULL;
            U64 castleAttacks = 0ULL;

            int kMS = SquareOf(board.kM);
            int kES = SquareOf(board.kE);

            if (board.checks) {
                /*

                    KING MOVES

                */
                int checkSquare = SquareOf(board.checks);

                U64 mask = board.kMA;
                //inverted PIN_MASKS, because the king cannot move in the attack ray of the check pieces
                attacks = mask & ~board.occM & ~PIN_MASKS[checkSquare][kMS] & ~PIN_MASKS[Ms1b(board.checks)][kMS];
                filterKingAttacks<side, true, true>(board.occM, board.occB, kMS, attacks, castleAttacks, board.pE, board.nE, board.bE, board.rE, board.qE, board.kEA, mask);
                makeMoves<depth, side, wKMoved, bKMoved, Piece::King>(nodes, attacks, kMS, board, kES);

                //if there is no second check, we need to check for other pieces
                if (Bitcount(board.checks) == 1) {
                    //remove the bit of the check piece for performance, because it cannot possibly pin a piece
                    U64 bPins = findBishopPins<depth>(board.occB, board.bE, board.qE, kMS);
                    U64 rPins = findRookPins<depth>(board.occB, board.rE, board.qE, kMS);
                    U64 allPins = bPins | rPins;
                    //if check piece is pawn or knight, only possible moves are capture of the check piece (+ king moves)
                    if (board.checks & (board.pE | board.nE)) {
                        /*

                           PAWN MOVES

                        */
                        to = checkSquare;

                        //en passant
                        bitboard = board.pM & PASSANT_CAPTURES[board.enPassant] & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            if (checkSquare == board.enPassant + PAWN_PUSH[!side]) {
                                BoardState newBoard = board.makeEnPassant<side>(from, board.enPassant, board, kES);
                                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                            }
                        }

                        bitboard = board.pM & PROMO_RANKS[side] & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = PAWN_CAPTURES[side][from] & board.checks;
                            if (attacks)
                            {
                                makePromotionMoves<depth, side, wKMoved, bKMoved, true>(nodes, from, to, board, kES);
                            }
                        }

                        bitboard = board.pM & ~PROMO_RANKS[side] & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = PAWN_CAPTURES[side][from] & board.checks;
                            if (attacks)
                            {
                                BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                            }
                        }

                        /*

                           KNIGHT MOVES

                        */
                        bitboard = board.nM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getKnightAttacks(from) & board.checks;
                            if (attacks) {
                                BoardState newBoard = board.make<Piece::Knight, side, true>(from, to, board, kES);
                                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                            }
                        }

                        /*

                           BISHOP MOVES

                        */
                        bitboard = board.bM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getBishopAttacks(from, board.occB) & board.checks;
                            if (attacks) {
                                BoardState newBoard = board.make<Piece::Bishop, side, true>(from, to, board, kES);
                                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                            }
                        }

                        /*

                           ROOK MOVES

                        */
                        bitboard = board.rM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getRookAttacks(from, board.occB) & board.checks;
                            if (attacks) {
                                BoardState newBoard = board.make<Piece::Rook, side, true>(from, to, board, kES);
                                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                            }
                        }

                        /*

                           QUEEN MOVES

                        */
                        bitboard = board.qM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getQueenAttacks(from, board.occB) & board.checks;
                            if (attacks) {
                                BoardState newBoard = board.make<Piece::Queen, side, true>(from, to, board, kES);
                                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                            }
                        }

                    }
                    else {
                        //squares between the check piece (included) and the king
                        //those are the only squares that a piece can go to to block the check
                        U64 validSquares = (board.checks | PIN_MASKS[checkSquare][kMS]);

                        /*

                           PAWN MOVES

                        */
                        U64 pawns = board.pM & ~allPins;

                        U64 pawnsLeft = pawnsAtkLeft<side>(pawns & ~FIRST_COL) & board.occE & validSquares;
                        U64 pawnsRight = pawnsAtkRight<side>(pawns & ~LAST_COL) & board.occE & validSquares;
                        U64 pawnsFwd = pawnsAtkForward<side>(pawns) & ~board.occB;
                        U64 pawnsDouble = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~board.occB & validSquares;
                        //mask after double push, to not ignore possible moves
                        pawnsFwd &= validSquares;

                        if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANKS[side]) {
                            U64 promosLeft = pawnsLeft & LAST_RANKS[side];
                            U64 promosRight = pawnsRight & LAST_RANKS[side];
                            U64 promosFwd = pawnsFwd & LAST_RANKS[side];

                            pawnsLeft ^= promosLeft;
                            pawnsRight ^= promosRight;
                            pawnsFwd ^= promosFwd;

                            Bitloop(promosLeft) { to = SquareOf(promosLeft);  from = to + PAWN_RIGHT[!side];  makePromotionMoves<depth, side, wKMoved, bKMoved, true>(nodes, from, to, board, kES); }
                            Bitloop(promosRight) { to = SquareOf(promosRight); from = to + PAWN_LEFT[!side];   makePromotionMoves<depth, side, wKMoved, bKMoved, true>(nodes, from, to, board, kES); }
                            Bitloop(promosFwd) { to = SquareOf(promosFwd);   from = to + PAWN_PUSH[!side];   makePromotionMoves<depth, side, wKMoved, bKMoved, false>(nodes, from, to, board, kES); }
                        }

                        Bitloop(pawnsLeft) {
                            to = SquareOf(pawnsLeft);
                            from = to + PAWN_RIGHT[!side];

                            BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                            nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsRight) {
                            to = SquareOf(pawnsRight);
                            from = to + PAWN_LEFT[!side];

                            BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                            nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsFwd) {
                            to = SquareOf(pawnsFwd);
                            from = to + PAWN_PUSH[!side];

                            BoardState newBoard = board.make<Piece::Pawn, side, false>(from, to, board, kES);
                            nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsDouble) {
                            to = SquareOf(pawnsDouble);
                            from = to + PAWN_DOUBLE_PUSH[!side];

                            BoardState newBoard = board.makeDoublePush<side>(from, to, board, kES);
                            nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                        }

                        /*

                           KNIGHT MOVES

                        */
                        bitboard = board.nM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getKnightAttacks(from) & validSquares;
                            makeMoves<depth, side, wKMoved, bKMoved, Piece::Knight>(nodes, attacks, from, board, kES);
                        }

                        /*

                           BISHOP MOVES

                        */
                        bitboard = board.bM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getBishopAttacks(from, board.occB) & validSquares;
                            makeMoves<depth, side, wKMoved, bKMoved, Piece::Bishop>(nodes, attacks, from, board, kES);
                        }

                        /*

                           ROOK MOVES

                        */
                        bitboard = board.rM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getRookAttacks(from, board.occB) & validSquares;
                            makeMoves<depth, side, wKMoved, bKMoved, Piece::Rook>(nodes, attacks, from, board, kES);
                        }

                        /*

                           QUEEN MOVES

                        */
                        bitboard = board.qM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getQueenAttacks(from, board.occB) & validSquares;
                            makeMoves<depth, side, wKMoved, bKMoved, Piece::Queen>(nodes, attacks, from, board, kES);
                        }
                    }
                }

                return nodes;
            }

            U64 bPins = findBishopPins<depth>(board.occB, board.bE, board.qE, kMS);
            U64 rPins = findRookPins<depth>(board.occB, board.rE, board.qE, kMS);
            U64 allPins = bPins | rPins;

            /*

               PAWN MOVES

            */
            //en passant
            U64 enPassant = PASSANT_CAPTURES[board.enPassant] & board.pM;
            bitboard = enPassant & ~allPins;
            Bitloop(bitboard) {
                from = SquareOf(bitboard);

                if ((1ULL << board.enPassant) & passantPinMask<side>(board.enPassant, from, board.occB, board.kM, board.rE, board.qE, kMS)) {
                    BoardState newBoard = board.makeEnPassant<side>(from, board.enPassant, board, kES);
                    nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                }
            }

            bitboard = enPassant & bPins;
            Bitloop(bitboard) {
                from = SquareOf(bitboard);

                if ((1ULL << board.enPassant) & bPins & passantPinMask<side>(board.enPassant, from, board.occB, board.kM, board.rE, board.qE, kMS)) {
                    BoardState newBoard = board.makeEnPassant<side>(from, board.enPassant, board, kES);
                    nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                }
            }

            U64 pawnsAtk = board.pM & ~rPins;
            U64 pawnsPush = board.pM & ~bPins;

            U64 pawnsLeft = (pawnsAtkLeft<side>(pawnsAtk & ~bPins & ~FIRST_COL) & board.occE) | (pawnsAtkLeft<side>(pawnsAtk & bPins & ~FIRST_COL) & board.occE & bPins);
            U64 pawnsRight = (pawnsAtkRight<side>(pawnsAtk & ~bPins & ~LAST_COL) & board.occE) | (pawnsAtkRight<side>(pawnsAtk & bPins & ~LAST_COL) & board.occE & bPins);
            U64 pawnsFwd = (pawnsAtkForward<side>(pawnsPush & ~rPins) & ~board.occB) | (pawnsAtkForward<side>(pawnsPush & rPins) & ~board.occB & rPins);
            U64 pawnsDouble = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~board.occB;

            if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANKS[side]) {
                U64 promosLeft = pawnsLeft & LAST_RANKS[side];
                U64 promosRight = pawnsRight & LAST_RANKS[side];
                U64 promosFwd = pawnsFwd & LAST_RANKS[side];

                pawnsLeft ^= promosLeft;
                pawnsRight ^= promosRight;
                pawnsFwd ^= promosFwd;

                Bitloop(promosLeft) {   to = SquareOf(promosLeft);  from = to + PAWN_RIGHT[!side];  makePromotionMoves<depth, side, wKMoved, bKMoved, true>(nodes, from, to, board, kES); }
                Bitloop(promosRight) {  to = SquareOf(promosRight); from = to + PAWN_LEFT[!side];   makePromotionMoves<depth, side, wKMoved, bKMoved, true>(nodes, from, to, board, kES); }
                Bitloop(promosFwd) {    to = SquareOf(promosFwd);   from = to + PAWN_PUSH[!side];   makePromotionMoves<depth, side, wKMoved, bKMoved, false>(nodes, from, to, board, kES); } 
            }

            Bitloop(pawnsLeft) {
                to = SquareOf(pawnsLeft);
                from = to + PAWN_RIGHT[!side];

                BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsRight) {
                to = SquareOf(pawnsRight);
                from = to + PAWN_LEFT[!side];

                BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsFwd) {
                to = SquareOf(pawnsFwd);
                from = to + PAWN_PUSH[!side];

                BoardState newBoard = board.make<Piece::Pawn, side, false>(from, to, board, kES);
                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsDouble) {
                to = SquareOf(pawnsDouble);
                from = to + PAWN_DOUBLE_PUSH[!side];

                BoardState newBoard = board.makeDoublePush<side>(from, to, board, kES);
                nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
            }

            /*

               KNIGHT MOVES

            */
            bitboard = board.nM & ~allPins;
            Bitloop(bitboard)
            {
                from = SquareOf(bitboard);

                attacks = getKnightAttacks(from) & ~board.occM;
                makeMoves<depth, side, wKMoved, bKMoved, Piece::Knight>(nodes, attacks, from, board, kES);
            }

            /*

               BISHOP MOVES

            */
            bitboard = board.bM & ~allPins;
            Bitloop(bitboard)
            {
                from = SquareOf(bitboard);

                attacks = getBishopAttacks(from, board.occB) & ~board.occM;
                makeMoves<depth, side, wKMoved, bKMoved, Piece::Bishop>(nodes, attacks, from, board, kES);
            }

            bitboard = (board.bM | board.qM) & bPins;
            Bitloop(bitboard)
            {
                from = SquareOf(bitboard);
    
                attacks = validAttacksMasks[depth][from];
                if ((1ULL << from) & board.qM)  makeMoves<depth, side, wKMoved, bKMoved, Piece::Queen>(nodes, attacks, from, board, kES);
                else                            makeMoves<depth, side, wKMoved, bKMoved, Piece::Bishop>(nodes, attacks, from, board, kES);
            }

            /*

               ROOK MOVES

            */
            bitboard = board.rM & ~allPins;
            Bitloop(bitboard)
            {
                from = SquareOf(bitboard);

                attacks = getRookAttacks(from, board.occB) & ~board.occM;
                makeMoves<depth, side, wKMoved, bKMoved, Piece::Rook>(nodes, attacks, from, board, kES);
            }

            bitboard = (board.rM | board.qM) & rPins;
            Bitloop(bitboard)
            {
                from = SquareOf(bitboard);
  
                attacks = validAttacksMasks[depth][from];
                if ((1ULL << from) & board.qM)  makeMoves<depth, side, wKMoved, bKMoved, Piece::Queen>(nodes, attacks, from, board, kES);
                else                            makeMoves<depth, side, wKMoved, bKMoved, Piece::Rook>(nodes, attacks, from, board, kES);
            }

            /*

               QUEEN MOVES

            */
            bitboard = board.qM & ~allPins;
            Bitloop(bitboard)
            {
                from = SquareOf(bitboard);

                attacks = getQueenAttacks(from, board.occB) & ~board.occM;
                makeMoves<depth, side, wKMoved, bKMoved, Piece::Queen>(nodes, attacks, from, board, kES);
            }

            /*

               KING MOVES

            */
            U64 mask = board.kMA;
            attacks = mask & ~board.occM;
            filterKingAttacks<side, wKMoved, bKMoved>(board.occM, board.occB, kMS, attacks, castleAttacks, board.pE, board.nE, board.bE, board.rE, board.qE, board.kEA, mask);
            makeMoves<depth, side, wKMoved, bKMoved, Piece::King>(nodes, attacks, kMS, board, kES);

            if constexpr ((side == white && !wKMoved)) {
                if (castle<CASTLING_SIDE_K[white]>(board.casPerms, board.occE, board.occB, board.nE, board.bE, board.rE, board.qE, castleAttacks)) {
                    BoardState newBoard = board.makeCastling<CASTLING_SIDE_K[white]>(board, kES);
                    nodes += PerftGenerator<depth - 1, black, true, bKMoved>::generateMoves(newBoard);
                }

                if (castle<CASTLING_SIDE_Q[white]>(board.casPerms, board.occE, board.occB, board.nE, board.bE, board.rE, board.qE, castleAttacks)) {
                    BoardState newBoard = board.makeCastling<CASTLING_SIDE_Q[white]>(board, kES);
                    nodes += PerftGenerator<depth - 1, black, true, bKMoved>::generateMoves(newBoard);
                }
            }
            else if constexpr ((side == black && !bKMoved)) {
                if (castle<CASTLING_SIDE_K[black]>(board.casPerms, board.occE, board.occB, board.nE, board.bE, board.rE, board.qE, castleAttacks)) {
                    BoardState newBoard = board.makeCastling<CASTLING_SIDE_K[black]>(board, kES);
                    nodes += PerftGenerator<depth - 1, white, wKMoved, true>::generateMoves(newBoard);
                }

                if (castle<CASTLING_SIDE_Q[black]>(board.casPerms, board.occE, board.occB, board.nE, board.bE, board.rE, board.qE, castleAttacks)) {
                    BoardState newBoard = board.makeCastling<CASTLING_SIDE_Q[black]>(board, kES);
                    nodes += PerftGenerator<depth - 1, white, wKMoved, true>::generateMoves(newBoard);
                }
            }

            return nodes;
        }
    };

    template <bool side, bool wKMoved, bool bKMoved>
    struct PerftGenerator<1, side, wKMoved, bKMoved> {
        ForceInline U64 generateMoves(const BoardState& board) {
            if (!board.occB) return 0;
            int from;
            U64 bitboard, attacks;

            U64 nodes = 0ULL;
            U64 castleAttacks = 0ULL;

            int kMS = SquareOf(board.kM);
   
            if (board.checks) {
                /*

                    KING MOVES

                */

                int checkSquare = SquareOf(board.checks);
                U64 mask = board.kMA;
                //inverted PIN_MASKS, because the king cannot move in the attack ray of the check pieces
                attacks = mask & ~board.occM & ~PIN_MASKS[checkSquare][kMS] & ~PIN_MASKS[Ms1b(board.checks)][kMS];
                filterKingAttacks<side, true, true>(board.occM, board.occB, kMS, attacks, castleAttacks, board.pE, board.nE, board.bE, board.rE, board.qE, board.kEA, mask);
                nodes += Bitcount(attacks);

                //if there is no second check, we need to check for other pieces
                if (Bitcount(board.checks) == 1) {
                    U64 bPins = findBishopPins<1>(board.occB, board.bE, board.qE, kMS);
                    U64 rPins = findRookPins<1>(board.occB, board.rE, board.qE, kMS);
                    U64 allPins = bPins | rPins;

                    //if check piece is pawn or knight, only possible moves are capture of the check piece (+ king moves)
                    if (board.checks & (board.pE | board.nE)) {
                        /*

                           PAWN MOVES

                        */
                        bitboard = board.pM & PASSANT_CAPTURES[board.enPassant] & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            nodes += Bitcount(board.checks & (1ULL << (board.enPassant + PAWN_PUSH[!side])));
                        }

                        bitboard = board.pM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            /*attacks = PAWN_CAPTURES[side][from] & board.checks;
                            nodes += Bitcount(attacks) * RANK_MULTIPLIER[side][from];*/
                            //nodes += getPawnAttacksCountCheckLeaper<side>(from, board.checks);
                            nodes += PAWN_ATTACK_COUNT_CHECK[SquareOf(PAWN_CAPTURES[side][from] & board.checks)];
                        }

                        /*

                           KNIGHT MOVES

                        */
                        bitboard = board.nM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getKnightAttacks(from) & board.checks;
                            nodes += Bitcount(attacks);
                        }

                        /*

                           BISHOP MOVES

                        */
                        bitboard = board.bM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getBishopAttacks(from, board.occB) & board.checks;
                            nodes += Bitcount(attacks);
                        }

                        /*

                           ROOK MOVES

                        */
                        bitboard = board.rM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getRookAttacks(from, board.occB) & board.checks;
                            nodes += Bitcount(attacks);
                        }

                        /*

                           QUEEN MOVES

                        */
                        bitboard = board.qM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getQueenAttacks(from, board.occB) & board.checks;
                            nodes += Bitcount(attacks);
                        }
                    }
                    else {
                        //squares between the check piece (included) and the king
                        //those are the only squares that a piece can go to to block the check
                        U64 validSquares = (board.checks | PIN_MASKS[checkSquare][kMS]);

                        /*

                           PAWN MOVES

                        */
                        U64 pawns = board.pM & ~allPins;

                        U64 pawnsLeft = pawnsAtkLeft<side>(pawns & ~FIRST_COL) & board.occE & validSquares;
                        U64 pawnsRight = pawnsAtkRight<side>(pawns & ~LAST_COL) & board.occE & validSquares;
                        U64 pawnsFwd = pawnsAtkForward<side>(pawns) & ~board.occB;
                        U64 pawnsDouble = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~board.occB & validSquares;
                        //mask after double push, to not ignore possible moves
                        pawnsFwd &= validSquares;

                        if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANKS[side]) {
                            U64 promosLeft = pawnsLeft & LAST_RANKS[side];
                            U64 promosRight = pawnsRight & LAST_RANKS[side];
                            U64 promosFwd = pawnsFwd & LAST_RANKS[side];

                            nodes += (Bitcount(promosLeft) + Bitcount(promosRight) + Bitcount(promosFwd)) * 4;

                            nodes += Bitcount(pawnsLeft ^ promosLeft);
                            nodes += Bitcount(pawnsRight ^ promosRight);
                            nodes += Bitcount((pawnsFwd ^ promosFwd) | pawnsDouble);
                        }
                        else {
                            nodes += Bitcount(pawnsLeft);
                            nodes += Bitcount(pawnsRight);
                            nodes += Bitcount(pawnsFwd | pawnsDouble);
                        }

                        /*

                           KNIGHT MOVES

                        */
                        bitboard = board.nM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getKnightAttacks(from) & validSquares;
                            nodes += Bitcount(attacks);
                        }

                        /*

                           BISHOP MOVES

                        */
                        bitboard = board.bM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getBishopAttacks(from, board.occB) & validSquares;
                            nodes += Bitcount(attacks);
                        }

                        /*

                           ROOK MOVES

                        */
                        bitboard = board.rM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getRookAttacks(from, board.occB) & validSquares;
                            nodes += Bitcount(attacks);
                        }

                        /*

                           QUEEN MOVES

                        */
                        bitboard = board.qM & ~allPins;
                        Bitloop(bitboard)
                        {
                            from = SquareOf(bitboard);

                            attacks = getQueenAttacks(from, board.occB) & validSquares;
                            nodes += Bitcount(attacks);
                        }
                    }
                }

                return nodes;
            }

            U64 bPins = findBishopPins<1>(board.occB, board.bE, board.qE, kMS);
            U64 rPins = findRookPins<1>(board.occB, board.rE, board.qE, kMS);
            U64 allPins = bPins | rPins;

            /*

               PAWN MOVES

            */
            //en passant
            U64 passant = PASSANT_CAPTURES[board.enPassant] & board.pM;
            bitboard = passant & ~allPins;
            Bitloop(bitboard) {
                from = SquareOf(bitboard);

                nodes += Bitcount((1ULL << board.enPassant) & passantPinMask<side>(board.enPassant, from, board.occB, board.kM, board.rE, board.qE, kMS));
            }

            bitboard = passant & bPins;
            Bitloop(bitboard) {
                from = SquareOf(bitboard);
            
                nodes += Bitcount((1ULL << board.enPassant) & bPins & passantPinMask<side>(board.enPassant, from, board.occB, board.kM, board.rE, board.qE, kMS));
            }

            U64 pawnsAtk = board.pM & ~rPins;
            U64 pawnsPush = board.pM & ~bPins;

            U64 pawnsLeft = (pawnsAtkLeft<side>(pawnsAtk & ~bPins & ~FIRST_COL) & board.occE) | (pawnsAtkLeft<side>(pawnsAtk & bPins & ~FIRST_COL) & board.occE & bPins);
            U64 pawnsRight = (pawnsAtkRight<side>(pawnsAtk & ~bPins & ~LAST_COL) & board.occE) | (pawnsAtkRight<side>(pawnsAtk & bPins & ~LAST_COL) & board.occE & bPins);
            U64 pawnsFwd = (pawnsAtkForward<side>(pawnsPush & ~rPins) & ~board.occB) | (pawnsAtkForward<side>(pawnsPush & rPins) & ~board.occB & rPins);
            U64 pawnsDouble = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~board.occB;

            if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANKS[side]) {
                U64 promosLeft = pawnsLeft & LAST_RANKS[side];
                U64 promosRight = pawnsRight & LAST_RANKS[side];
                U64 promosFwd = pawnsFwd & LAST_RANKS[side];

                nodes += (Bitcount(promosLeft) + Bitcount(promosRight) + Bitcount(promosFwd)) * 4;
                
                nodes += Bitcount(pawnsLeft ^ promosLeft);
                nodes += Bitcount(pawnsRight ^ promosRight);
                nodes += Bitcount((pawnsFwd ^ promosFwd) | pawnsDouble);
            }
            else {
                nodes += Bitcount(pawnsLeft);
                nodes += Bitcount(pawnsRight);
                nodes += Bitcount(pawnsFwd | pawnsDouble);
            }

            /*

               KNIGHT MOVES

            */
            bitboard = board.nM & ~allPins;
            Bitloop(bitboard)
            {
                from = SquareOf(bitboard);

                attacks = getKnightAttacks(from) & ~board.occM;
                nodes += Bitcount(attacks);
            }

            /*

               BISHOP MOVES

            */
            bitboard = board.bM & ~allPins;
            Bitloop(bitboard)
            {
                from = SquareOf(bitboard);

                attacks = getBishopAttacks(from, board.occB) & ~board.occM;
                nodes += Bitcount(attacks);
            }

            bitboard = (board.bM | board.qM) & bPins;
            Bitloop(bitboard)
            {
                from = SquareOf(bitboard);

                attacks = validAttacksMasks[1][from];
                nodes += Bitcount(attacks);
            }

            /*

               ROOK MOVES

            */
            bitboard = board.rM & ~allPins;
            Bitloop(bitboard)
            {
                from = SquareOf(bitboard);

                attacks = getRookAttacks(from, board.occB) & ~board.occM;
                nodes += Bitcount(attacks);
            }

            bitboard = (board.rM | board.qM) & rPins;
            Bitloop(bitboard)
            {
                from = SquareOf(bitboard);
 
                attacks = validAttacksMasks[1][from];
                nodes += Bitcount(attacks);
            }

            /*

               QUEEN MOVES

            */
            bitboard = board.qM & ~allPins;
            Bitloop(bitboard)
            {
                from = SquareOf(bitboard);

                attacks = getQueenAttacks(from, board.occB) & ~board.occM;
                nodes += Bitcount(attacks);
            }

            /*

               KING MOVES

            */
            U64 mask = board.kMA;
            attacks = mask & ~board.occM;
            filterKingAttacks<side, wKMoved, bKMoved>(board.occM, board.occB, kMS, attacks, castleAttacks, board.pE, board.nE, board.bE, board.rE, board.qE, board.kEA, mask);
            nodes += Bitcount(attacks);

            if constexpr ((side == white && !wKMoved) || (side == black && !bKMoved)) {
                if (castle<CASTLING_SIDE_K[side]>(board.casPerms, board.occE, board.occB, board.nE, board.bE, board.rE, board.qE, castleAttacks)) {
                    nodes++;
                }

                if (castle<CASTLING_SIDE_Q[side]>(board.casPerms, board.occE, board.occB, board.nE, board.bE, board.rE, board.qE, castleAttacks)) {
                    nodes++;
                }
            }

            return nodes;
        }
    };

}

#endif