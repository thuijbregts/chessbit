#ifndef MOVEGENERATOR_H
#define MOVEGENERATOR_H

#include "MoveArray.h"
#include "BoardState.h"
#include "Game.h"
#include <vector>

using namespace movarray;
using namespace moveinfo;
using namespace bstate;

namespace movegen {

    /****************************************************
    * Naming conventions
    *
    * pM -> kM	|	pawn to king, current side
    * pE -> kE	|	pawn to king, opposite side
    * occM|E|B	|	occupancies (current, opposite, both)
    *
    *****************************************************/

    //union of pinMask and the slider piece square, where index is the square of the pinned piece
    static inline U64 validAttacksMasks[100][64];

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

    template <bool side, bool kMMoved, bool kEMoved>
    ForceInline void filterKingAttacks(U64 occM, U64 occB, int kMS, U64& kingAttacks, U64& castleAttacks, U64 pE, U64 nE, U64 bE, U64 rE, U64 qE, U64 kEA, U64 mask) {
        U64 attacks = 0ULL;
        attacks |= pawnsAtkLeft<!side>(pE & ~FIRST_COL) | pawnsAtkRight<!side>(pE & ~LAST_COL);
        attacks |= kEA;

        //remove king to avoid collisions, as it should not be considered when checking for threats
        PopBit(occB, kMS);

        U64 bitboard;
        if constexpr (!kMMoved) bitboard = nE & KNIGHT_ATTACK_ZONE_CASTLE[side];
        else bitboard = nE & KNIGHT_ATTACK_ZONES[kMS];
        Bitloop(bitboard) {
            attacks |= getKnightAttacks(SquareOf(bitboard));
        }

        if constexpr (!kMMoved) bitboard = (bE | qE) & getBishopAttackZoneCastle<side>(occM);
        else bitboard = (bE | qE) & getBishopAttackZone(kMS, occM, mask);
        Bitloop(bitboard) {
            attacks |= getBishopAttacks(SquareOf(bitboard), occB);
        }

        if constexpr (!kMMoved) bitboard = (rE | qE) & getRookAttackZoneCastle<side>(occM);
        else bitboard = (rE | qE) & getRookAttackZone(kMS, occM, mask);
        Bitloop(bitboard) {
            attacks |= getRookAttacks(SquareOf(bitboard), occB);
        }

        kingAttacks &= ~attacks;

        if constexpr (!kMMoved) castleAttacks = attacks;
    }

    template <int castlingSide>
    ForceInline bool castle(int casPerm, U64 occE, U64 occB, U64 nE, U64 bE, U64 rE, U64 qE, U64 attacks) {
        return !(!(casPerm & CASTLING[castlingSide]) | (CASTLING_OCCUPIED_SQUARES[castlingSide] & occB) | (attacks & CASTLING_PASSING_SQUARES[castlingSide]));
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

    template <int depth, bool side, bool kMMoved, bool kEMoved>
    struct PerftGenerator;

    template <int depth, bool side, bool kMMoved, bool kEMoved, Piece piece, bool capture>
    ForceInline void enumMoves(U64& nodes, U64 moves, int from, const BoardState& board, int kES) {
        if (moves) {
            int to = SquareOf(moves);

            const BoardState newBoard = board.make<piece, side, capture>(from, to, board, kES);
            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, capture, newBoard));
            else {
                if constexpr (piece == Piece::King) nodes += PerftGenerator<depth - 1, !side, kEMoved, true>::generateMoves(newBoard);
                else                                nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
            }

            enumMoves<depth, side, kMMoved, kEMoved, piece, capture>(nodes, _blsr_u64(moves), from, board, kES);
        }
    }

    template <int depth, bool side, bool kMMoved, bool kEMoved, Piece piece>
    ForceInline void makeMoves(U64& nodes, U64 attacks, int from, const BoardState& board, int kES) {
        U64 moves = attacks & ~board.occE;
        enumMoves<depth, side, kMMoved, kEMoved, piece, false>(nodes, moves, from, board, kES);

        moves = attacks & board.occE;
        enumMoves<depth, side, kMMoved, kEMoved, piece, true>(nodes, moves, from, board, kES);
    }

    template <int depth, bool side, bool kMMoved, bool kEMoved, bool capture>
    ForceInline void makePromotionMoves(U64& nodes, int from, int to, const BoardState& board, int kES) {
        const BoardState newBoardN = board.makePromotion<Piece::Knight, side, capture>(from, to, board, kES);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, n, capture, newBoardN));
        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoardN);

        const BoardState newBoardB = board.makePromotion<Piece::Bishop, side, capture>(from, to, board, kES);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, b, capture, newBoardB));
        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoardB);

        const BoardState newBoardR = board.makePromotion<Piece::Rook, side, capture>(from, to, board, kES);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, r, capture, newBoardR));
        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoardR);

        const BoardState newBoardQ = board.makePromotion<Piece::Queen, side, capture>(from, to, board, kES);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, q, capture, newBoardQ));
        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoardQ);
    }

    template <int depth, bool side, bool kMMoved, bool kEMoved>
    ForceInline U64 allMoves(const BoardState& board) {
        int from, to;
        U64 bitboard, attacks;

        U64 nodes = 0ULL;
        U64 castleAttacks = 0ULL;

        const int kMS = SquareOf(board.kM);
        const int kES = SquareOf(board.kE);

        /*

            KING MOVES

        */
        U64 mask = board.kMA;
        attacks = mask & ~board.occM;
        filterKingAttacks<side, kMMoved, kEMoved>(board.occM, board.occB, kMS, attacks, castleAttacks, board.pE, board.nE, board.bE, board.rE, board.qE, board.kEA, mask);
        if constexpr (depth == 1) nodes += Bitcount(attacks);
        else makeMoves<depth, side, kMMoved, kEMoved, Piece::King>(nodes, attacks, kMS, board, kES);

        if (board.checks) {
            int checkSquare = SquareOf(board.checks);

            if (Bitcount(board.checks) == 1) {
                const U64 bPins = findBishopPins<depth>(board.occB, board.bE, board.qE, kMS);
                const U64 rPins = findRookPins<depth>(board.occB, board.rE, board.qE, kMS);
                const U64 allPins = bPins | rPins;
                if (board.checks & (board.pE | board.nE)) {
                    /*

                       PAWN MOVES

                    */
                    to = checkSquare;

                    const U64 pawns = board.pM & ~allPins;
                    U64 enPassant = pawns & PASSANT_CAPTURES[board.eP];
                    U64 caps = pawns & PAWN_CAPTURES[!side][checkSquare];
                    U64 promos = caps & PROMO_RANKS[side];
      
                    if constexpr (depth == 1) {
                        if (promos) nodes += Bitcount(promos) * 4;
                        else nodes += Bitcount(enPassant | caps);
                    }
                    else {
                        caps ^= promos;
                        Bitloop(enPassant)
                        {
                            from = SquareOf(enPassant);

                            const BoardState newBoard = board.makeEnPassant<side>(from, board.eP, board, kES);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, board.eP, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                        }

                        Bitloop(promos) { from = SquareOf(promos); makePromotionMoves<depth, side, kMMoved, kEMoved, true>(nodes, from, to, board, kES); }

                        Bitloop(caps) {
                            from = SquareOf(caps);

                            const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                        }
                    }

                    /*

                       KNIGHT MOVES

                    */
                    bitboard = board.nM & ~allPins;
                    if (bitboard)
                    {
                        attacks = getKnightAttacks(checkSquare) & bitboard;
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);

                                const BoardState newBoard = board.make<Piece::Knight, side, true>(from, to, board, kES);
                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                            }
                        }
                    }

                    /*

                       BISHOP MOVES

                    */
                    bitboard = board.bM & ~allPins;
                    if (bitboard)
                    {
                        attacks = getBishopAttacks(checkSquare, board.occB) & bitboard;
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);

                                const BoardState newBoard = board.make<Piece::Bishop, side, true>(from, to, board, kES);
                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                            }
                        }
                    }

                    /*

                       ROOK MOVES

                    */
                    bitboard = board.rM & ~allPins;
                    if (bitboard)
                    {
                        attacks = getRookAttacks(checkSquare, board.occB) & bitboard;
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);

                                const BoardState newBoard = board.make<Piece::Rook, side, true>(from, to, board, kES);
                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                            }
                        }
                    }

                    /*

                       QUEEN MOVES

                    */
                    bitboard = board.qM & ~allPins;
                    if (bitboard)
                    {
                        attacks = getQueenAttacks(checkSquare, board.occB) & bitboard;
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);

                                const BoardState newBoard = board.make<Piece::Queen, side, true>(from, to, board, kES);
                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                            }
                        }
                    }

                }
                else {
                    const U64 validSquares = (board.checks | PIN_MASKS[checkSquare][kMS]);

                    /*

                       PAWN MOVES

                    */
                    const U64 pawns = board.pM & ~allPins;

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

                        if constexpr (depth == 1) nodes += (Bitcount(promosLeft) + Bitcount(promosRight | promosFwd)) * 4;
                        else {
                            Bitloop(promosLeft) { to = SquareOf(promosLeft);  from = to + PAWN_RIGHT[!side];  makePromotionMoves<depth, side, kMMoved, kEMoved, true>(nodes, from, to, board, kES); }
                            Bitloop(promosRight) { to = SquareOf(promosRight); from = to + PAWN_LEFT[!side];   makePromotionMoves<depth, side, kMMoved, kEMoved, true>(nodes, from, to, board, kES); }
                            Bitloop(promosFwd) { to = SquareOf(promosFwd);   from = to + PAWN_PUSH[!side];   makePromotionMoves<depth, side, kMMoved, kEMoved, false>(nodes, from, to, board, kES); }
                        }
                    }

                    if constexpr (depth == 1) {
                        nodes += Bitcount(pawnsLeft);
                        nodes += Bitcount(pawnsRight | pawnsFwd | pawnsDouble);
                    }
                    else {
                        Bitloop(pawnsLeft) {
                            to = SquareOf(pawnsLeft);
                            from = to + PAWN_RIGHT[!side];

                            const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsRight) {
                            to = SquareOf(pawnsRight);
                            from = to + PAWN_LEFT[!side];

                            const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsFwd) {
                            to = SquareOf(pawnsFwd);
                            from = to + PAWN_PUSH[!side];

                            const BoardState newBoard = board.make<Piece::Pawn, side, false>(from, to, board, kES);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsDouble) {
                            to = SquareOf(pawnsDouble);
                            from = to + PAWN_DOUBLE_PUSH[!side];

                            const BoardState newBoard = board.makeDoublePush<side>(from, to, board, kES);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
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
                        else makeMoves<depth, side, kMMoved, kEMoved, Piece::Knight>(nodes, attacks, from, board, kES);
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
                        else makeMoves<depth, side, kMMoved, kEMoved, Piece::Bishop>(nodes, attacks, from, board, kES);
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
                        else makeMoves<depth, side, kMMoved, kEMoved, Piece::Rook>(nodes, attacks, from, board, kES);
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
                        else makeMoves<depth, side, kMMoved, kEMoved, Piece::Queen>(nodes, attacks, from, board, kES);
                    }
                }
            }

            return nodes;
        }

        const U64 bPins = findBishopPins<depth>(board.occB, board.bE, board.qE, kMS);
        const U64 rPins = findRookPins<depth>(board.occB, board.rE, board.qE, kMS);
        const U64 allPins = bPins | rPins;

        /*

           PAWN MOVES

        */
        const U64 pawnsAtk = board.pM & ~rPins;
        const U64 pawnsPush = board.pM & ~bPins;

        const U64 pawnsLeftAll = pawnsAtkLeft<side>(pawnsAtk & ~bPins & ~FIRST_COL) | (pawnsAtkLeft<side>(pawnsAtk & bPins & ~FIRST_COL) & bPins);
        U64 pawnsLeft = pawnsLeftAll & board.occE;
        const U64 pawnsRightAll = pawnsAtkRight<side>(pawnsAtk & ~bPins & ~LAST_COL) | (pawnsAtkRight<side>(pawnsAtk & bPins & ~LAST_COL) & bPins);
        U64 pawnsRight = pawnsRightAll & board.occE;
        U64 pawnsFwd = (pawnsAtkForward<side>(pawnsPush & ~rPins) & ~board.occB) | (pawnsAtkForward<side>(pawnsPush & rPins) & ~board.occB & rPins);
        U64 pawnsDouble = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~board.occB;

        if (board.eP != noSquare) {
            const U64 ePBit = (1ULL << board.eP);
            U64 ePP = pawnsAtkRight<!side>(pawnsLeftAll & ePBit) | pawnsAtkLeft<!side>(pawnsRightAll & ePBit);
            Bitloop(ePP) {
                from = SquareOf(ePP);

                if constexpr (depth == 1) nodes += Bitcount(ePBit & passantPinMask<side>(board.eP, from, board.occB, board.kM, board.rE, board.qE, kMS));
                else {
                    if (ePBit & passantPinMask<side>(board.eP, from, board.occB, board.kM, board.rE, board.qE, kMS)) {
                        const BoardState newBoard = board.makeEnPassant<side>(from, board.eP, board, kES);
                        if constexpr (depth == 0) movesArray.add(MoveInfo(from, board.eP, true, newBoard));
                        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                    }
                }
            }
        }

        if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANKS[side]) {
            U64 promosLeft = pawnsLeft & LAST_RANKS[side];
            U64 promosRight = pawnsRight & LAST_RANKS[side];
            U64 promosFwd = pawnsFwd & LAST_RANKS[side];

            pawnsLeft ^= promosLeft;
            pawnsRight ^= promosRight;
            pawnsFwd ^= promosFwd;

            if constexpr (depth == 1) nodes += (Bitcount(promosLeft) + Bitcount(promosRight | promosFwd)) * 4;
            else {
                Bitloop(promosLeft) { to = SquareOf(promosLeft);  from = to + PAWN_RIGHT[!side];  makePromotionMoves<depth, side, kMMoved, kEMoved, true>(nodes, from, to, board, kES); }
                Bitloop(promosRight) { to = SquareOf(promosRight); from = to + PAWN_LEFT[!side];   makePromotionMoves<depth, side, kMMoved, kEMoved, true>(nodes, from, to, board, kES); }
                Bitloop(promosFwd) { to = SquareOf(promosFwd);   from = to + PAWN_PUSH[!side];   makePromotionMoves<depth, side, kMMoved, kEMoved, false>(nodes, from, to, board, kES); }
            }
        }

        if constexpr (depth == 1) {
            nodes += Bitcount(pawnsLeft);
            nodes += Bitcount(pawnsRight | pawnsFwd | pawnsDouble);
        }
        else {
            Bitloop(pawnsLeft) {
                to = SquareOf(pawnsLeft);
                from = to + PAWN_RIGHT[!side];

                const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsRight) {
                to = SquareOf(pawnsRight);
                from = to + PAWN_LEFT[!side];

                const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board, kES);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsFwd) {
                to = SquareOf(pawnsFwd);
                from = to + PAWN_PUSH[!side];

                const BoardState newBoard = board.make<Piece::Pawn, side, false>(from, to, board, kES);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsDouble) {
                to = SquareOf(pawnsDouble);
                from = to + PAWN_DOUBLE_PUSH[!side];

                const BoardState newBoard = board.makeDoublePush<side>(from, to, board, kES);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
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
            else makeMoves<depth, side, kMMoved, kEMoved, Piece::Knight>(nodes, attacks, from, board, kES);
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
            else makeMoves<depth, side, kMMoved, kEMoved, Piece::Bishop>(nodes, attacks, from, board, kES);
        }

        bitboard = (board.bM | board.qM) & bPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = validAttacksMasks[depth][from];
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else {
                if ((1ULL << from) & board.qM)  makeMoves<depth, side, kMMoved, kEMoved, Piece::Queen>(nodes, attacks, from, board, kES);
                else                            makeMoves<depth, side, kMMoved, kEMoved, Piece::Bishop>(nodes, attacks, from, board, kES);
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
            else makeMoves<depth, side, kMMoved, kEMoved, Piece::Rook>(nodes, attacks, from, board, kES);
        }

        bitboard = (board.rM | board.qM) & rPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = validAttacksMasks[depth][from];
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else {
                if ((1ULL << from) & board.qM)  makeMoves<depth, side, kMMoved, kEMoved, Piece::Queen>(nodes, attacks, from, board, kES);
                else                            makeMoves<depth, side, kMMoved, kEMoved, Piece::Rook>(nodes, attacks, from, board, kES);
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
            else makeMoves<depth, side, kMMoved, kEMoved, Piece::Queen>(nodes, attacks, from, board, kES);
        }

        if constexpr (!kMMoved) {
            if (castle<CASTLING_SIDE_K[side]>(board.casPerms, board.occE, board.occB, board.nE, board.bE, board.rE, board.qE, castleAttacks)) {
                if constexpr (depth == 1) nodes++;
                else {
                    const BoardState newBoard = board.makeCastling<CASTLING_SIDE_K[side]>(board, kES);
                    if constexpr (depth == 0) movesArray.add(MoveInfo(kMS, CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_K[side]], false, newBoard));
                    else nodes += PerftGenerator<depth - 1, !side, kEMoved, true>::generateMoves(newBoard);
                }
            }

            if (castle<CASTLING_SIDE_Q[side]>(board.casPerms, board.occE, board.occB, board.nE, board.bE, board.rE, board.qE, castleAttacks)) {
                if constexpr (depth == 1) nodes++;
                else {
                    const BoardState newBoard = board.makeCastling<CASTLING_SIDE_Q[side]>(board, kES);
                    if constexpr (depth == 0) movesArray.add(MoveInfo(kMS, CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_Q[side]], false, newBoard));
                    else nodes += PerftGenerator<depth - 1, !side, kEMoved, true>::generateMoves(newBoard);
                }
            }
        }

        return nodes;
    }

    template <int depth, bool side, bool kMMoved, bool kEMoved>
    struct PerftGenerator {
        static inline U64 generateMoves(const BoardState& board) {
            return allMoves<depth, side, kMMoved, kEMoved>(board);
        }
    };

    template <bool side, bool kMMoved, bool kEMoved>
    struct PerftGenerator<1, side, kMMoved, kEMoved> {
        ForceInline U64 generateMoves(const BoardState& board) {
            return allMoves<1, side, kMMoved, kEMoved>(board);
        }
    };

    template <bool side, bool kMMoved, bool kEMoved>
    struct PerftGenerator<0, side, kMMoved, kEMoved> {
        ForceInline U64 generateMoves(const BoardState& board) {
            return allMoves<0, side, kMMoved, kEMoved>(board);
        }
    };

}

#endif