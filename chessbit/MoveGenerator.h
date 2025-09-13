#ifndef MOVEGENERATOR_H
#define MOVEGENERATOR_H

#include "Definitions.h"
#include "MoveArray.h"
#include <vector>

using namespace defs;
using namespace movarray;
using namespace moveinfo;

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

    enum class Piece {
        Pawn, Knight, Bishop, Rook, Queen, King
    };

    struct BoardState {
        U64 pM;
        U64 nM;
        U64 bM;
        U64 rM;
        U64 qM;
        U64 kM;

        U64 pE;
        U64 nE;
        U64 bE;
        U64 rE;
        U64 qE;
        U64 kE;

        U64 kMA;
        U64 kEA;

        U64 occM;
        U64 occE;
        U64 occB;

        U64 checks;
        int casPerms;
        int enPassant;

        bool s;

        constexpr BoardState(
            U64 pM, U64 nM, U64 bM, U64 rM, U64 qM, U64 kM,
            U64 pE, U64 nE, U64 bE, U64 rE, U64 qE, U64 kE,
            U64 kMA, U64 kEA,
            U64 occM, U64 occE, U64 occB,
            U64 checks, int casPerms, int enPassant, bool side) :
            pM(pM), nM(nM), bM(bM), rM(rM), qM(qM), kM(kM),
            pE(pE), nE(nE), bE(bE), rE(rE), qE(qE), kE(kE),
            kMA(kMA), kEA(kEA),
            occM(occM), occE(occE), occB(occB),
            checks(checks), casPerms(casPerms), enPassant(enPassant), s(side)
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
                if constexpr (Piece::King == piece)         return BoardState(board.pE & occE, board.nE & occE, board.bE & occE, board.rE & occE, board.qE & occE, board.kE, pM, nM, bM, rM, qM, kM, board.kEA, getKingAttacks(to), occE, occM, occB, checks, casPerms, noSquare, !side);
                else                                        return BoardState(board.pE & occE, board.nE & occE, board.bE & occE, board.rE & occE, board.qE & occE, board.kE, pM, nM, bM, rM, qM, kM, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare, !side);
            }
            else {
                const U64 occB = occM | occE;
                checks |= sliderChecks(bM, rM, qM, occB, kES);

                if constexpr (Piece::King == piece)         return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, nM, bM, rM, qM, kM, board.kEA, getKingAttacks(to), occE, occM, occB, checks, casPerms, noSquare, !side);
                else                                        return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, nM, bM, rM, qM, kM, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare, !side);
            }
        }

        template <Piece piece, bool side, bool capture>
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
                return BoardState(board.pE & occE, board.nE & occE, board.bE & occE, board.rE & occE, board.qE & occE, board.kE, pM, nM, bM, rM, qM, board.kM, board.kEA, board.kMA, occE, occM, occB, checks, casPerms, noSquare, !side);
            }
            else {
                const U64 occB = occM | occE;
                checks |= sliderChecks(bM, rM, qM, occB, kES);
                return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, nM, bM, rM, qM, board.kM, board.kEA, board.kMA, occE, occM, occB, checks, board.casPerms, noSquare, !side);
            }
        }

        template <bool side>
        ForceInline BoardState makeDoublePush(int from, int to, const BoardState& board, int kES) {
            const U64 move = (1ULL << from) | (1ULL << to);

            const U64 pM = board.pM ^ move;
            const U64 occM = board.occM ^ move;

            const U64 occB = occM | board.occE;

            const U64 checks = (PAWN_CAPTURES[!side][kES] & pM) | sliderChecks(board.bM, board.rM, board.qM, occB, kES);

            return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, board.nM, board.bM, board.rM, board.qM, board.kM, board.kEA, board.kMA, board.occE, occM, occB, checks, board.casPerms, from + PAWN_PUSH[side], !side);
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

            return BoardState(pE, board.nE, board.bE, board.rE, board.qE, board.kE, pM, board.nM, board.bM, board.rM, board.qM, board.kM, board.kEA, board.kMA, occE, occM, occB, checks, board.casPerms, noSquare, !side);
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
        ForceInline U64 bothSwitch() {
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

            return BoardState(board.pE, board.nE, board.bE, board.rE, board.qE, board.kE, board.pM, board.nM, board.bM, rM, board.qM, kM, board.kEA, getKingAttacks(to), board.occE, occM, occB, checks, board.casPerms, noSquare, CASTLING_SIDE_OPPOSITE[castlingSide]);
        }
    };

    extern BoardState dummy;

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
    ForceInline U64 pawnsAtkLeft(U64 pM) {
        if constexpr (side == white) return pM >> 9;
        return pM << 7;
    }

    template <bool side>
    ForceInline U64 pawnsAtkRight(U64 pM) {
        if constexpr (side == white) return pM >> 7;
        return pM << 9;
    }

    template <bool side>
    ForceInline U64 pawnsAtkForward(U64 pM) {
        if constexpr (side == white) return pM >> 8;
        return pM << 8;
    }

    template <int depth, bool side, bool wKMoved, bool bKMoved>
    struct PerftGenerator;

    template <int depth, bool side, bool wKMoved, bool bKMoved, Piece piece, bool capture>
    ForceInline void enumMoves(U64& nodes, U64 moves, int from, const BoardState& board, int kES) {
        if (moves) {
            int to = SquareOf(moves);

            BoardState newBoard = board.make<piece, side, capture>(from, to, board, kES);
            if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, capture, newBoard); movesArray.add(mov); }
            else {
                if constexpr (piece == Piece::King) {
                    if constexpr (side == white)    nodes += PerftGenerator<depth - 1, !side, true, bKMoved>::generateMoves(newBoard);
                    else                            nodes += PerftGenerator<depth - 1, !side, wKMoved, true>::generateMoves(newBoard);
                }
                else {
                    nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                }
            }

            enumMoves<depth, side, wKMoved, bKMoved, piece, capture>(nodes, _blsr_u64(moves), from, board, kES);
        }
    }

    template <int depth, bool side, bool wKMoved, bool bKMoved, Piece piece>
    ForceInline void makeMoves(U64& nodes, U64 attacks, int from, const BoardState& board, int kES) {
        U64 moves = attacks & ~board.occE;
        enumMoves<depth, side, wKMoved, bKMoved, piece, false>(nodes, moves, from, board, kES);

        moves = attacks & board.occE;
        enumMoves<depth, side, wKMoved, bKMoved, piece, true>(nodes, moves, from, board, kES);
    }

    template <int depth, bool side, bool wKMoved, bool bKMoved, bool capture>
    ForceInline void makePromotionMoves(U64& nodes, int from, int to, const BoardState& board, int kES) {
        BoardState newBoardN = board.makePromotion<Piece::Knight, side, capture>(from, to, board, kES);
        if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, n, capture, newBoardN); movesArray.add(mov); }
        else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoardN);

        BoardState newBoardB = board.makePromotion<Piece::Bishop, side, capture>(from, to, board, kES);
        if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, b, capture, newBoardB); movesArray.add(mov); }
        else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoardB);

        BoardState newBoardR = board.makePromotion<Piece::Rook, side, capture>(from, to, board, kES);
        if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, r, capture, newBoardR); movesArray.add(mov); }
        else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoardR);

        BoardState newBoardQ = board.makePromotion<Piece::Queen, side, capture>(from, to, board, kES);
        if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, q, capture, newBoardQ); movesArray.add(mov); }
        else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoardQ);
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
    ForceInline U64 allMoves(const BoardState& board) {
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
            attacks = mask & ~board.occM & ~PIN_MASKS[checkSquare][kMS] & ~PIN_MASKS[Ms1b(board.checks)][kMS];
            filterKingAttacks<side, true, true>(board.occM, board.occB, kMS, attacks, castleAttacks, board.pE, board.nE, board.bE, board.rE, board.qE, board.kEA, mask);
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else makeMoves<depth, side, wKMoved, bKMoved, Piece::King>(nodes, attacks, kMS, board, kES);

            if (Bitcount(board.checks) == 1) {
                U64 bPins = findBishopPins<depth>(board.occB, board.bE, board.qE, kMS);
                U64 rPins = findRookPins<depth>(board.occB, board.rE, board.qE, kMS);
                U64 allPins = bPins | rPins;
                if (board.checks & (board.pE | board.nE)) {
                    /*

                       PAWN MOVES

                    */
                    to = checkSquare;

                    U64 pawns = board.pM & ~allPins;
                    U64 enPassant = pawns & PASSANT_CAPTURES[board.enPassant];
                    U64 caps = pawns & PAWN_CAPTURES[!side][checkSquare];
                    U64 promos = caps & PROMO_RANKS[side];
                    caps ^= promos;

                    if constexpr (depth == 1) {
                        nodes += Bitcount(enPassant | (caps ^ promos));
                        nodes += Bitcount(promos) * 4;
                    }
                    else {
                        Bitloop(enPassant)
                        {
                            from = SquareOf(enPassant);

                            BoardState newBoard = board.makeEnPassant<side>(from, board.enPassant, board, kES);
                            if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, board.enPassant, true, newBoard); movesArray.add(mov); }
                            else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                        }

                        Bitloop(promos) { from = SquareOf(promos); makePromotionMoves<depth, side, wKMoved, bKMoved, true>(nodes, from, to, board, kES); }

                        Bitloop(caps) {
                            from = SquareOf(caps);

                            BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                            if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, true, newBoard); movesArray.add(mov); }
                            else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
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
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else {
                            if (attacks) {
                                BoardState newBoard = board.make<Piece::Knight, side, true>(from, to, board, kES);
                                if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, true, newBoard); movesArray.add(mov); }
                                else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                            }
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
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else {
                            if (attacks) {
                                BoardState newBoard = board.make<Piece::Bishop, side, true>(from, to, board, kES);
                                if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, true, newBoard); movesArray.add(mov); }
                                else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                            }
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
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else {
                            if (attacks) {
                                BoardState newBoard = board.make<Piece::Rook, side, true>(from, to, board, kES);
                                if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, true, newBoard); movesArray.add(mov); }
                                else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                            }
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
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else {
                            if (attacks) {
                                BoardState newBoard = board.make<Piece::Queen, side, true>(from, to, board, kES);
                                if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, true, newBoard); movesArray.add(mov); }
                                else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                            }
                        }
                    }

                }
                else {
                    U64 validSquares = (board.checks | PIN_MASKS[checkSquare][kMS]);

                    /*

                       PAWN MOVES

                    */
                    U64 pawns = board.pM & ~allPins;

                    U64 pawnsLeft = pawnsAtkLeft<side>(pawns & ~FIRST_COL) & board.occE & validSquares;
                    U64 pawnsRight = pawnsAtkRight<side>(pawns & ~LAST_COL) & board.occE & validSquares;
                    U64 pawnsFwd = pawnsAtkForward<side>(pawns) & ~board.occB;
                    U64 pawnsDouble = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~board.occB & validSquares;
                    pawnsFwd &= validSquares;

                    if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANKS[side]) {
                        U64 promosLeft = pawnsLeft & LAST_RANKS[side];
                        U64 promosRight = pawnsRight & LAST_RANKS[side];
                        U64 promosFwd = pawnsFwd & LAST_RANKS[side];

                        pawnsLeft ^= promosLeft;
                        pawnsRight ^= promosRight;
                        pawnsFwd ^= promosFwd;

                        if constexpr (depth == 1) nodes += (Bitcount(promosLeft) + Bitcount(promosRight) + Bitcount(promosFwd)) * 4;
                        else {
                            Bitloop(promosLeft) { to = SquareOf(promosLeft);  from = to + PAWN_RIGHT[!side];  makePromotionMoves<depth, side, wKMoved, bKMoved, true>(nodes, from, to, board, kES); }
                            Bitloop(promosRight) { to = SquareOf(promosRight); from = to + PAWN_LEFT[!side];   makePromotionMoves<depth, side, wKMoved, bKMoved, true>(nodes, from, to, board, kES); }
                            Bitloop(promosFwd) { to = SquareOf(promosFwd);   from = to + PAWN_PUSH[!side];   makePromotionMoves<depth, side, wKMoved, bKMoved, false>(nodes, from, to, board, kES); }
                        }
                    }

                    if constexpr (depth == 1) {
                        nodes += Bitcount(pawnsLeft);
                        nodes += Bitcount(pawnsRight);
                        nodes += Bitcount(pawnsFwd | pawnsDouble);
                    }
                    else {
                        Bitloop(pawnsLeft) {
                            to = SquareOf(pawnsLeft);
                            from = to + PAWN_RIGHT[!side];

                            BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                            if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, true, newBoard); movesArray.add(mov); }
                            else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsRight) {
                            to = SquareOf(pawnsRight);
                            from = to + PAWN_LEFT[!side];

                            BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                            if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, true, newBoard); movesArray.add(mov); }
                            else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsFwd) {
                            to = SquareOf(pawnsFwd);
                            from = to + PAWN_PUSH[!side];

                            BoardState newBoard = board.make<Piece::Pawn, side, false>(from, to, board, kES);
                            if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, false, newBoard); movesArray.add(mov); }
                            else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsDouble) {
                            to = SquareOf(pawnsDouble);
                            from = to + PAWN_DOUBLE_PUSH[!side];

                            BoardState newBoard = board.makeDoublePush<side>(from, to, board, kES);
                            if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, false, newBoard); movesArray.add(mov); }
                            else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                        }
                    }
                    /*

                       KNIGHT MOVES

                    */
                    bitboard = board.nM & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getKnightAttacks(from) & validSquares;
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else makeMoves<depth, side, wKMoved, bKMoved, Piece::Knight>(nodes, attacks, from, board, kES);
                    }

                    /*

                       BISHOP MOVES

                    */
                    bitboard = board.bM & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getBishopAttacks(from, board.occB) & validSquares;
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else makeMoves<depth, side, wKMoved, bKMoved, Piece::Bishop>(nodes, attacks, from, board, kES);
                    }

                    /*

                       ROOK MOVES

                    */
                    bitboard = board.rM & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getRookAttacks(from, board.occB) & validSquares;
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else makeMoves<depth, side, wKMoved, bKMoved, Piece::Rook>(nodes, attacks, from, board, kES);
                    }

                    /*

                       QUEEN MOVES

                    */
                    bitboard = board.qM & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getQueenAttacks(from, board.occB) & validSquares;
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else makeMoves<depth, side, wKMoved, bKMoved, Piece::Queen>(nodes, attacks, from, board, kES);
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
        U64 enPassant = PASSANT_CAPTURES[board.enPassant] & board.pM;
        bitboard = enPassant & ~allPins;
        Bitloop(bitboard) {
            from = SquareOf(bitboard);

            if constexpr (depth == 1) nodes += Bitcount((1ULL << board.enPassant) & passantPinMask<side>(board.enPassant, from, board.occB, board.kM, board.rE, board.qE, kMS));
            else {
                if ((1ULL << board.enPassant) & passantPinMask<side>(board.enPassant, from, board.occB, board.kM, board.rE, board.qE, kMS)) {
                    BoardState newBoard = board.makeEnPassant<side>(from, board.enPassant, board, kES);
                    if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, board.enPassant, true, newBoard); movesArray.add(mov); }
                    else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                }
            }
        }

        bitboard = enPassant & bPins;
        Bitloop(bitboard) {
            from = SquareOf(bitboard);

            if constexpr (depth == 1) nodes += Bitcount((1ULL << board.enPassant) & bPins & passantPinMask<side>(board.enPassant, from, board.occB, board.kM, board.rE, board.qE, kMS));
            else {
                if ((1ULL << board.enPassant) & bPins & passantPinMask<side>(board.enPassant, from, board.occB, board.kM, board.rE, board.qE, kMS)) {
                    BoardState newBoard = board.makeEnPassant<side>(from, board.enPassant, board, kES);
                    if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, board.enPassant, true, newBoard); movesArray.add(mov); }
                    else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
                }
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

            if constexpr (depth == 1) nodes += (Bitcount(promosLeft) + Bitcount(promosRight) + Bitcount(promosFwd)) * 4;
            else {
                Bitloop(promosLeft) { to = SquareOf(promosLeft);  from = to + PAWN_RIGHT[!side];  makePromotionMoves<depth, side, wKMoved, bKMoved, true>(nodes, from, to, board, kES); }
                Bitloop(promosRight) { to = SquareOf(promosRight); from = to + PAWN_LEFT[!side];   makePromotionMoves<depth, side, wKMoved, bKMoved, true>(nodes, from, to, board, kES); }
                Bitloop(promosFwd) { to = SquareOf(promosFwd);   from = to + PAWN_PUSH[!side];   makePromotionMoves<depth, side, wKMoved, bKMoved, false>(nodes, from, to, board, kES); }
            }
        }

        if constexpr (depth == 1) {
            nodes += Bitcount(pawnsLeft);
            nodes += Bitcount(pawnsRight);
            nodes += Bitcount(pawnsFwd | pawnsDouble);
        }
        else {
            Bitloop(pawnsLeft) {
                to = SquareOf(pawnsLeft);
                from = to + PAWN_RIGHT[!side];

                BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, true, newBoard); movesArray.add(mov); }
                else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsRight) {
                to = SquareOf(pawnsRight);
                from = to + PAWN_LEFT[!side];

                BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, true, newBoard); movesArray.add(mov); }
                else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsFwd) {
                to = SquareOf(pawnsFwd);
                from = to + PAWN_PUSH[!side];

                BoardState newBoard = board.make<Piece::Pawn, side, false>(from, to, board, kES);
                if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, false, newBoard); movesArray.add(mov); }
                else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsDouble) {
                to = SquareOf(pawnsDouble);
                from = to + PAWN_DOUBLE_PUSH[!side];

                BoardState newBoard = board.makeDoublePush<side>(from, to, board, kES);
                if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, to, false, newBoard); movesArray.add(mov); }
                else nodes += PerftGenerator<depth - 1, !side, wKMoved, bKMoved>::generateMoves(newBoard);
            }
        }

        /*

           KNIGHT MOVES

        */
        bitboard = board.nM & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getKnightAttacks(from) & ~board.occM;
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else makeMoves<depth, side, wKMoved, bKMoved, Piece::Knight>(nodes, attacks, from, board, kES);
        }

        /*

           BISHOP MOVES

        */
        bitboard = board.bM & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getBishopAttacks(from, board.occB) & ~board.occM;
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else makeMoves<depth, side, wKMoved, bKMoved, Piece::Bishop>(nodes, attacks, from, board, kES);
        }

        bitboard = (board.bM | board.qM) & bPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = validAttacksMasks[depth][from];
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else {
                if ((1ULL << from) & board.qM)  makeMoves<depth, side, wKMoved, bKMoved, Piece::Queen>(nodes, attacks, from, board, kES);
                else                            makeMoves<depth, side, wKMoved, bKMoved, Piece::Bishop>(nodes, attacks, from, board, kES);
            }
        }

        /*

           ROOK MOVES

        */
        bitboard = board.rM & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getRookAttacks(from, board.occB) & ~board.occM;
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else makeMoves<depth, side, wKMoved, bKMoved, Piece::Rook>(nodes, attacks, from, board, kES);
        }

        bitboard = (board.rM | board.qM) & rPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = validAttacksMasks[depth][from];
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else {
                if ((1ULL << from) & board.qM)  makeMoves<depth, side, wKMoved, bKMoved, Piece::Queen>(nodes, attacks, from, board, kES);
                else                            makeMoves<depth, side, wKMoved, bKMoved, Piece::Rook>(nodes, attacks, from, board, kES);
            }
        }

        /*

           QUEEN MOVES

        */
        bitboard = board.qM & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getQueenAttacks(from, board.occB) & ~board.occM;
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else makeMoves<depth, side, wKMoved, bKMoved, Piece::Queen>(nodes, attacks, from, board, kES);
        }

        /*

           KING MOVES

        */
        U64 mask = board.kMA;
        attacks = mask & ~board.occM;
        filterKingAttacks<side, wKMoved, bKMoved>(board.occM, board.occB, kMS, attacks, castleAttacks, board.pE, board.nE, board.bE, board.rE, board.qE, board.kEA, mask);
        if constexpr (depth == 1) nodes += Bitcount(attacks);
        else makeMoves<depth, side, wKMoved, bKMoved, Piece::King>(nodes, attacks, kMS, board, kES);

        if constexpr ((side == white && !wKMoved)) {
            if (castle<CASTLING_SIDE_K[white]>(board.casPerms, board.occE, board.occB, board.nE, board.bE, board.rE, board.qE, castleAttacks)) {
                if constexpr (depth == 1) nodes++;
                else {
                    BoardState newBoard = board.makeCastling<CASTLING_SIDE_K[white]>(board, kES);
                    if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_K[white]], true, newBoard); movesArray.add(mov); }
                    else nodes += PerftGenerator<depth - 1, black, true, bKMoved>::generateMoves(newBoard);
                }
            }

            if (castle<CASTLING_SIDE_Q[white]>(board.casPerms, board.occE, board.occB, board.nE, board.bE, board.rE, board.qE, castleAttacks)) {
                if constexpr (depth == 1) nodes++;
                else {
                    BoardState newBoard = board.makeCastling<CASTLING_SIDE_Q[white]>(board, kES);
                    if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_Q[white]], true, newBoard); movesArray.add(mov); }
                    else nodes += PerftGenerator<depth - 1, black, true, bKMoved>::generateMoves(newBoard);
                }
            }
        }
        else if constexpr ((side == black && !bKMoved)) {
            if (castle<CASTLING_SIDE_K[black]>(board.casPerms, board.occE, board.occB, board.nE, board.bE, board.rE, board.qE, castleAttacks)) {
                if constexpr (depth == 1) nodes++;
                else {
                    BoardState newBoard = board.makeCastling<CASTLING_SIDE_K[black]>(board, kES);
                    if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_K[black]], true, newBoard); movesArray.add(mov); }
                    else nodes += PerftGenerator<depth - 1, white, wKMoved, true>::generateMoves(newBoard);
                }
            }

            if (castle<CASTLING_SIDE_Q[black]>(board.casPerms, board.occE, board.occB, board.nE, board.bE, board.rE, board.qE, castleAttacks)) {
                if constexpr (depth == 1) nodes++;
                else {
                    BoardState newBoard = board.makeCastling<CASTLING_SIDE_Q[black]>(board, kES);
                    if constexpr (depth == 0) { MoveInfo mov = MoveInfo(from, CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_Q[black]], true, newBoard); movesArray.add(mov); }
                    else nodes += PerftGenerator<depth - 1, white, wKMoved, true>::generateMoves(newBoard);
                }
            }
        }

        return nodes;
    }

    template <int depth, bool side, bool wKMoved, bool bKMoved>
    struct PerftGenerator {
        static inline U64 generateMoves(const BoardState& board) {
            return allMoves<depth, side, wKMoved, bKMoved>(board);
        }
    };

    template <bool side, bool wKMoved, bool bKMoved>
    struct PerftGenerator<1, side, wKMoved, bKMoved> {
        ForceInline U64 generateMoves(const BoardState& board) {
            return allMoves<1, side, wKMoved, bKMoved>(board);
        }
    };

    template <bool side, bool wKMoved, bool bKMoved>
    struct PerftGenerator<0, side, wKMoved, bKMoved> {
        ForceInline U64 generateMoves(const BoardState& board) {
            return allMoves<0, side, wKMoved, bKMoved>(board);
        }
    };

}

#endif