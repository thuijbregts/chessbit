#ifndef MOVEGENERATOR_H
#define MOVEGENERATOR_H

#include "MoveArray.h"
#include "BoardState.h"
#include "Stats.h"
#include "Game.h"
#include "TranspositionTable.h"

using namespace movarray;
using namespace moveinfo;
using namespace bstate;
using namespace stats;
using namespace tt;

namespace movegen {

    /****************************************************
    * Naming conventions
    *
    * pM -> kM	|	pawn to king, current side
    * pE -> kE	|	pawn to king, opposite side
    * occM|E|B	|	occupancies (current, opposite, both)
    *
    *****************************************************/

    template <int depth, bool side, bool kMMoved, bool kEMoved, bool isStats>
    struct PerftGenerator;

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
                SetBit(pinMask, sliderSquare);
                pins |= pinMask;
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

    template <int depth, bool side, bool kMMoved, bool kEMoved, bool isStats, Piece piece, bool capture>
    ForceInline void enumMoves(U64& nodes, U64 moves, int from, const BoardState& board, Stats& stats, MoveArray& movesArray) {
        if (moves) {
            int to = SquareOf(moves);

            const BoardState newBoard = board.make<piece, side, capture>(from, to, board);
            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, capture, newBoard));
            else {
                if constexpr (piece == Piece::King) nodes += PerftGenerator<depth - 1, !side, kEMoved, true, isStats>::generateMoves(newBoard, stats, movesArray);
                else                                nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
            }

            enumMoves<depth, side, kMMoved, kEMoved, isStats, piece, capture>(nodes, _blsr_u64(moves), from, board, stats, movesArray);
        }
    }

    template <int depth, bool side, bool kMMoved, bool kEMoved, bool isStats, Piece piece>
    ForceInline void makeMoves(U64& nodes, U64 attacks, int from, const BoardState& board, Stats& stats, MoveArray& movesArray) {
        U64 moves = attacks & ~board.occE;
        enumMoves<depth, side, kMMoved, kEMoved, isStats, piece, false>(nodes, moves, from, board, stats, movesArray);

        moves = attacks & board.occE;
        enumMoves<depth, side, kMMoved, kEMoved, isStats, piece, true>(nodes, moves, from, board, stats, movesArray);
    }

    template <int depth, bool side, bool kMMoved, bool kEMoved, bool isStats, bool capture>
    ForceInline void makePromotionMoves(U64& nodes, int from, int to, const BoardState& board, Stats& stats, MoveArray& movesArray) {
        const BoardState newBoardN = board.makePromotion<Piece::Knight, side, capture>(from, to, board);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, n, capture, newBoardN));
        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoardN, stats, movesArray);

        const BoardState newBoardB = board.makePromotion<Piece::Bishop, side, capture>(from, to, board);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, b, capture, newBoardB));
        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoardB, stats, movesArray);

        const BoardState newBoardR = board.makePromotion<Piece::Rook, side, capture>(from, to, board);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, r, capture, newBoardR));
        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoardR, stats, movesArray);

        const BoardState newBoardQ = board.makePromotion<Piece::Queen, side, capture>(from, to, board);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, q, capture, newBoardQ));
        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoardQ, stats, movesArray);
    }

    template <int depth, bool side, bool kMMoved, bool kEMoved, bool isStats>
    ForceInline U64 allMoves(const BoardState& board, Stats& stats, MoveArray& movesArray) {
        Entry& e = TT[depth][board.zobrist % tt::MASK];
        if ((e.zobrist ^ e.nodes) == board.zobrist) {
            return e.nodes;
        }

        int from, to;
        U64 bitboard, attacks;

        U64 nodes = 0ULL;
        U64 castleAttacks = 0ULL;

        /*

            KING MOVES

        */
        attacks = board.kMA & ~board.occM;
        filterKingAttacks<side, kMMoved>(board, attacks, castleAttacks);
        if constexpr (depth == 1) { nodes += Bitcount(attacks); if constexpr (isStats) stats.caps += Bitcount(attacks & board.occE); }
        else makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::King>(nodes, attacks, board.kMS, board, stats, movesArray);

        if (board.checks) {
            int checkSquare = SquareOf(board.checks);

            if (Bitcount(board.checks) == 1) {
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

                    if constexpr (depth == 1) {
                        if (promos) { int count = Bitcount(promos) << 2; nodes += count; if constexpr (isStats) { stats.prom += count; stats.caps += count; } }
                        else { int count = Bitcount(enPassant | caps); nodes += count; if constexpr (isStats) { stats.eP += Bitcount(enPassant); stats.caps += count; } }
                    }
                    else {
                        caps ^= promos;
                        Bitloop(enPassant)
                        {
                            from = SquareOf(enPassant);

                            const BoardState newBoard = board.makeEnPassant<side>(from, board.eP, board);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(EnPassant, from, board.eP, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
                        }

                        Bitloop(promos) { from = SquareOf(promos); makePromotionMoves<depth, side, kMMoved, kEMoved, isStats, true>(nodes, from, to, board, stats, movesArray); }

                        Bitloop(caps) {
                            from = SquareOf(caps);

                            const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
                        }
                    }

                    /*

                       KNIGHT MOVES

                    */
                    bitboard = board.nM & ~allPins;
                    if (bitboard)
                    {
                        attacks = getKnightAttacks(checkSquare) & bitboard;
                        if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += count; }
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);

                                const BoardState newBoard = board.make<Piece::Knight, side, true>(from, to, board);
                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
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
                        if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += count; }
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);

                                const BoardState newBoard = board.make<Piece::Bishop, side, true>(from, to, board);
                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
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
                        if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += count; }
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);

                                const BoardState newBoard = board.make<Piece::Rook, side, true>(from, to, board);
                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
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
                        if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += count; }
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);

                                const BoardState newBoard = board.make<Piece::Queen, side, true>(from, to, board);
                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
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

                        if constexpr (depth == 1) {
                            int count = (Bitcount(promosLeft) + Bitcount(promosRight | promosFwd)) << 2;
                            nodes += count;
                            if constexpr (isStats) {
                                stats.prom += count;
                                stats.caps += (Bitcount(promosLeft) + Bitcount(promosRight)) << 2;
                            }
                        }
                        else {
                            Bitloop(promosLeft) { to = SquareOf(promosLeft);  from = to + PAWN_RIGHT[!side];  makePromotionMoves<depth, side, kMMoved, kEMoved, isStats, true>(nodes, from, to, board, stats, movesArray); }
                            Bitloop(promosRight) { to = SquareOf(promosRight); from = to + PAWN_LEFT[!side];   makePromotionMoves<depth, side, kMMoved, kEMoved, isStats, true>(nodes, from, to, board, stats, movesArray); }
                            Bitloop(promosFwd) { to = SquareOf(promosFwd);   from = to + PAWN_PUSH[!side];   makePromotionMoves<depth, side, kMMoved, kEMoved, isStats, false>(nodes, from, to, board, stats, movesArray); }
                        }
                    }

                    if constexpr (depth == 1) {
                        nodes += Bitcount(pawnsLeft) + Bitcount(pawnsRight | pawnsFwd | pawnsDouble);
                        if constexpr (isStats) stats.caps += Bitcount(pawnsLeft) + Bitcount(pawnsRight);
                    }
                    else {
                        Bitloop(pawnsLeft) {
                            to = SquareOf(pawnsLeft);
                            from = to + PAWN_RIGHT[!side];

                            const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
                        }

                        Bitloop(pawnsRight) {
                            to = SquareOf(pawnsRight);
                            from = to + PAWN_LEFT[!side];

                            const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
                        }

                        Bitloop(pawnsFwd) {
                            to = SquareOf(pawnsFwd);
                            from = to + PAWN_PUSH[!side];

                            const BoardState newBoard = board.make<Piece::Pawn, side, false>(from, to, board);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
                        }

                        Bitloop(pawnsDouble) {
                            to = SquareOf(pawnsDouble);
                            from = to + PAWN_DOUBLE_PUSH[!side];

                            const BoardState newBoard = board.makeDoublePush<side>(from, to, board);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
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
                        if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += Bitcount(attacks & board.occE); }
                        else makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::Knight>(nodes, attacks, from, board, stats, movesArray);
                    }

                    /*

                       BISHOP MOVES

                    */
                    bitboard = board.bM & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getBishopAttacks(from, board.occB) & validSquares;
                        if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += Bitcount(attacks & board.occE); }
                        else makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::Bishop>(nodes, attacks, from, board, stats, movesArray);
                    }

                    /*

                       ROOK MOVES

                    */
                    bitboard = board.rM & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getRookAttacks(from, board.occB) & validSquares;
                        if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += Bitcount(attacks & board.occE); }
                        else makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::Rook>(nodes, attacks, from, board, stats, movesArray);
                    }

                    /*

                       QUEEN MOVES

                    */
                    bitboard = board.qM & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getQueenAttacks(from, board.occB) & validSquares;
                        if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += Bitcount(attacks & board.occE); }
                        else makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::Queen>(nodes, attacks, from, board, stats, movesArray);
                    }
                }
            }

            if constexpr (depth == 1 && isStats) {
                stats.chk++;
                if (nodes == 0) stats.chkm++;
                else {
                    U64 to = (1ULL << board.to);
                    if (board.checks & to) {
                        U64 checks = board.checks ^ to;
                        if (checks) stats.dblchk++;
                    }
                    else stats.dischck++;
                }
            }

            e.zobrist = board.zobrist ^ nodes;
            e.nodes = nodes;

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

                if constexpr (depth == 1) { int count = Bitcount(ePBit & passantPinMask<side>(board, from)); nodes += count; if constexpr (isStats) stats.eP += count; stats.caps += count; }
                else {
                    if (ePBit & passantPinMask<side>(board, from)) {
                        const BoardState newBoard = board.makeEnPassant<side>(from, board.eP, board);
                        if constexpr (depth == 0) movesArray.add(MoveInfo(EnPassant, from, board.eP, true, newBoard));
                        else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
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

            if constexpr (depth == 1) {
                int count = (Bitcount(promosLeft) + Bitcount(promosRight | promosFwd)) << 2;
                nodes += count;
                if constexpr (isStats) {
                    stats.prom += count;
                    stats.caps += (Bitcount(promosLeft) + Bitcount(promosRight)) << 2;
                }
            }
            else {
                Bitloop(promosLeft) { to = SquareOf(promosLeft);  from = to + PAWN_RIGHT[!side];  makePromotionMoves<depth, side, kMMoved, kEMoved, isStats, true>(nodes, from, to, board, stats, movesArray); }
                Bitloop(promosRight) { to = SquareOf(promosRight); from = to + PAWN_LEFT[!side];   makePromotionMoves<depth, side, kMMoved, kEMoved, isStats, true>(nodes, from, to, board, stats, movesArray); }
                Bitloop(promosFwd) { to = SquareOf(promosFwd);   from = to + PAWN_PUSH[!side];   makePromotionMoves<depth, side, kMMoved, kEMoved, isStats, false>(nodes, from, to, board, stats, movesArray); }
            }
        }

        if constexpr (depth == 1) {
            nodes += Bitcount(pawnsLeft) + Bitcount(pawnsRight | pawnsFwd | pawnsDouble);
            if constexpr (isStats) stats.caps += Bitcount(pawnsLeft) + Bitcount(pawnsRight);
        }
        else {
            Bitloop(pawnsLeft) {
                to = SquareOf(pawnsLeft);
                from = to + PAWN_RIGHT[!side];

                const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
            }

            Bitloop(pawnsRight) {
                to = SquareOf(pawnsRight);
                from = to + PAWN_LEFT[!side];

                const BoardState newBoard = board.make<Piece::Pawn, side, true>(from, to, board);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
            }

            Bitloop(pawnsFwd) {
                to = SquareOf(pawnsFwd);
                from = to + PAWN_PUSH[!side];

                const BoardState newBoard = board.make<Piece::Pawn, side, false>(from, to, board);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
            }

            Bitloop(pawnsDouble) {
                to = SquareOf(pawnsDouble);
                from = to + PAWN_DOUBLE_PUSH[!side];

                const BoardState newBoard = board.makeDoublePush<side>(from, to, board);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kEMoved, kMMoved, isStats>::generateMoves(newBoard, stats, movesArray);
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
            if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += Bitcount(attacks & board.occE); }
            else makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::Knight>(nodes, attacks, from, board, stats, movesArray);
        }

        /*

           BISHOP MOVES

        */
        bitboard = board.bM & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getBishopAttacks(from, board.occB) & ~board.occM;
            if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += Bitcount(attacks & board.occE); }
            else makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::Bishop>(nodes, attacks, from, board, stats, movesArray);
        }

        bitboard = (board.bM | board.qM) & bPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getBishopAttacks(from, board.occB) & bPins;
            if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += Bitcount(attacks & board.occE); }
            else {
                if ((1ULL << from) & board.qM)  makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::Queen>(nodes, attacks, from, board, stats, movesArray);
                else                            makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::Bishop>(nodes, attacks, from, board, stats, movesArray);
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
            if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += Bitcount(attacks & board.occE); }
            else makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::Rook>(nodes, attacks, from, board, stats, movesArray);
        }

        bitboard = (board.rM | board.qM) & rPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getRookAttacks(from, board.occB) & rPins;
            if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += Bitcount(attacks & board.occE); }
            else {
                if ((1ULL << from) & board.qM)  makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::Queen>(nodes, attacks, from, board, stats, movesArray);
                else                            makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::Rook>(nodes, attacks, from, board, stats, movesArray);
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
            if constexpr (depth == 1) { int count = Bitcount(attacks); nodes += count; if constexpr (isStats) stats.caps += Bitcount(attacks & board.occE); }
            else makeMoves<depth, side, kMMoved, kEMoved, isStats, Piece::Queen>(nodes, attacks, from, board, stats, movesArray);
        }

        if constexpr (!kMMoved) {
            if (castle<CASTLING_SIDE_K[side]>(board, castleAttacks)) {
                if constexpr (depth == 1) { nodes++; if constexpr (isStats) stats.cstl++; }
                else {
                    const BoardState newBoard = board.makeCastling<CASTLING_SIDE_K[side]>(board);
                    if constexpr (depth == 0) movesArray.add(MoveInfo(Castle, CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_K[side]], false, newBoard));
                    else nodes += PerftGenerator<depth - 1, !side, kEMoved, true, isStats>::generateMoves(newBoard, stats, movesArray);
                }
            }

            if (castle<CASTLING_SIDE_Q[side]>(board, castleAttacks)) {
                if constexpr (depth == 1) { nodes++; if constexpr (isStats) stats.cstl++; }
                else {
                    const BoardState newBoard = board.makeCastling<CASTLING_SIDE_Q[side]>(board);
                    if constexpr (depth == 0) movesArray.add(MoveInfo(Castle, CASTLING_KING_TARGET_SQUARE[CASTLING_SIDE_Q[side]], false, newBoard));
                    else nodes += PerftGenerator<depth - 1, !side, kEMoved, true, isStats>::generateMoves(newBoard, stats, movesArray);
                }
            }
        }

        e.zobrist = board.zobrist ^ nodes;
        e.nodes = nodes;

        return nodes;
    }

    template <int depth, bool side, bool kMMoved, bool kEMoved, bool isStats>
    struct PerftGenerator {
        static inline U64 generateMoves(const BoardState& board, Stats& stats, MoveArray& movesArray) {
            return allMoves<depth, side, kMMoved, kEMoved, isStats>(board, stats, movesArray);
        }
    };

    template <bool side, bool kMMoved, bool kEMoved, bool isStats>
    struct PerftGenerator<1, side, kMMoved, kEMoved, isStats> {
        ForceInline U64 generateMoves(const BoardState& board, Stats& stats, MoveArray& movesArray) {
            return allMoves<1, side, kMMoved, kEMoved, isStats>(board, stats, movesArray);
        }
    };

    template <bool side, bool kMMoved, bool kEMoved, bool isStats>
    struct PerftGenerator<0, side, kMMoved, kEMoved, isStats> {
        ForceInline U64 generateMoves(const BoardState& board, Stats& stats, MoveArray& movesArray) {
            return allMoves<0, side, kMMoved, kEMoved, isStats>(board, stats, movesArray);
        }
    };

}

#endif