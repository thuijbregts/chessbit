#ifndef MOVEGENERATOR_H
#define MOVEGENERATOR_H

#include "Batch.h"
#include <vector>

using namespace bstate;
using namespace batch;

namespace movegen {

    /****************************************************
    * Naming conventions
    *
    * pM -> kM	|	pawn to king, current side
    * pE -> kE	|	pawn to king, opposite side
    * occM|E|B	|	occupancies (current, opposite, both)
    *
    *****************************************************/

    template <bool side, bool kMMoved>
    ForceInline U64 enemyAttacks(const BoardState& board) noexcept {
        U64 attacks = 0ULL;
        attacks |= pawnsAtkLeft<!side>(board.pE) | pawnsAtkRight<!side>(board.pE);
        attacks |= board.kEA;

        //remove king to avoid collisions, as it should not be considered when checking for threats
        U64 occB = board.occB ^ board.kM;

        U64 bitboard, nZone, bZone, rZone;

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

        return attacks;
    }

    template <int castlingSide>
    ForceInline bool castle(const BoardState& board, U64 attacks) noexcept {
        return !(!(board.casPerms & CASTLING[castlingSide]) | (CASTLING_OCCUPIED_SQUARES[castlingSide] & board.occB) | (attacks & CASTLING_PASSING_SQUARES[castlingSide]));
    }

    template <bool side>
    ForceInline bool passantPinned(const BoardState& board, int from) noexcept {
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

            if (!BitReset(pin)) [[unlikely]] {
                pins |= pinMask | SQUARE_BITS[sS];
            }
        }

        return pins;
    }

    template <int depth>
    ForceInline U64 findBishopPins(const BoardState& board) noexcept {
        return findPins<depth>((board.bE | board.qE) & BISHOP_XRAYS[board.kMS] & ~board.checks, board);
    }

    template <int depth>
    ForceInline U64 findRookPins(const BoardState& board) noexcept {
        return findPins<depth>((board.rE | board.qE) & ROOK_XRAYS[board.kMS] & ~board.checks, board);
    }

    template <int depth>
    ForceInline U64 findDiscoverers(const BoardState& board) noexcept {
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

    template <int depth, bool side, uint8_t kMoved, bool useTT, Piece piece, bool capture>
    ForceInline void enumMoves(U64& nodes, U64 moves, int from, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        Bitloop(moves) {
            int to = SquareOf(moves);
            const BoardState newBoard = board.make<piece, side, capture, kMoved, useTT>(from, to, board, discovers);
            
            if constexpr (piece == Piece::King) batch->add<depth, true, useTT>(newBoard);
            else                                batch->add<depth, false, useTT>(newBoard);
        }
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT, Piece piece>
    ForceInline void makeMoves(U64& nodes, U64 attacks, int from, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        enumMoves<depth, side, kMoved, useTT, piece, false>(nodes, attacks & ~board.occE, from, board, discovers, batch);
        enumMoves<depth, side, kMoved, useTT, piece, true>(nodes, attacks & board.occE, from, board, discovers, batch);
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT, bool capture, Piece piece>
    ForceInline void makeMove(U64& nodes, int from, int to, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        const BoardState newBoard = board.make<piece, side, capture, kMoved, useTT>(from, to, board, discovers);
        batch->add<depth, false, useTT>(newBoard);
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    ForceInline void makeEnPassant(U64& nodes, int from, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        const BoardState newBoard = board.makeEnPassant<side, useTT>(from, board.eP, board);
        batch->add<depth, false, useTT>(newBoard);
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    ForceInline void makeDoublePush(U64& nodes, int from, int to, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        const BoardState newBoard = board.makeDoublePush<side, useTT>(from, to, board, discovers);
        batch->add<depth, false, useTT>(newBoard);
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT, bool capture>
    ForceInline void makePromotionMoves(U64& nodes, int from, int to, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        const BoardState newBoardN = board.makePromotion<Piece::Knight, side, capture, kMoved, useTT>(from, to, board, discovers);
        batch->add<depth, false, useTT>(newBoardN);

        const BoardState newBoardB = board.makePromotion<Piece::Bishop, side, capture, kMoved, useTT>(from, to, board, discovers);
        batch->add<depth, false, useTT>(newBoardB);

        const BoardState newBoardR = board.makePromotion<Piece::Rook, side, capture, kMoved, useTT>(from, to, board, discovers);
        batch->add<depth, false, useTT>(newBoardR);

        const BoardState newBoardQ = board.makePromotion<Piece::Queen, side, capture, kMoved, useTT>(from, to, board, discovers);
        batch->add<depth, false, useTT>(newBoardQ);
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT, int castlingSide>
    ForceInline void makeCastling(U64& nodes, const BoardState& board, Batch* batch) noexcept {
        const BoardState newBoard = board.makeCastling<castlingSide, useTT>(board);
        batch->add<depth, true, useTT>(newBoard);
    }

    template <int depth, bool count, bool side, uint8_t kMoved, bool useTT>
    ForceInline U64 generate(const BoardState& board, Batch* batch = nullptr) noexcept {
        int from, to;
        U64 bitboard, attacks;

        constexpr bool kMMoved = kMoved & KING_MOVED[side];

        U64 nodes = 0ULL;
        U64 eAttacks = enemyAttacks<side, kMMoved>(board);

        U64 discovers = 0ULL;
        if constexpr (!count) discovers = findDiscoverers<depth>(board);

        if (board.checks) [[unlikely]] {
            /*

                KING MOVES

            */
            U64 pseudoAttacks = board.kMA & ~board.occM;
            attacks = pseudoAttacks & ~eAttacks;
            if constexpr (count) nodes += Bitcount(attacks);
            else makeMoves<depth, side, kMoved, useTT, Piece::King>(nodes, attacks, board.kMS, board, discovers, batch);

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

                    if constexpr (count) nodes += Bitcount(enPassant | caps) + (Bitcount(promos) << 2);
                    else {
                        Bitloop(enPassant)
                        {
                            makeEnPassant<depth, side, kMoved, useTT>(nodes, SquareOf(enPassant), board, discovers, batch);
                        }

                        Bitloop(promos) {
                            makePromotionMoves<depth, side, kMoved, useTT, true>(nodes, SquareOf(promos), to, board, discovers, batch);
                        }

                        Bitloop(caps) {
                            makeMove<depth, side, kMoved, useTT, true, Piece::Pawn>(nodes, SquareOf(caps), to, board, discovers, batch);
                        }
                    }

                    /*
                        KNIGHT MOVES
                    */
                    bitboard = board.nM & ~allPins;
                    attacks = getKnightAttacks(checkSquare) & bitboard;
                    if constexpr (count) nodes += Bitcount(attacks);
                    else {
                        Bitloop(attacks) {
                            makeMove<depth, side, kMoved, useTT, true, Piece::Knight>(nodes, SquareOf(attacks), to, board, discovers, batch);
                        }
                    }

                    /*
                        BISHOP MOVES
                    */
                    bitboard = (board.bM | board.qM) & ~allPins & BISHOP_XRAYS[checkSquare];
                    if (bitboard) [[unlikely]] {
                        attacks = getBishopAttacks(checkSquare, board.occB) & bitboard;
                        if constexpr (count) nodes += Bitcount(attacks);
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);

                                ((1ULL << from) & board.qM)
                                    ? makeMove<depth, side, kMoved, useTT, true, Piece::Queen>(nodes, from, to, board, discovers, batch)
                                    : makeMove<depth, side, kMoved, useTT, true, Piece::Bishop>(nodes, from, to, board, discovers, batch);
                            }
                        }
                    }

                    /*
                        ROOK MOVES
                    */
                    bitboard = (board.rM | board.qM) & ~allPins & ROOK_XRAYS[checkSquare];
                    if (bitboard) [[unlikely]] {
                        attacks = getRookAttacks(checkSquare, board.occB) & bitboard;
                        if constexpr (count) nodes += Bitcount(attacks);
                        else {
                            Bitloop(attacks) {
                                from = SquareOf(attacks);

                                ((1ULL << from) & board.qM)
                                    ? makeMove<depth, side, kMoved, useTT, true, Piece::Queen>(nodes, from, to, board, discovers, batch)
                                    : makeMove<depth, side, kMoved, useTT, true, Piece::Rook>(nodes, from, to, board, discovers, batch);
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

                    if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANK[side]) [[unlikely]] {
                        U64 promosLeft = pawnsLeft & LAST_RANK[side];
                        U64 promosRight = pawnsRight & LAST_RANK[side];
                        U64 promosFwd = pawnsFwd & LAST_RANK[side];


                        pawnsLeft ^= promosLeft;
                        pawnsRight ^= promosRight;
                        pawnsFwd ^= promosFwd;

                        if constexpr (count) nodes += (Bitcount(promosLeft) + Bitcount(promosRight | promosFwd)) << 2;
                        else {
                            Bitloop(promosLeft) {
                                to = SquareOf(promosLeft);
                                from = to + PAWN_RIGHT[!side];
                                makePromotionMoves<depth, side, kMoved, useTT, true>(nodes, from, to, board, discovers, batch);
                            }

                            Bitloop(promosRight) {
                                to = SquareOf(promosRight);
                                from = to + PAWN_LEFT[!side];
                                makePromotionMoves<depth, side, kMoved, useTT, true>(nodes, from, to, board, discovers, batch);
                            }

                            Bitloop(promosFwd) {
                                to = SquareOf(promosFwd);
                                from = to + PAWN_PUSH[!side];
                                makePromotionMoves<depth, side, kMoved, useTT, false>(nodes, from, to, board, discovers, batch);
                            }
                        }
                    }

                    if constexpr (count) {
                        nodes += Bitcount(pawnsLeft);
                        nodes += Bitcount(pawnsRight | pawnsFwd | pawnsDbl);
                    }
                    else {
                        Bitloop(pawnsLeft) {
                            to = SquareOf(pawnsLeft);
                            from = to + PAWN_RIGHT[!side];
                            makeMove<depth, side, kMoved, useTT, true, Piece::Pawn>(nodes, from, to, board, discovers, batch);
                        }

                        Bitloop(pawnsRight) {
                            to = SquareOf(pawnsRight);
                            from = to + PAWN_LEFT[!side];
                            makeMove<depth, side, kMoved, useTT, true, Piece::Pawn>(nodes, from, to, board, discovers, batch);
                        }

                        Bitloop(pawnsFwd) {
                            to = SquareOf(pawnsFwd);
                            from = to + PAWN_PUSH[!side];
                            makeMove<depth, side, kMoved, useTT, false, Piece::Pawn>(nodes, from, to, board, discovers, batch);
                        }

                        Bitloop(pawnsDbl) {
                            to = SquareOf(pawnsDbl);
                            from = to + PAWN_DOUBLE_PUSH[!side];
                            makeDoublePush<depth, side, kMoved, useTT>(nodes, from, to, board, discovers, batch);
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
                        if constexpr (count) nodes += Bitcount(attacks);
                        else makeMoves<depth, side, kMoved, useTT, Piece::Knight>(nodes, attacks, from, board, discovers, batch);
                    }

                    /*
                        BISHOP MOVES
                    */
                    bitboard = board.bM & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getBishopAttacks(from, board.occB) & validSquares;
                        if constexpr (count) nodes += Bitcount(attacks);
                        else makeMoves<depth, side, kMoved, useTT, Piece::Bishop>(nodes, attacks, from, board, discovers, batch);
                    }

                    /*
                        ROOK MOVES
                    */
                    bitboard = board.rM & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getRookAttacks(from, board.occB) & validSquares;
                        if constexpr (count) nodes += Bitcount(attacks);
                        else makeMoves<depth, side, kMoved, useTT, Piece::Rook>(nodes, attacks, from, board, discovers, batch);
                    }

                    /*
                        QUEEN MOVES
                    */
                    bitboard = board.qM & ~allPins;
                    Bitloop(bitboard)
                    {
                        from = SquareOf(bitboard);

                        attacks = getQueenAttacks(from, board.occB) & validSquares;
                        if constexpr (count) nodes += Bitcount(attacks);
                        else makeMoves<depth, side, kMoved, useTT, Piece::Queen>(nodes, attacks, from, board, 0ULL, batch);
                    }
                }
            }

            return nodes;
        }

        const U64 bPins = findBishopPins<depth>(board);
        const U64 rPins = findRookPins<depth>(board);
        const U64 allPins = bPins | rPins;

        /*
            KING MOVES
        */
        U64 pseudoAttacks = board.kMA & ~board.occM;
        attacks = pseudoAttacks & ~eAttacks;

        if constexpr (count) nodes += Bitcount(attacks);
        else makeMoves<depth, side, kMoved, useTT, Piece::King>(nodes, attacks, board.kMS, board, discovers, batch);

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

        if (board.eP != noSquare) [[unlikely]] {
            const U64 ePBit = (1ULL << board.eP);
            U64 ePP = pawnsAtkRight<!side>(pawnsLeftAll & ePBit) | pawnsAtkLeft<!side>(pawnsRightAll & ePBit);

            if (!passantPinned<side>(board, SquareOf(ePP))) [[likely]] {
                if constexpr (count) nodes += Bitcount(ePP);
                else {
                    Bitloop(ePP) {
                        makeEnPassant<depth, side, kMoved, useTT>(nodes, SquareOf(ePP), board, discovers, batch);
                    }
                }
            }
        }

        if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANK[side]) [[unlikely]] {
            U64 promosLeft = pawnsLeft & LAST_RANK[side];
            U64 promosRight = pawnsRight & LAST_RANK[side];
            U64 promosFwd = pawnsFwd & LAST_RANK[side];

            pawnsLeft ^= promosLeft;
            pawnsRight ^= promosRight;
            pawnsFwd ^= promosFwd;

            if constexpr (count) nodes += (Bitcount(promosLeft) + Bitcount(promosRight | promosFwd)) << 2;
            else {
                Bitloop(promosLeft) {
                    to = SquareOf(promosLeft);
                    from = to + PAWN_RIGHT[!side];
                    makePromotionMoves<depth, side, kMoved, useTT, true>(nodes, from, to, board, discovers, batch);
                }
                Bitloop(promosRight) {
                    to = SquareOf(promosRight);
                    from = to + PAWN_LEFT[!side];
                    makePromotionMoves<depth, side, kMoved, useTT, true>(nodes, from, to, board, discovers, batch);
                }
                Bitloop(promosFwd) {
                    to = SquareOf(promosFwd);
                    from = to + PAWN_PUSH[!side];
                    makePromotionMoves<depth, side, kMoved, useTT, false>(nodes, from, to, board, discovers, batch);
                }
            }
        }

        if constexpr (count) {
            nodes += Bitcount(pawnsLeft);
            nodes += Bitcount(pawnsRight | pawnsFwd | pawnsDbl);
        }
        else {
            Bitloop(pawnsLeft) {
                to = SquareOf(pawnsLeft);
                from = to + PAWN_RIGHT[!side];
                makeMove<depth, side, kMoved, useTT, true, Piece::Pawn>(nodes, from, to, board, discovers, batch);
            }

            Bitloop(pawnsRight) {
                to = SquareOf(pawnsRight);
                from = to + PAWN_LEFT[!side];
                makeMove<depth, side, kMoved, useTT, true, Piece::Pawn>(nodes, from, to, board, discovers, batch);
            }

            Bitloop(pawnsFwd) {
                to = SquareOf(pawnsFwd);
                from = to + PAWN_PUSH[!side];
                makeMove<depth, side, kMoved, useTT, false, Piece::Pawn>(nodes, from, to, board, discovers, batch);
            }

            Bitloop(pawnsDbl) {
                to = SquareOf(pawnsDbl);
                from = to + PAWN_DOUBLE_PUSH[!side];
                makeDoublePush<depth, side, kMoved, useTT>(nodes, from, to, board, discovers, batch);
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

            if constexpr (count) nodes += Bitcount(attacks);
            else makeMoves<depth, side, kMoved, useTT, Piece::Knight>(nodes, attacks, from, board, discovers, batch);
        }

        /*
            BISHOP MOVES
        */
        bitboard = board.bM & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getBishopAttacks(from, board.occB) & ~board.occM;

            if constexpr (count) nodes += Bitcount(attacks);
            else makeMoves<depth, side, kMoved, useTT, Piece::Bishop>(nodes, attacks, from, board, discovers, batch);
        }

        bitboard = (board.bM | board.qM) & bPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = bPins & PIN_RAYS[board.kMS][from];

            if constexpr (count) nodes += Bitcount(attacks);
            else {
                const bool isQueen = ((1ULL << from) & board.qM) != 0ULL;

                if (isQueen)    makeMoves<depth, side, kMoved, useTT, Piece::Queen>(nodes, attacks, from, board, 0ULL, batch);
                else            makeMoves<depth, side, kMoved, useTT, Piece::Bishop>(nodes, attacks, from, board, discovers, batch);
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

            if constexpr (count) nodes += Bitcount(attacks);
            else makeMoves<depth, side, kMoved, useTT, Piece::Rook>(nodes, attacks, from, board, discovers, batch);
        }

        bitboard = (board.rM | board.qM) & rPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = rPins & PIN_RAYS[board.kMS][from];

            if constexpr (count) nodes += Bitcount(attacks);
            else {
                const bool isQueen = ((1ULL << from) & board.qM) != 0ULL;

                if (isQueen)    makeMoves<depth, side, kMoved, useTT, Piece::Queen>(nodes, attacks, from, board, 0ULL, batch);
                else            makeMoves<depth, side, kMoved, useTT, Piece::Rook>(nodes, attacks, from, board, discovers, batch);
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

            if constexpr (count) nodes += Bitcount(attacks);
            else makeMoves<depth, side, kMoved, useTT, Piece::Queen>(nodes, attacks, from, board, 0ULL, batch);
        }

        /*
            CASTLING
        */
        if constexpr (!kMMoved) {
            constexpr int kSide = CASTLING_SIDE_K[side];
            constexpr int qSide = CASTLING_SIDE_Q[side];

            if (castle<kSide>(board, eAttacks)) {
                if constexpr (count) nodes++;
                else makeCastling<depth, side, kMoved, useTT, kSide>(nodes, board, batch);
            }
            if (castle<qSide>(board, eAttacks)) {
                if constexpr (count) nodes++;
                else makeCastling<depth, side, kMoved, useTT, qSide>(nodes, board, batch);
            }
        }

        return nodes;
    }
}

#endif