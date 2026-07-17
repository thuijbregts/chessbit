#ifndef MOVEGENERATOR_H
#define MOVEGENERATOR_H

#include "MoveArray.h"
#include "BoardState.h"
#include "TranspositionTable.h"
#include "Game.h"
#include <vector>

using namespace movarray;
using namespace moveinfo;
using namespace bstate;
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

    //union of pinMask and the slider piece square, where index is the square of the pinned piece
    alignas(64) static inline U64 validAttacksMasks[100][64];
    alignas(64) static inline U64 discoverMasks[100][65];

    template <bool side, bool kMMoved>
    ForceInline U64 enemyAttacks(const BoardState& board) {
        U64 attacks = 0ULL;
        attacks |= pawnsAtkLeft<!side>(board.pE) | pawnsAtkRight<!side>(board.pE);
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

        return attacks;
    }

    template <int castlingSide>
    ForceInline bool castle(const BoardState& board, U64 attacks) {
        return !(!(board.casPerms & CASTLING[castlingSide]) | (CASTLING_OCCUPIED_SQUARES[castlingSide] & board.occB) | (attacks & CASTLING_PASSING_SQUARES[castlingSide]));
    }

    template <bool side>
    ForceInline bool passantPinned(const BoardState& board, int from) {
        if (!(EN_PASSANT_RANK[side] & board.kM)) return false;

        U64 occB = board.occB;
        int enemyPawn = board.eP + PAWN_PUSH[!side];
        PopBit(occB, enemyPawn);
        PopBit(occB, from);

        return getRookAttacks(board.kMS, occB) & (board.rE | board.qE);
    }

    template <int depth>
    ForceInline U64 findPins(U64 sE, const BoardState& board) {
        U64 pins = 0ULL;

        Bitloop(sE)
        {
            int sS = SquareOf(sE);

            U64 pinMask = PIN_MASKS[board.kMS][sS];
            U64 pin = pinMask & board.occB;

            if (Bitcount(pin) == 1) [[unlikely]] {
                U64 attacks = pinMask | SQUARE_BITS[sS];
                validAttacksMasks[depth][SquareOf(pin)] = attacks ^ pin;
                pins |= attacks;
            }
        }

        return pins;
    }

    template <int depth>
    ForceInline U64 findBishopPins(const BoardState& board) {
        return findPins<depth>((board.bE | board.qE) & BISHOP_XRAYS[board.kMS] & ~board.checks, board);
    }

    template <int depth>
    ForceInline U64 findRookPins(const BoardState& board) {
        return findPins<depth>((board.rE | board.qE) & ROOK_XRAYS[board.kMS] & ~board.checks, board);
    }

    template <int depth>
    ForceInline U64 findDiscoverers(const BoardState& board) {
        U64 disc = 0ULL;

        U64 sM = ((board.bM | board.qM) & BISHOP_XRAYS[board.kES]) | ((board.rM | board.qM) & ROOK_XRAYS[board.kES]);

        Bitloop(sM) {
            int sS = SquareOf(sM);

            U64 pinMask = PIN_MASKS[board.kES][sS];
            U64 pin = pinMask & board.occB;

            if (!BitReset(pin) && (pin & board.occM)) [[unlikely]] {
                U64 attacks = pinMask | SQUARE_BITS[sS];
                discoverMasks[depth][SquareOf(pin)] = attacks ^ pin;
                disc |= pin;
            }
        }

        return disc;
    }

    template <int depth>
    ForceInline U64 getDiscoverMask(int from, U64 disc) {
        if (disc) [[unlikely]] {
            return discoverMasks[depth][SquareOf((1ULL << from) & disc)];
        }
        return 0ULL;
    }

    template <int depth, bool side, uint8_t kMoved>
    struct PerftGenerator;

    template <int depth, bool side, uint8_t kMoved, Piece piece, bool capture>
    ForceInline void enumMoves(U64& nodes, U64 moves, int from, const BoardState& board, MoveArray& movesArray, U64 discoverMask) {
        Bitloop(moves) {
            int to = SquareOf(moves);
            const BoardState newBoard = board.make<piece, side, capture, kMoved>(from, to, board, discoverMask);
            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, capture, newBoard));
            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
        }
    }

    template <int depth, bool side, uint8_t kMoved, Piece piece>
    ForceInline void makeMoves(U64& nodes, U64 attacks, int from, const BoardState& board, MoveArray& movesArray, U64 discoverMask) {
        enumMoves<depth, side, kMoved, piece, false>(nodes, attacks & ~board.occE, from, board, movesArray, discoverMask);
        enumMoves<depth, side, kMoved, piece, true>(nodes, attacks & board.occE, from, board, movesArray, discoverMask);
    }

    template <int depth, bool side, uint8_t kMoved, bool capture>
    ForceInline void makePromotionMoves(U64& nodes, int from, int to, const BoardState& board, MoveArray& movesArray, U64 discoverMask) {
        const BoardState newBoardN = board.makePromotion<Piece::Knight, side, capture, kMoved>(from, to, board, discoverMask);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, n, capture, newBoardN));
        else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoardN, movesArray);

        const BoardState newBoardB = board.makePromotion<Piece::Bishop, side, capture, kMoved>(from, to, board, discoverMask);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, b, capture, newBoardB));
        else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoardB, movesArray);

        const BoardState newBoardR = board.makePromotion<Piece::Rook, side, capture, kMoved>(from, to, board, discoverMask);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, r, capture, newBoardR));
        else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoardR, movesArray);

        const BoardState newBoardQ = board.makePromotion<Piece::Queen, side, capture, kMoved>(from, to, board, discoverMask);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, q, capture, newBoardQ));
        else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoardQ, movesArray);
    }

    template <int depth, bool side, uint8_t kMoved>
    ForceInline U64 allMoves(const BoardState& board, MoveArray& movesArray) {
	    if constexpr (depth > 1) {
            tt::Entry& e = TT[depth][board.zobrist.low & MASK];
            if ((e.key ^ e.nodes) == board.zobrist.high) {
                return e.nodes;
            }
        }

        int from, to;
        U64 bitboard, attacks;

        constexpr bool kMMoved = kMoved & KING_MOVED[side];

        U64 nodes = 0ULL;
        U64 eAttacks = enemyAttacks<side, kMMoved>(board);

        U64 disc = 0ULL;
        if constexpr (depth != 1) disc = findDiscoverers<depth>(board);

        /*

            KING MOVES

        */
        attacks = board.kMA & ~board.occM & ~eAttacks;
        if constexpr (depth == 1) nodes += Bitcount(attacks);
        else makeMoves<depth, side, (kMoved | KING_MOVED[side]), Piece::King>(nodes, attacks, board.kMS, board, movesArray, getDiscoverMask<depth>(board.kMS, disc));

        if (board.checks) [[unlikely]] {
            if (!BitReset(board.checks)) [[likely]] {
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
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
                        }

                        Bitloop(promos) {
                            from = SquareOf(promos);
                            const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                            makePromotionMoves<depth, side, kMoved, true>(nodes, from, to, board, movesArray, discoverMask);
                        }

                        Bitloop(caps) {
                            from = SquareOf(caps);
                            const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                            const BoardState newBoard = board.make<Piece::Pawn, side, true, kMoved>(from, to, board, discoverMask);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
                        }
                    }

                    /*

                       KNIGHT MOVES

                    */
                    bitboard = board.nM & ~allPins;
                    attacks = getKnightAttacks(checkSquare) & bitboard;
                    if constexpr (depth == 1) nodes += Bitcount(attacks);
                    else {
                        Bitloop(attacks) {
                            from = SquareOf(attacks);
                            const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                            const BoardState newBoard = board.make<Piece::Knight, side, true, kMoved>(from, to, board, discoverMask);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
                        }
                    }

                    /*

                       BISHOP MOVES

                    */
                    bitboard = (board.bM | board.qM) & ~allPins & BISHOP_XRAYS[checkSquare];
                    if (bitboard) [[unlikely]] {
                        attacks = getBishopAttacks(checkSquare, board.occB) & bitboard;
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);
                                const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                                const BoardState newBoard =
                                    ((1ULL << from) & board.qM)
                                    ? board.make<Piece::Queen, side, true, kMoved>(from, to, board, discoverMask)
                                    : board.make<Piece::Bishop, side, true, kMoved>(from, to, board, discoverMask);

                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
                            }
                        }
                    }

                    /*

                       ROOK MOVES

                    */
                    bitboard = (board.rM | board.qM) & ~allPins & ROOK_XRAYS[checkSquare];
                    if (bitboard) [[unlikely]] {
                        attacks = getRookAttacks(checkSquare, board.occB) & bitboard;
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);
                                const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                                const BoardState newBoard =
                                    ((1ULL << from) & board.qM)
                                    ? board.make<Piece::Queen, side, true, kMoved>(from, to, board, discoverMask)
                                    : board.make<Piece::Rook, side, true, kMoved>(from, to, board, discoverMask);

                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
                            }
                        }
                    }
                }
                else {
                    const U64 validSquares = (board.checks | PIN_MASKS[board.kMS][checkSquare]);

                    /*

                       PAWN MOVES

                    */
                    const U64 pawns = board.pM & ~allPins;

                    U64 pawnsLeft = pawnsAtkLeft<side>(pawns) & board.occE & validSquares;
                    U64 pawnsRight = pawnsAtkRight<side>(pawns) & board.occE & validSquares;
                    U64 pawnsFwd = pawnsAtkForward<side>(pawns) & ~board.occB;
                    U64 pawnsDouble = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~board.occB & validSquares;
                    pawnsFwd &= validSquares;

                    if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANKS[side]) [[unlikely]] {
                        U64 promosLeft = pawnsLeft & LAST_RANKS[side];
                        U64 promosRight = pawnsRight & LAST_RANKS[side];
                        U64 promosFwd = pawnsFwd & LAST_RANKS[side];

                        pawnsLeft ^= promosLeft;
                        pawnsRight ^= promosRight;
                        pawnsFwd ^= promosFwd;

                        if constexpr (depth == 1) nodes += (Bitcount(promosLeft) + Bitcount(promosRight | promosFwd)) << 2;
                        else {
                            Bitloop(promosLeft) {
                                to = SquareOf(promosLeft);
                                from = to + PAWN_RIGHT[!side];
                                const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                                makePromotionMoves<depth, side, kMoved, true>(nodes, from, to, board, movesArray, discoverMask);
                            }
                            Bitloop(promosRight) {
                                to = SquareOf(promosRight);
                                from = to + PAWN_LEFT[!side];
                                const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                                makePromotionMoves<depth, side, kMoved, true>(nodes, from, to, board, movesArray, discoverMask);
                            }
                            Bitloop(promosFwd) {
                                to = SquareOf(promosFwd);
                                from = to + PAWN_PUSH[!side];
                                const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                                makePromotionMoves<depth, side, kMoved, false>(nodes, from, to, board, movesArray, discoverMask);
                            }
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
                            const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                            const BoardState newBoard = board.make<Piece::Pawn, side, true, kMoved>(from, to, board, discoverMask);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
                        }

                        Bitloop(pawnsRight) {
                            to = SquareOf(pawnsRight);
                            from = to + PAWN_LEFT[!side];
                            const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                            const BoardState newBoard = board.make<Piece::Pawn, side, true, kMoved>(from, to, board, discoverMask);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
                        }

                        Bitloop(pawnsFwd) {
                            to = SquareOf(pawnsFwd);
                            from = to + PAWN_PUSH[!side];
                            const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                            const BoardState newBoard = board.make<Piece::Pawn, side, false, kMoved>(from, to, board, discoverMask);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
                        }

                        Bitloop(pawnsDouble) {
                            to = SquareOf(pawnsDouble);
                            from = to + PAWN_DOUBLE_PUSH[!side];
                            const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                            const BoardState newBoard = board.makeDoublePush<side>(from, to, board, discoverMask);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
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
                        else makeMoves<depth, side, kMoved, Piece::Knight>(nodes, attacks, from, board, movesArray, getDiscoverMask<depth>(from, disc));
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
                        else makeMoves<depth, side, kMoved, Piece::Bishop>(nodes, attacks, from, board, movesArray, getDiscoverMask<depth>(from, disc));
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
                        else makeMoves<depth, side, kMoved, Piece::Rook>(nodes, attacks, from, board, movesArray, getDiscoverMask<depth>(from, disc));
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
                        else makeMoves<depth, side, kMoved, Piece::Queen>(nodes, attacks, from, board, movesArray, 0ULL);
                    }
                }
            }
			
			if constexpr (depth > 1) tt::write<depth>(board.zobrist, nodes);
			
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

        const U64 pawnsLeftAll = pawnsAtkLeft<side>(pawnsAtk & ~bPins) | (pawnsAtkLeft<side>(pawnsAtk & bPins) & bPins);
        U64 pawnsLeft = pawnsLeftAll & board.occE;
        const U64 pawnsRightAll = pawnsAtkRight<side>(pawnsAtk & ~bPins) | (pawnsAtkRight<side>(pawnsAtk & bPins) & bPins);
        U64 pawnsRight = pawnsRightAll & board.occE;
        U64 pawnsFwd = (pawnsAtkForward<side>(pawnsPush & ~rPins) & ~board.occB) | (pawnsAtkForward<side>(pawnsPush & rPins) & ~board.occB & rPins);
        U64 pawnsDouble = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~board.occB;

        if (board.eP != noSquare) [[unlikely]] {
            const U64 ePBit = (1ULL << board.eP);
            U64 ePP = pawnsAtkRight<!side>(pawnsLeftAll & ePBit) | pawnsAtkLeft<!side>(pawnsRightAll & ePBit);

            if (!passantPinned<side>(board, SquareOf(ePP))) [[likely]] {
                if constexpr (depth == 1) nodes += Bitcount(ePP);
                else {
                    Bitloop(ePP) {
                        from = SquareOf(ePP);

                        const BoardState newBoard = board.makeEnPassant<side>(from, board.eP, board);
                        if constexpr (depth == 0) movesArray.add(MoveInfo(EnPassant, from, board.eP, true, newBoard));
                        else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
                    }
                }
            }
        }

        if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANKS[side]) [[unlikely]] {
            U64 promosLeft = pawnsLeft & LAST_RANKS[side];
            U64 promosRight = pawnsRight & LAST_RANKS[side];
            U64 promosFwd = pawnsFwd & LAST_RANKS[side];

            pawnsLeft ^= promosLeft;
            pawnsRight ^= promosRight;
            pawnsFwd ^= promosFwd;

            if constexpr (depth == 1) nodes += (Bitcount(promosLeft) + Bitcount(promosRight | promosFwd)) << 2;
            else {
                Bitloop(promosLeft) {
                    to = SquareOf(promosLeft);
                    from = to + PAWN_RIGHT[!side];
                    const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                    makePromotionMoves<depth, side, kMoved, true>(nodes, from, to, board, movesArray, discoverMask);
                }
                Bitloop(promosRight) {
                    to = SquareOf(promosRight);
                    from = to + PAWN_LEFT[!side];
                    const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                    makePromotionMoves<depth, side, kMoved, true>(nodes, from, to, board, movesArray, discoverMask);
                }
                Bitloop(promosFwd) {
                    to = SquareOf(promosFwd);
                    from = to + PAWN_PUSH[!side];
                    const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                    makePromotionMoves<depth, side, kMoved, false>(nodes, from, to, board, movesArray, discoverMask);
                }
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
                const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                const BoardState newBoard = board.make<Piece::Pawn, side, true, kMoved>(from, to, board, discoverMask);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
            }

            Bitloop(pawnsRight) {
                to = SquareOf(pawnsRight);
                from = to + PAWN_LEFT[!side];
                const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                const BoardState newBoard = board.make<Piece::Pawn, side, true, kMoved>(from, to, board, discoverMask);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
            }

            Bitloop(pawnsFwd) {
                to = SquareOf(pawnsFwd);
                from = to + PAWN_PUSH[!side];
                const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                const BoardState newBoard = board.make<Piece::Pawn, side, false, kMoved>(from, to, board, discoverMask);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
            }

            Bitloop(pawnsDouble) {
                to = SquareOf(pawnsDouble);
                from = to + PAWN_DOUBLE_PUSH[!side];
                const U64 discoverMask = getDiscoverMask<depth>(from, disc);

                const BoardState newBoard = board.makeDoublePush<side>(from, to, board, discoverMask);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard, movesArray);
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
            else makeMoves<depth, side, kMoved, Piece::Knight>(nodes, attacks, from, board, movesArray, getDiscoverMask<depth>(from, disc));
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
            else makeMoves<depth, side, kMoved, Piece::Bishop>(nodes, attacks, from, board, movesArray, getDiscoverMask<depth>(from, disc));
        }

        bitboard = (board.bM | board.qM) & bPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = validAttacksMasks[depth][from];
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else {
                const U64 discoverMask = getDiscoverMask<depth>(from, disc);
                if ((1ULL << from) & board.qM)  makeMoves<depth, side, kMoved, Piece::Queen>(nodes, attacks, from, board, movesArray, discoverMask);
                else                            makeMoves<depth, side, kMoved, Piece::Bishop>(nodes, attacks, from, board, movesArray, discoverMask);
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
            else makeMoves<depth, side, kMoved, Piece::Rook>(nodes, attacks, from, board, movesArray, getDiscoverMask<depth>(from, disc));
        }

        bitboard = (board.rM | board.qM) & rPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = validAttacksMasks[depth][from];
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else {
                const U64 discoverMask = getDiscoverMask<depth>(from, disc);
                if ((1ULL << from) & board.qM)  makeMoves<depth, side, kMoved, Piece::Queen>(nodes, attacks, from, board, movesArray, discoverMask);
                else                            makeMoves<depth, side, kMoved, Piece::Rook>(nodes, attacks, from, board, movesArray, discoverMask);
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
            else makeMoves<depth, side, kMoved, Piece::Queen>(nodes, attacks, from, board, movesArray, 0ULL);
        }

        /*

            CASTLING

        */
        if constexpr (!kMMoved) {
            constexpr int kSide = CASTLING_SIDE_K[side];
            constexpr int qSide = CASTLING_SIDE_Q[side];
            if (castle<kSide>(board, eAttacks)) {
                if constexpr (depth == 1) nodes++;
                else {
                    const BoardState newBoard = board.makeCastling<kSide>(board);
                    if constexpr (depth == 0) movesArray.add(MoveInfo(Castle, CASTLING_KING_TARGET_SQUARE[kSide], false, newBoard));
                    else nodes += PerftGenerator<depth - 1, !side, (kMoved | KING_MOVED[side])>::generateMoves(newBoard, movesArray);
                }
            }
            if (castle<qSide>(board, eAttacks)) {
                if constexpr (depth == 1) nodes++;
                else {
                    const BoardState newBoard = board.makeCastling<qSide>(board);
                    if constexpr (depth == 0) movesArray.add(MoveInfo(Castle, CASTLING_KING_TARGET_SQUARE[qSide], false, newBoard));
                    else nodes += PerftGenerator<depth - 1, !side, (kMoved | KING_MOVED[side])>::generateMoves(newBoard, movesArray);
                }
            }
        }
		
		if constexpr (depth > 1) tt::write<depth>(board.zobrist, nodes);

        return nodes;
    }

    template <int depth, bool side, uint8_t kMoved>
    struct PerftGenerator {
        static __declspec(noinline) U64 generateMoves(const BoardState& board, MoveArray& movesArray) {
            return allMoves<depth, side, kMoved>(board, movesArray);
        }
    };

    template <bool side, uint8_t kMoved>
    struct PerftGenerator<1, side, kMoved> {
        ForceInline U64 generateMoves(const BoardState& board, MoveArray& movesArray) {
            return allMoves<1, side, kMoved>(board, movesArray);
        }
    };

    template <bool side, uint8_t kMoved>
    struct PerftGenerator<0, side, kMoved> {
        ForceInline U64 generateMoves(const BoardState& board, MoveArray& movesArray) {
            return allMoves<0, side, kMoved>(board, movesArray);
        }
    };

}

#endif