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

    struct NullEval {
        NullMaps maps{};
        U64 count = 0ULL;
        U64 quiet = 0ULL;

        U64 king = 0ULL;
        U64 pawn = 0ULL;
        U64 knight = 0ULL;
        U64 bishop = 0ULL;
        U64 rook = 0ULL;
        U64 queen = 0ULL;

        __forceinline constexpr void take(U64& attacks, int from, U64 map) {
            if ((1ULL << from) & map) return;

            const U64 q = attacks & ~map;
            attacks ^= q;
            quiet += Bitcount(q);
        }
    };

    template <bool side, bool kMMoved>
    ForceInline U64 enemyAttacks(const BoardState& board, NullMaps* maps) {
        U64 attacks = 0ULL;
        attacks |= pawnsAtkLeft<!side>(board.pE) | pawnsAtkRight<!side>(board.pE);
        attacks |= board.kEA;

        //remove king to avoid collisions, as it should not be considered when checking for threats
        U64 occB = board.occB ^ board.kM;

        int from;
        U64 bitboard, tmp, nZone, bZone, rZone;

        if (maps) {
            U64 cstlSq = 0ULL;
            bool addZone;

            if constexpr (!kMMoved) {
                constexpr U64 kSC = CASTLING_OCCUPIED_SQUARES[CASTLING_SIDE_K[side]];
                constexpr U64 qSC = CASTLING_OCCUPIED_SQUARES[CASTLING_SIDE_Q[side]];

                int casPerms = board.casPerms;

                if (board.occB & kSC)  casPerms &= ~CASTLING_BIT_K[side];
                if (board.occB & qSC)  casPerms &= ~CASTLING_BIT_Q[side];

                cstlSq = CASTLING_PASSING_SQUARES_NULL[side][casPerms];

                maps->nKZ = KNIGHT_ATTACK_ZONE_CASTLE[side];

            }
            else maps->nKZ = KNIGHT_ATTACK_ZONES[board.kMS];

            bitboard = board.nE & maps->nKZ;
            Bitloop(bitboard) {
                attacks |= getKnightAttacks(SquareOf(bitboard));
            }

            const U64 bqE = (board.bE | board.qE);
            const U64 rqE = (board.rE | board.qE);

            const U64 occBishop = occB & ~(board.occE & ~bqE);
            const U64 occRook = occB & ~(board.occE & ~rqE);

            U64 threatened = (board.kMA & ~board.occM) | cstlSq;

            bitboard = threatened & ~attacks;
            Bitloop(bitboard) {
                from = SquareOf(bitboard);
                const U64 fb = (1ULL << from);

                addZone = true;

                bZone = getBishopAttacks(from, occBishop);
                rZone = getRookAttacks(from, occRook);

                tmp = (bZone & bqE) | (rZone & rqE);
                Bitloop(tmp) {
                    int sq = SquareOf(tmp);
                    U64 threats = PIN_MASKS[sq][from];
                    U64 blockers = threats & board.occE;
                    if (!blockers) {
                        attacks |= fb;
                        maps->eMap |= threats | fb | (1ULL << sq);
                        addZone = false;
                    }
                    else if (!BitReset(blockers)) {
                        maps->eMap |= blockers;
                    }
                }
                if (addZone) {
                    maps->bKZ |= bZone;
                    maps->rKZ |= rZone;
                }
            }
        }
        else {
            if constexpr (!kMMoved) {
                nZone = KNIGHT_ATTACK_ZONE_CASTLE[side];
                bZone = getBishopAttackZoneCastle<side>(board.occM);
                rZone = getRookAttackZoneCastle<side>(board.occM);
            }
            else {
                nZone = KNIGHT_ATTACK_ZONES[board.kMS];
                bZone = getBishopAttackZone(board.kMS, board.occM, board.kMA);
                rZone = getRookAttackZone(board.kMS, board.occM, board.kMA);
            }

            bitboard = board.nE & nZone;
            Bitloop(bitboard) {
                attacks |= getKnightAttacks(SquareOf(bitboard));
            }

            bitboard = (board.bE | board.qE) & bZone;
            Bitloop(bitboard) {
                attacks |= getBishopAttacks(SquareOf(bitboard), occB);
            }

            bitboard = (board.rE | board.qE) & rZone;
            Bitloop(bitboard) {
                attacks |= getRookAttacks(SquareOf(bitboard), occB);
            }
        }

        return attacks;
    }

    template <int castlingSide>
    ForceInline bool castle(const BoardState& board, U64 attacks, NullMaps* maps = nullptr) {
        if (!(board.casPerms & CASTLING[castlingSide]) || (CASTLING_OCCUPIED_SQUARES[castlingSide] & board.occB)) {
            return false;
        }
        if (maps) {
            maps->cstlBit |= CASTLE_NULL_BIT[castlingSide];
        }
        return !(attacks & CASTLING_PASSING_SQUARES[castlingSide]);
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
    ForceInline U64 findPins(U64 sE, const BoardState& board, NullMaps* maps = nullptr) {
        U64 pins = 0ULL;

        Bitloop(sE)
        {
            int sS = SquareOf(sE);

            U64 pinMask = PIN_MASKS[board.kMS][sS];
            U64 pin = pinMask & board.occB;

            if (!BitReset(pin)) [[unlikely]] {
                pins |= pinMask | SQUARE_BITS[sS];
            }
            else {
                if (maps) {
                    if (Bitcount(pin) == 2 && Bitcount(pin & board.occE) == 1) [[unlikely]] {
                        maps->eMap |= pin & board.occE;
                    }
                }
            }
        }

        return pins;
    }

    template <int depth>
    ForceInline U64 findBishopPins(const BoardState& board, NullMaps* maps = nullptr) {
        return findPins<depth>((board.bE | board.qE) & BISHOP_XRAYS[board.kMS] & ~board.checks, board, maps);
    }

    template <int depth>
    ForceInline U64 findRookPins(const BoardState& board, NullMaps* maps = nullptr) {
        return findPins<depth>((board.rE | board.qE) & ROOK_XRAYS[board.kMS] & ~board.checks, board, maps);
    }

    template <int depth>
    ForceInline U64 findDiscoverers(const BoardState& board) {
        U64 disc = 0ULL;

        U64 sM = ((board.bM | board.qM) & BISHOP_XRAYS[board.kES]) | ((board.rM | board.qM) & ROOK_XRAYS[board.kES]);

        Bitloop(sM) {
            int sS = SquareOf(sM);

            U64 pinMask = PIN_MASKS[board.kES][sS];
            U64 pin = pinMask & board.occB;

            if (!BitReset(pin)) [[unlikely]] {
                disc |= SQUARE_BITS[sS];
            }
        }

        return disc;
    }

    template <int depth, bool side, uint8_t kMoved>
    struct PerftGenerator;

    template <int depth, bool side, uint8_t kMoved>
    ForceInline U64 allMoves(const BoardState& board, NullMaps* maps = nullptr);

    template <bool side, uint8_t kMoved>
    ForceInline NullEval buildNullEval(const BoardState& board, NullEval& eval) {
        NullMaps& m = eval.maps;

        const BoardState nullBoard = board.makeNull(board);
        eval.count = allMoves<1, !side, kMoved>(nullBoard, &m);

        m.eMap |= board.occE;
 
        const U64 bBlockers = getBishopAttacks(board.kES, board.occB) & board.occE;
        const U64 rBlockers = getRookAttacks(board.kES, board.occB) & board.occE;
        m.bPins = getBishopAttacks(board.kES, board.occB & ~bBlockers);
        m.rPins = getRookAttacks(board.kES, board.occB & ~rBlockers);

        eval.king = m.eMap | m.kKZ | m.cstlBit;
        eval.pawn = m.eMap | m.pKZ | m.cstlBit;
        eval.knight = m.eMap | m.nKZ | getKnightAttacks(board.kES);
        eval.bishop = m.eMap | m.bKZ | m.bPins;
        eval.rook = m.eMap | m.rKZ | m.rPins;
        eval.queen = eval.bishop | eval.rook;

        return eval;
    }

    template <int depth, bool side, uint8_t kMoved, Piece piece, bool capture>
    ForceInline void enumMoves(U64& nodes, U64 moves, int from, const BoardState& board, U64 discovers) {
        Bitloop(moves) {
            int to = SquareOf(moves);
            const BoardState newBoard = board.make<depth, piece, side, capture, kMoved>(from, to, board, discovers);
            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, capture, newBoard));
            else {
                if constexpr (piece == Piece::King) nodes += PerftGenerator<depth - 1, !side, (kMoved | KING_MOVED[side])>::generateMoves(newBoard);
                else                                nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);
            }
        }
    }

    template <int depth, bool side, uint8_t kMoved, Piece piece>
    ForceInline void makeMoves(U64& nodes, U64 attacks, int from, const BoardState& board, U64 discovers) {
        enumMoves<depth, side, kMoved, piece, false>(nodes, attacks & ~board.occE, from, board, discovers);
        enumMoves<depth, side, kMoved, piece, true>(nodes, attacks & board.occE, from, board, discovers);
    }

    template <int depth, bool side, uint8_t kMoved, bool capture>
    ForceInline void makePromotionMoves(U64& nodes, int from, int to, const BoardState& board, U64 discovers) {
        const BoardState newBoardN = board.makePromotion<depth, Piece::Knight, side, capture, kMoved>(from, to, board, discovers);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, n, capture, newBoardN));
        else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoardN);

        const BoardState newBoardB = board.makePromotion<depth, Piece::Bishop, side, capture, kMoved>(from, to, board, discovers);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, b, capture, newBoardB));
        else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoardB);

        const BoardState newBoardR = board.makePromotion<depth, Piece::Rook, side, capture, kMoved>(from, to, board, discovers);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, r, capture, newBoardR));
        else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoardR);

        const BoardState newBoardQ = board.makePromotion<depth, Piece::Queen, side, capture, kMoved>(from, to, board, discovers);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, q, capture, newBoardQ));
        else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoardQ);
    }

    template <int depth, bool side, uint8_t kMoved>
    ForceInline U64 allMoves(const BoardState& board, NullMaps* maps) {
        if constexpr (USE_HASH<depth>) {
            if (ttEnabled) {
                U64 cached;
                if (tt::probe<depth>(board.zobrist, cached)) {
                    return cached;
                }
            }
        }

        int from, to;
        U64 bitboard, attacks;

        constexpr bool kMMoved = kMoved & KING_MOVED[side];

        U64 nodes = 0ULL;
        U64 eAttacks = enemyAttacks<side, kMMoved>(board, maps);

        U64 discovers = 0ULL;
        if constexpr (depth != 1) discovers = findDiscoverers<depth>(board);

        if (board.checks) [[unlikely]] {
            /*

                KING MOVES

            */
            U64 pseudoAttacks = board.kMA & ~board.occM;
            attacks = pseudoAttacks & ~eAttacks;
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else makeMoves<depth, side, kMoved, Piece::King>(nodes, attacks, board.kMS, board, discovers);

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

                            const BoardState newBoard = board.makeEnPassant<depth, side>(from, board.eP, board);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(EnPassant, from, board.eP, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);
                        }

                        Bitloop(promos) {
                            from = SquareOf(promos);
                            const U64 discoverMask = discovers;

                            makePromotionMoves<depth, side, kMoved, true>(nodes, from, to, board, discoverMask);
                        }

                        Bitloop(caps) {
                            from = SquareOf(caps);
                            const U64 discoverMask = discovers;

                            const BoardState newBoard = board.make<depth, Piece::Pawn, side, true, kMoved>(from, to, board, discoverMask);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);

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
                            const U64 discoverMask = discovers;

                            const BoardState newBoard = board.make<depth, Piece::Knight, side, true, kMoved>(from, to, board, discoverMask);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);

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
                                const U64 discoverMask = discovers;

                                const BoardState newBoard =
                                    ((1ULL << from) & board.qM)
                                    ? board.make<depth, Piece::Queen, side, true, kMoved>(from, to, board, discoverMask)
                                    : board.make<depth, Piece::Bishop, side, true, kMoved>(from, to, board, discoverMask);

                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);

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
                                const U64 discoverMask = discovers;

                                const BoardState newBoard =
                                    ((1ULL << from) & board.qM)
                                    ? board.make<depth, Piece::Queen, side, true, kMoved>(from, to, board, discoverMask)
                                    : board.make<depth, Piece::Rook, side, true, kMoved>(from, to, board, discoverMask);

                                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                                else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);

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
                    U64 pawnsDbl = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~board.occB & validSquares;
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
                                const U64 discoverMask = discovers;

                                makePromotionMoves<depth, side, kMoved, true>(nodes, from, to, board, discoverMask);
                            }

                            Bitloop(promosRight) {
                                to = SquareOf(promosRight);
                                from = to + PAWN_LEFT[!side];
                                const U64 discoverMask = discovers;

                                makePromotionMoves<depth, side, kMoved, true>(nodes, from, to, board, discoverMask);
                            }

                            Bitloop(promosFwd) {
                                to = SquareOf(promosFwd);
                                from = to + PAWN_PUSH[!side];
                                const U64 discoverMask = discovers;

                                makePromotionMoves<depth, side, kMoved, false>(nodes, from, to, board, discoverMask);
                            }
                        }
                    }

                    if constexpr (depth == 1) {
                        nodes += Bitcount(pawnsLeft);
                        nodes += Bitcount(pawnsRight | pawnsFwd | pawnsDbl);
                    }
                    else {
                        Bitloop(pawnsLeft) {
                            to = SquareOf(pawnsLeft);
                            from = to + PAWN_RIGHT[!side];
                            const U64 discoverMask = discovers;

                            const BoardState newBoard = board.make<depth, Piece::Pawn, side, true, kMoved>(from, to, board, discoverMask);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);

                        }

                        Bitloop(pawnsRight) {

                            to = SquareOf(pawnsRight);
                            from = to + PAWN_LEFT[!side];
                            const U64 discoverMask = discovers;

                            const BoardState newBoard = board.make<depth, Piece::Pawn, side, true, kMoved>(from, to, board, discoverMask);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsFwd) {
                            to = SquareOf(pawnsFwd);
                            from = to + PAWN_PUSH[!side];
                            const U64 discoverMask = discovers;

                            const BoardState newBoard = board.make<depth, Piece::Pawn, side, false, kMoved>(from, to, board, discoverMask);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);
                        }

                        Bitloop(pawnsDbl) {

                            to = SquareOf(pawnsDbl);
                            from = to + PAWN_DOUBLE_PUSH[!side];
                            const U64 discoverMask = discovers;

                            const BoardState newBoard = board.makeDoublePush<depth, side>(from, to, board, discoverMask);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);
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
                        else makeMoves<depth, side, kMoved, Piece::Knight>(nodes, attacks, from, board, discovers);
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
                        else makeMoves<depth, side, kMoved, Piece::Bishop>(nodes, attacks, from, board, discovers);
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
                        else makeMoves<depth, side, kMoved, Piece::Rook>(nodes, attacks, from, board, discovers);
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
                        else makeMoves<depth, side, kMoved, Piece::Queen>(nodes, attacks, from, board, 0ULL);
                    }
                }
            }

            if constexpr (USE_HASH<depth>) {
                if (ttEnabled)  tt::write<depth>(board.zobrist, nodes);
            }

            return nodes;
        }

        const U64 bPins = findBishopPins<depth>(board, maps);
        const U64 rPins = findRookPins<depth>(board, maps);
        const U64 allPins = bPins | rPins;

        NullEval null;
        if constexpr (depth == 2) buildNullEval<side, kMoved>(board, null);

        /*
            KING MOVES
        */
        U64 pseudoAttacks = board.kMA & ~board.occM;
        attacks = pseudoAttacks & ~eAttacks;

        if constexpr (depth == 2) null.take(attacks, board.kMS, null.king);

        if constexpr (depth == 1) {
            nodes += Bitcount(attacks);
            if (maps) {
                maps->eMap |= attacks;
                maps->pKZ = pawnsAtkLeft<side>(pseudoAttacks | board.kM) | pawnsAtkRight<side>(pseudoAttacks | board.kM);
                maps->kKZ = KING_ZONES[board.kMS];
            }
        }
        else makeMoves<depth, side, kMoved, Piece::King>(nodes, attacks, board.kMS, board, discovers);

        /*
            PAWN MOVES
        */
        const U64 pawnsAtk = board.pM & ~rPins;
        const U64 pawnsPush = board.pM & ~bPins;

        const U64 pawnsLeftAll = pawnsAtkLeft<side>(pawnsAtk & ~bPins) | (pawnsAtkLeft<side>(pawnsAtk & bPins) & bPins);
        U64 pawnsLeft = pawnsLeftAll & board.occE;
        const U64 pawnsRightAll = pawnsAtkRight<side>(pawnsAtk & ~bPins) | (pawnsAtkRight<side>(pawnsAtk & bPins) & bPins);
        U64 pawnsRight = pawnsRightAll & board.occE;
        const U64 pawnsFwdAll = pawnsAtkForward<side>(pawnsPush & ~rPins) | (pawnsAtkForward<side>(pawnsPush & rPins) & rPins);
        U64 pawnsFwd = pawnsFwdAll & ~board.occB;
        const U64 pawnsDblAll = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]);
        U64 pawnsDbl = pawnsDblAll & ~board.occB;

        if (!maps) {
            if (board.eP != noSquare) [[unlikely]] {
                const U64 ePBit = (1ULL << board.eP);
                U64 ePP = pawnsAtkRight<!side>(pawnsLeftAll & ePBit) | pawnsAtkLeft<!side>(pawnsRightAll & ePBit);

                if (!passantPinned<side>(board, SquareOf(ePP))) [[likely]] {
                    if constexpr (depth == 1) nodes += Bitcount(ePP);
                    else {
                        Bitloop(ePP) {
                            from = SquareOf(ePP);

                            const BoardState newBoard = board.makeEnPassant<depth, side>(from, board.eP, board);
                            if constexpr (depth == 0) movesArray.add(MoveInfo(EnPassant, from, board.eP, true, newBoard));
                            else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);
                        }
                    }
                }
            }
        }
        else {
            maps->eMap |= allPins;

            maps->eMap |= (pawnsLeftAll | pawnsRightAll | pawnsFwdAll | pawnsDblAll);
            maps->ePCL = pawnsAtkRight<!side>(pawnsLeftAll) & EN_PASSANT_RANK[side];
            maps->ePCR = pawnsAtkLeft<!side>(pawnsRightAll) & EN_PASSANT_RANK[side];
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
                    const U64 discoverMask = discovers;

                    makePromotionMoves<depth, side, kMoved, true>(nodes, from, to, board, discoverMask);
                }
                Bitloop(promosRight) {
                    to = SquareOf(promosRight);
                    from = to + PAWN_LEFT[!side];
                    const U64 discoverMask = discovers;

                    makePromotionMoves<depth, side, kMoved, true>(nodes, from, to, board, discoverMask);
                }
                Bitloop(promosFwd) {
                    to = SquareOf(promosFwd);
                    from = to + PAWN_PUSH[!side];
                    const U64 discoverMask = discovers;

                    makePromotionMoves<depth, side, kMoved, false>(nodes, from, to, board, discoverMask);
                }
            }
        }

        if constexpr (depth == 2) {
            const U64 map = null.pawn;

            const U64 quietF = pawnsAtkForward<side>(pawnsAtkForward<!side>(pawnsFwd) & ~map) & ~map;
            const U64 quietD = pawnsAtkDouble<side>(pawnsAtkDouble<!side>(pawnsDbl) & ~map) & ~map;

            null.quiet += Bitcount(quietF | quietD);

            const U64 ePL = left(quietD) & null.maps.ePCR;
            const U64 ePR = right(quietD) & null.maps.ePCL;
            if (ePL | ePR) nodes += Bitcount(ePL) + Bitcount(ePR);

            pawnsFwd ^= quietF;
            pawnsDbl ^= quietD;
        }

        if constexpr (depth == 1) {
            nodes += Bitcount(pawnsLeft);
            nodes += Bitcount(pawnsRight | pawnsFwd | pawnsDbl);
        }
        else {
            Bitloop(pawnsLeft) {
                to = SquareOf(pawnsLeft);
                from = to + PAWN_RIGHT[!side];
                const U64 discoverMask = discovers;

                const BoardState newBoard = board.make<depth, Piece::Pawn, side, true, kMoved>(from, to, board, discoverMask);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsRight) {
                to = SquareOf(pawnsRight);
                from = to + PAWN_LEFT[!side];
                const U64 discoverMask = discovers;

                const BoardState newBoard = board.make<depth, Piece::Pawn, side, true, kMoved>(from, to, board, discoverMask);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, true, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsFwd) {
                to = SquareOf(pawnsFwd);
                from = to + PAWN_PUSH[!side];
                const U64 discoverMask = discovers;

                const BoardState newBoard = board.make<depth, Piece::Pawn, side, false, kMoved>(from, to, board, discoverMask);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);
            }

            Bitloop(pawnsDbl) {
                to = SquareOf(pawnsDbl);
                from = to + PAWN_DOUBLE_PUSH[!side];
                const U64 discoverMask = discovers;

                const BoardState newBoard = board.makeDoublePush<depth, side>(from, to, board, discoverMask);
                if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
                else nodes += PerftGenerator<depth - 1, !side, kMoved>::generateMoves(newBoard);
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

            if constexpr (depth == 2) null.take(attacks, from, null.knight);

            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else makeMoves<depth, side, kMoved, Piece::Knight>(nodes, attacks, from, board, discovers);
        }

        /*
            BISHOP MOVES
        */
        bitboard = board.bM & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getBishopAttacks(from, board.occB) & ~board.occM;

            if constexpr (depth == 2) null.take(attacks, from, null.bishop);

            if constexpr (depth == 1) {
                nodes += Bitcount(attacks);
                if (maps) maps->eMap |= attacks;
            }
            else makeMoves<depth, side, kMoved, Piece::Bishop>(nodes, attacks, from, board, discovers);
        }

        bitboard = (board.bM | board.qM) & bPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            const bool isQueen = ((1ULL << from) & board.qM) != 0ULL;
            attacks = bPins & PIN_RAYS[board.kMS][from];

            if constexpr (depth == 2) null.take(attacks, from, isQueen ? null.queen : null.bishop);

            if constexpr (depth == 1) {
                nodes += Bitcount(attacks);
                if (maps) maps->eMap |= attacks;
            }
            else {
                if (isQueen)    makeMoves<depth, side, kMoved, Piece::Queen>(nodes, attacks, from, board, 0ULL);
                else            makeMoves<depth, side, kMoved, Piece::Bishop>(nodes, attacks, from, board, discovers);
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

            if constexpr (depth == 2) null.take(attacks, from, null.rook);

            if constexpr (depth == 1) {
                nodes += Bitcount(attacks);
                if (maps) maps->eMap |= attacks;
            }
            else makeMoves<depth, side, kMoved, Piece::Rook>(nodes, attacks, from, board, discovers);
        }

        bitboard = (board.rM | board.qM) & rPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            const bool isQueen = ((1ULL << from) & board.qM) != 0ULL;
            attacks = rPins & PIN_RAYS[board.kMS][from];

            if constexpr (depth == 2) null.take(attacks, from, isQueen ? null.queen : null.rook);

            if constexpr (depth == 1) {
                nodes += Bitcount(attacks);
                if (maps) maps->eMap |= attacks;
            }
            else {
                if (isQueen)    makeMoves<depth, side, kMoved, Piece::Queen>(nodes, attacks, from, board, 0ULL);
                else            makeMoves<depth, side, kMoved, Piece::Rook>(nodes, attacks, from, board, discovers);
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

            if constexpr (depth == 2) null.take(attacks, from, null.queen);

            if constexpr (depth == 1) {
                nodes += Bitcount(attacks);
                if (maps) maps->eMap |= attacks;
            }
            else makeMoves<depth, side, kMoved, Piece::Queen>(nodes, attacks, from, board, 0ULL);
        }

        /*
            CASTLING
        */
        if constexpr (!kMMoved) {
            constexpr int kSide = CASTLING_SIDE_K[side];
            constexpr int qSide = CASTLING_SIDE_Q[side];
            if (castle<kSide>(board, eAttacks, maps)) {

                if constexpr (depth == 2) {
                    constexpr U64 rBit = rookSwitch<kSide>();
                    constexpr U64 kBit = kingSwitch<kSide>();

                    U64 hit = (kBit & null.maps.kKZ) | (rBit & null.rook);
                    if (!hit) {
                        null.quiet++;
                    }
                    else {
                        const BoardState newBoard = board.makeCastling<depth, kSide>(board);
                        nodes += PerftGenerator<depth - 1, !side, (kMoved | KING_MOVED[side])>::generateMoves(newBoard);
                    }
                }
                else {
                    if constexpr (depth == 1) nodes++;
                    else {
                        const BoardState newBoard = board.makeCastling<depth, kSide>(board);
                        if constexpr (depth == 0) movesArray.add(MoveInfo(KING_SOURCE_SQUARE[side], CASTLING_KING_TARGET_SQUARE[kSide], false, newBoard));
                        else nodes += PerftGenerator<depth - 1, !side, (kMoved | KING_MOVED[side])>::generateMoves(newBoard);
                    }
                }
            }
            if (castle<qSide>(board, eAttacks, maps)) {
                if constexpr (depth == 2) {
                    constexpr U64 rBit = rookSwitch<qSide>();
                    constexpr U64 kBit = kingSwitch<qSide>();

                    U64 hit = (kBit & null.maps.kKZ) | (rBit & null.rook);
                    if (!hit) {
                        null.quiet++;
                    }
                    else {
                        const BoardState newBoard = board.makeCastling<depth, qSide>(board);
                        nodes += PerftGenerator<depth - 1, !side, (kMoved | KING_MOVED[side])>::generateMoves(newBoard);
                    }
                }
                else {
                    if constexpr (depth == 1) nodes++;
                    else {
                        const BoardState newBoard = board.makeCastling<depth, qSide>(board);
                        if constexpr (depth == 0) movesArray.add(MoveInfo(KING_SOURCE_SQUARE[side], CASTLING_KING_TARGET_SQUARE[qSide], false, newBoard));
                        else nodes += PerftGenerator<depth - 1, !side, (kMoved | KING_MOVED[side])>::generateMoves(newBoard);
                    }
                }
            }
        }

        if constexpr (depth == 2) nodes += null.quiet * null.count;

        if constexpr (USE_HASH<depth>) {
            if (ttEnabled)  tt::write<depth>(board.zobrist, nodes);
        }

        return nodes;
    }

    template <int depth, bool side, uint8_t kMoved>
    struct PerftGenerator {
        static __declspec(noinline) U64 generateMoves(const BoardState& board) {
            return allMoves<depth, side, kMoved>(board);
        }
    };

    template <bool side, uint8_t kMoved>
    struct PerftGenerator<1, side, kMoved> {
        ForceInline U64 generateMoves(const BoardState& board) {
            return allMoves<1, side, kMoved>(board);
        }
    };

    template <bool side, uint8_t kMoved>
    struct PerftGenerator<0, side, kMoved> {
        ForceInline U64 generateMoves(const BoardState& board) {
            return allMoves<0, side, kMoved>(board);
        }
    };

}

#endif