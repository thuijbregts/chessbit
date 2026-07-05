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

    template <bool side, bool kMMoved>
    ForceInline void filterKingAttacks(const BoardState& board, U64& kingAttacks, U64& castleAttacks) {
        U64 attacks = 0ULL;
        attacks |= pawnsAtkLeft<!side>(board.pE & ~FIRST_COL) | pawnsAtkRight<!side>(board.pE & ~LAST_COL);
        attacks |= board.kEA;

        //remove king to avoid collisions, as it should not be considered when checking for threats
        U64 occB = board.occB ^ board.kM;

        U64 bitboard;
        if constexpr (!kMMoved) bitboard = board.nE & KNIGHT_ATTACK_ZONE_CASTLE[side];
        else                    bitboard = board.nE & KNIGHT_ATTACK_ZONES[board.kMS];
        Bitloop(bitboard) {
            attacks |= getKnightAttacks(SquareOf(bitboard));
        }

        if constexpr (!kMMoved) bitboard = (board.bE | board.qE) & getBishopAttackZoneCastle<side>(board.occM);
        else                    bitboard = (board.bE | board.qE) & getBishopAttackZone(board.kMS, board.occM, board.kMA);
        Bitloop(bitboard) {
            attacks |= getBishopAttacks(SquareOf(bitboard), occB);
        }

        if constexpr (!kMMoved) bitboard = (board.rE | board.qE) & getRookAttackZoneCastle<side>(board.occM);
        else                    bitboard = (board.rE | board.qE) & getRookAttackZone(board.kMS, board.occM, board.kMA);
        Bitloop(bitboard) {
            attacks |= getRookAttacks(SquareOf(bitboard), occB);
        }

        kingAttacks &= ~attacks;

        if constexpr (!kMMoved) castleAttacks = attacks;
    }

    template <int castlingSide>
    ForceInline bool castle(const BoardState& board, U64 attacks) {
        return !(!(board.casPerms & CASTLING[castlingSide]) | (CASTLING_OCCUPIED_SQUARES[castlingSide] & board.occB) | (attacks & CASTLING_PASSING_SQUARES[castlingSide]));
    }

    template <bool side>
    ForceInline U64 passantPinMask(const BoardState& board, int from) {
        if (!(EN_PASSANT_RANK[side] & board.kM)) {
            return FULL_BOARD;
        }

        U64 occB = board.occB;
        int enemyPawn = board.eP + PAWN_PUSH[!side];
        PopBit(occB, enemyPawn);
        PopBit(occB, from);

        return PASSANT_PIN_RESULT[SquareOf(getRookAttacks(board.kMS, occB) & (board.rE | board.qE))];
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
    ForceInline U64 findBishopPins(const BoardState& board) {
        return iteratePieces<depth>((board.bE | board.qE) & BISHOP_XRAYS[board.kMS] & ~board.checks, board.occB, board.kMS);
    }

    template <int depth>
    ForceInline U64 findRookPins(const BoardState& board) {
        return iteratePieces<depth>((board.rE | board.qE) & ROOK_XRAYS[board.kMS] & ~board.checks, board.occB, board.kMS);
    }

    template <int depth, bool side, bool kMMoved, bool kEMoved>
    struct PerftGenerator;

    template <int depth, bool side, bool kMMoved, bool kEMoved, Piece piece, bool capture>
    ForceInline void enumMoves(U64& nodes, U64 moves, int from, const BoardState& board) {
        if (moves) {
            int to = SquareOf(moves);

            const BoardState newBoard = board.make<piece, side, capture>(from, to, board);
            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, capture, newBoard));
            else {
                if constexpr (piece == Piece::King) nodes += PerftGenerator<depth - 1, !side, kEMoved, true>::generateMoves(newBoard);
                else                                nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
            }

            enumMoves<depth, side, kMMoved, kEMoved, piece, capture>(nodes, _blsr_u64(moves), from, board);
        }
    }

    template <int depth, bool side, bool kMMoved, bool kEMoved, Piece piece>
    ForceInline void makeMoves(U64& nodes, U64 attacks, int from, const BoardState& board) {
        U64 moves = attacks & ~board.occE;
        enumMoves<depth, side, kMMoved, kEMoved, piece, false>(nodes, moves, from, board);

        moves = attacks & board.occE;
        enumMoves<depth, side, kMMoved, kEMoved, piece, true>(nodes, moves, from, board);
    }

    template <int depth, bool side, bool kMMoved, bool kEMoved, bool capture>
    ForceInline void makePromotionMoves(U64& nodes, int from, int to, const BoardState& board) {
        const BoardState newBoardN = board.makePromotion<Piece::Knight, side, capture>(from, to, board);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, n, capture, newBoardN));
        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoardN);

        const BoardState newBoardB = board.makePromotion<Piece::Bishop, side, capture>(from, to, board);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, b, capture, newBoardB));
        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoardB);

        const BoardState newBoardR = board.makePromotion<Piece::Rook, side, capture>(from, to, board);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, r, capture, newBoardR));
        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoardR);

        const BoardState newBoardQ = board.makePromotion<Piece::Queen, side, capture>(from, to, board);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, q, capture, newBoardQ));
        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoardQ);
    }

    template <int depth, bool side, bool kMMoved, bool kEMoved>
    ForceInline U64 allMoves(const BoardState& board) {
        int from, to;
        U64 bitboard, attacks;

        U64 nodes = 0ULL;
        U64 castleAttacks = 0ULL;

        /*

            KING MOVES

        */
        attacks = board.kMA & ~board.occM;
        filterKingAttacks<side, kMMoved>(board, attacks, castleAttacks);
        if constexpr (depth == 1) nodes += Bitcount(attacks);
        else makeMoves<depth, side, kMMoved, kEMoved, Piece::King>(nodes, attacks, board.kMS, board);

        if (board.checks) {
            if (Bitcount(board.checks) == 1) {
                int checkSquare = SquareOf(board.checks);

                const U64 bPins = findBishopPins<depth>(board);
                const U64 rPins = findRookPins<depth>(board);
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
                    caps ^= promos;

                    if constexpr (depth == 1) nodes += Bitcount(enPassant | caps) + (Bitcount(promos) << 2);
                    else {
                        Bitloop(enPassant)
                        {
                            from = SquareOf(enPassant);

                            const BoardState newBoard = board.makeEnPassant<side>(from, board.eP, board);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(EnPassant, from, board.eP, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                        }

                        Bitloop(promos) { from = SquareOf(promos); makePromotionMoves<depth, side, kMMoved, kEMoved, true>(nodes, from, to, board); }

                        Bitloop(caps) {
                            from = SquareOf(caps);

                            const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board);
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

                                const BoardState newBoard = board.make<Piece::Knight, side, true>(from, to, board);
                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                            }
                        }
                    }

                    /*

                       BISHOP MOVES

                    */
                    bitboard = (board.bM | board.qM) & ~allPins;
                    if (bitboard)
                    {
                        attacks = getBishopAttacks(checkSquare, board.occB) & bitboard;
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);

                                const BoardState newBoard =
                                    ((1ULL << from) & board.qM)
                                    ? board.make<Piece::Queen, side, true>(from, to, board)
                                    : board.make<Piece::Bishop, side, true>(from, to, board);

                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                            }
                        }
                    }

                    /*

                       ROOK MOVES

                    */
                    bitboard = (board.rM | board.qM) & ~allPins;
                    if (bitboard)
                    {
                        attacks = getRookAttacks(checkSquare, board.occB) & bitboard;
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);

                                const BoardState newBoard =
                                    ((1ULL << from) & board.qM)
                                    ? board.make<Piece::Queen, side, true>(from, to, board)
                                    : board.make<Piece::Rook, side, true>(from, to, board);

                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                            }
                        }
                    }
                }
                else {
                    const U64 validSquares = (board.checks | PIN_MASKS[checkSquare][board.kMS]);

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

                        if constexpr (depth == 1) nodes += (Bitcount(promosLeft) + Bitcount(promosRight | promosFwd)) << 2;
                        else {
                            Bitloop(promosLeft) {   to = SquareOf(promosLeft);  from = to + PAWN_RIGHT[!side];  makePromotionMoves<depth, side, kMMoved, kEMoved, true>(nodes, from, to, board); }
                            Bitloop(promosRight) {  to = SquareOf(promosRight); from = to + PAWN_LEFT[!side];   makePromotionMoves<depth, side, kMMoved, kEMoved, true>(nodes, from, to, board); }
                            Bitloop(promosFwd) {    to = SquareOf(promosFwd);   from = to + PAWN_PUSH[!side];   makePromotionMoves<depth, side, kMMoved, kEMoved, false>(nodes, from, to, board); }
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

                            const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsRight) {
                            to = SquareOf(pawnsRight);
                            from = to + PAWN_LEFT[!side];

                            const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsFwd) {
                            to = SquareOf(pawnsFwd);
                            from = to + PAWN_PUSH[!side];

                            const BoardState newBoard = board.make<Piece::Pawn, side, false>(from, to, board);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsDouble) {
                            to = SquareOf(pawnsDouble);
                            from = to + PAWN_DOUBLE_PUSH[!side];

                            const BoardState newBoard = board.makeDoublePush<side>(from, to, board);
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
                        else makeMoves<depth, side, kMMoved, kEMoved, Piece::Knight>(nodes, attacks, from, board);
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
                        else makeMoves<depth, side, kMMoved, kEMoved, Piece::Bishop>(nodes, attacks, from, board);
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
                        else makeMoves<depth, side, kMMoved, kEMoved, Piece::Rook>(nodes, attacks, from, board);
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
                        else makeMoves<depth, side, kMMoved, kEMoved, Piece::Queen>(nodes, attacks, from, board);
                    }
                }
            }

            return nodes;
        }

        const U64 bPins = findBishopPins<depth>(board);
        const U64 rPins = findRookPins<depth>(board);
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

                if constexpr (depth == 1) nodes += Bitcount(ePBit & passantPinMask<side>(board, from));
                else {
                    if (ePBit & passantPinMask<side>(board, from)) {
                        const BoardState newBoard = board.makeEnPassant<side>(from, board.eP, board);
                        if constexpr (depth == 0) movesArray.add(MoveInfo(EnPassant, from, board.eP, true, newBoard));
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

            if constexpr (depth == 1) nodes += (Bitcount(promosLeft) + Bitcount(promosRight | promosFwd)) << 2;
            else {
                Bitloop(promosLeft) {   to = SquareOf(promosLeft);  from = to + PAWN_RIGHT[!side];  makePromotionMoves<depth, side, kMMoved, kEMoved, true>(nodes, from, to, board); }
                Bitloop(promosRight) {  to = SquareOf(promosRight); from = to + PAWN_LEFT[!side];   makePromotionMoves<depth, side, kMMoved, kEMoved, true>(nodes, from, to, board); }
                Bitloop(promosFwd) {    to = SquareOf(promosFwd);   from = to + PAWN_PUSH[!side];   makePromotionMoves<depth, side, kMMoved, kEMoved, false>(nodes, from, to, board); }
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

                const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsRight) {
                to = SquareOf(pawnsRight);
                from = to + PAWN_LEFT[!side];

                const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsFwd) {
                to = SquareOf(pawnsFwd);
                from = to + PAWN_PUSH[!side];

                const BoardState newBoard = board.make<Piece::Pawn, side, false>(from, to, board);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsDouble) {
                to = SquareOf(pawnsDouble);
                from = to + PAWN_DOUBLE_PUSH[!side];

                const BoardState newBoard = board.makeDoublePush<side>(from, to, board);
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
            else makeMoves<depth, side, kMMoved, kEMoved, Piece::Knight>(nodes, attacks, from, board);
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
            else makeMoves<depth, side, kMMoved, kEMoved, Piece::Bishop>(nodes, attacks, from, board);
        }

        bitboard = (board.bM | board.qM) & bPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = validAttacksMasks[depth][from];
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else {
                if ((1ULL << from) & board.qM)  makeMoves<depth, side, kMMoved, kEMoved, Piece::Queen>(nodes, attacks, from, board);
                else                            makeMoves<depth, side, kMMoved, kEMoved, Piece::Bishop>(nodes, attacks, from, board);
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
            else makeMoves<depth, side, kMMoved, kEMoved, Piece::Rook>(nodes, attacks, from, board);
        }

        bitboard = (board.rM | board.qM) & rPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = validAttacksMasks[depth][from];
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else {
                if ((1ULL << from) & board.qM)  makeMoves<depth, side, kMMoved, kEMoved, Piece::Queen>(nodes, attacks, from, board);
                else                            makeMoves<depth, side, kMMoved, kEMoved, Piece::Rook>(nodes, attacks, from, board);
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
            else makeMoves<depth, side, kMMoved, kEMoved, Piece::Queen>(nodes, attacks, from, board);
        }

        if constexpr (!kMMoved) {
            if (castle<CASTLING_SIDE_K[side]>(board, castleAttacks)) {
                if constexpr (depth == 1) nodes++;
                else {
                    const BoardState newBoard = board.makeCastling<CASTLING_SIDE_K[side]>(board);
                    if constexpr (depth == 0) movesArray.add(MoveInfo(Castle, CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_K[side]], false, newBoard));
                    else nodes += PerftGenerator<depth - 1, !side, kEMoved, true>::generateMoves(newBoard);
                }
            }

            if (castle<CASTLING_SIDE_Q[side]>(board, castleAttacks)) {
                if constexpr (depth == 1) nodes++;
                else {
                    const BoardState newBoard = board.makeCastling<CASTLING_SIDE_Q[side]>(board);
                    if constexpr (depth == 0) movesArray.add(MoveInfo(Castle, CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_Q[side]], false, newBoard));
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