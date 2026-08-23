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

    ForceInline U64 findBishopPins(const BoardState& board) noexcept {
        return findPins((board.bE | board.qE) & BISHOP_XRAYS[board.kMS] & ~board.checks, board);
    }

    ForceInline U64 findRookPins(const BoardState& board) noexcept {
        return findPins((board.rE | board.qE) & ROOK_XRAYS[board.kMS] & ~board.checks, board);
    }

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

    template <bool side, uint8_t kMoved, Piece piece, bool capture>
    Inline void enumMoves(U64& nodes, U64 moves, int from, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        Bitloop(moves) {
            int to = SquareOf(moves);
 
            batch->add<side, capture>([&] { return board.make<piece, side, capture, kMoved>(from, to, board, discovers); });
        }
    }

    template <bool side, uint8_t kMoved, Piece piece, bool capsOnly>
    Inline void makeMoves(U64& nodes, U64 attacks, int from, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        if constexpr (!capsOnly)
            enumMoves<side, kMoved, piece, false>(nodes, attacks & ~board.occE, from, board, discovers, batch);
        enumMoves<side, kMoved, piece, true>(nodes, attacks & board.occE, from, board, discovers, batch);
    }

    template <bool side, uint8_t kMoved, bool capture, Piece piece>
    Inline void makeMove(U64& nodes, int from, int to, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        batch->add<side, capture>([&] { return board.make<piece, side, capture, kMoved>(from, to, board, discovers); });
    }

    template <bool side, uint8_t kMoved>
    Inline void makeEnPassant(U64& nodes, int from, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        batch->add<side, true>([&] { return board.makeEnPassant<side>(from, board.eP, board); });
    }

    template <bool side, uint8_t kMoved>
    Inline void makeDoublePush(U64& nodes, int from, int to, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        batch->add<side, false>([&] { return board.makeDoublePush<side>(from, to, board, discovers); });
    }

    template <bool side, uint8_t kMoved, bool capture>
    Inline void makePromotionMoves(U64& nodes, int from, int to, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        batch->add<side, capture, true>([&] { return board.makePromotion<Piece::Knight, side, capture, kMoved>(from, to, board, discovers); });
        batch->add<side, capture, true>([&] { return board.makePromotion<Piece::Bishop, side, capture, kMoved>(from, to, board, discovers); });
        batch->add<side, capture, true>([&] { return board.makePromotion<Piece::Rook, side, capture, kMoved>(from, to, board, discovers); });
        batch->add<side, capture, true>([&] { return board.makePromotion<Piece::Queen, side, capture, kMoved>(from, to, board, discovers); });
    }

    template <bool side, uint8_t kMoved, int castlingSide>
    Inline void makeCastling(U64& nodes, const BoardState& board, Batch* batch) noexcept {
        batch->add<side, false>([&] { return board.makeCastling<castlingSide>(board); });
    }

    template <bool count, bool side, uint8_t kMoved, bool capsOnly = false>
    Inline U64 generate(const BoardState& board, Batch* batch = nullptr) noexcept {
        int from, to;
        U64 bitboard, attacks;

        constexpr bool kMMoved = kMoved & KING_MOVED[side];

        U64 nodes = 0ULL;
        U64 eAttacks = enemyAttacks<side, kMMoved>(board);

        U64 discovers = 0ULL;
        if constexpr (!count) discovers = findDiscoverers(board);

        if (board.checks) [[unlikely]] {
            /*

                KING MOVES

            */
            U64 pseudoAttacks = board.kMA & ~board.occM;
            attacks = pseudoAttacks & ~eAttacks;
            if constexpr (capsOnly) attacks &= board.occE;

            if constexpr (count) nodes += Bitcount(attacks);
            else makeMoves<side, kMoved, Piece::King, capsOnly>(nodes, attacks, board.kMS, board, discovers, batch);

            if (!BitReset(board.checks)) [[likely]] {
                int checkSquare = SquareOf(board.checks);

                const U64 bPins = findBishopPins(board);
                const U64 rPins = findRookPins(board);
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
                            makeEnPassant<side, kMoved>(nodes, SquareOf(enPassant), board, discovers, batch);
                        }

                        Bitloop(promos) {
                            makePromotionMoves<side, kMoved, true>(nodes, SquareOf(promos), to, board, discovers, batch);
                        }

                        Bitloop(caps) {
                            makeMove<side, kMoved, true, Piece::Pawn>(nodes, SquareOf(caps), to, board, discovers, batch);
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
                            makeMove<side, kMoved, true, Piece::Knight>(nodes, SquareOf(attacks), to, board, discovers, batch);
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
                                    ? makeMove<side, kMoved, true, Piece::Queen>(nodes, from, to, board, discovers, batch)
                                    : makeMove<side, kMoved, true, Piece::Bishop>(nodes, from, to, board, discovers, batch);
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
                                    ? makeMove<side, kMoved, true, Piece::Queen>(nodes, from, to, board, discovers, batch)
                                    : makeMove<side, kMoved, true, Piece::Rook>(nodes, from, to, board, discovers, batch);
                            }
                        }
                    }
                }
                else {
                    U64 validSquares;
                    if constexpr (capsOnly) validSquares = board.checks;
                    else                    validSquares = (board.checks | PIN_MASKS[board.kMS][checkSquare]);

                    /*
                        PAWN MOVES
                    */
                    const U64 pawns = board.pM & ~allPins;

                    U64 pawnsLeft = pawnsAtkLeft<side>(pawns) & board.occE & validSquares;
                    U64 pawnsRight = pawnsAtkRight<side>(pawns) & board.occE & validSquares;
                    U64 pawnsFwd = 0ULL;
                    U64 pawnsDbl = 0ULL;
                    if constexpr (!capsOnly) {
                        pawnsFwd = pawnsAtkForward<side>(pawns) & ~board.occB;
                        pawnsDbl = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~board.occB & validSquares;
                        pawnsFwd &= validSquares;
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
                                makePromotionMoves<side, kMoved, true>(nodes, from, to, board, discovers, batch);
                            }

                            Bitloop(promosRight) {
                                to = SquareOf(promosRight);
                                from = to + PAWN_LEFT[!side];
                                makePromotionMoves<side, kMoved, true>(nodes, from, to, board, discovers, batch);
                            }

                            if constexpr (!capsOnly) {
                                Bitloop(promosFwd) {
                                    to = SquareOf(promosFwd);
                                    from = to + PAWN_PUSH[!side];
                                    makePromotionMoves<side, kMoved, false>(nodes, from, to, board, discovers, batch);
                                }
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
                            makeMove<side, kMoved, true, Piece::Pawn>(nodes, from, to, board, discovers, batch);
                        }

                        Bitloop(pawnsRight) {
                            to = SquareOf(pawnsRight);
                            from = to + PAWN_LEFT[!side];
                            makeMove<side, kMoved, true, Piece::Pawn>(nodes, from, to, board, discovers, batch);
                        }

                        if constexpr (!capsOnly) {
                            Bitloop(pawnsFwd) {
                                to = SquareOf(pawnsFwd);
                                from = to + PAWN_PUSH[!side];
                                makeMove<side, kMoved, false, Piece::Pawn>(nodes, from, to, board, discovers, batch);
                            }

                            Bitloop(pawnsDbl) {
                                to = SquareOf(pawnsDbl);
                                from = to + PAWN_DOUBLE_PUSH[!side];
                                makeDoublePush<side, kMoved>(nodes, from, to, board, discovers, batch);
                            }
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
                        else makeMoves<side, kMoved, Piece::Knight, capsOnly>(nodes, attacks, from, board, discovers, batch);
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
                        else makeMoves<side, kMoved, Piece::Bishop, capsOnly>(nodes, attacks, from, board, discovers, batch);
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
                        else makeMoves<side, kMoved, Piece::Rook, capsOnly>(nodes, attacks, from, board, discovers, batch);
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
                        else makeMoves<side, kMoved, Piece::Queen, capsOnly>(nodes, attacks, from, board, 0ULL, batch);
                    }
                }
            }

            return nodes;
        }

        const U64 bPins = findBishopPins(board);
        const U64 rPins = findRookPins(board);
        const U64 allPins = bPins | rPins;

        /*
            KING MOVES
        */
        U64 pseudoAttacks = board.kMA & ~board.occM;
        attacks = pseudoAttacks & ~eAttacks;
        if constexpr (capsOnly) attacks &= board.occE;

        if constexpr (count) nodes += Bitcount(attacks);
        else makeMoves<side, kMoved, Piece::King, capsOnly>(nodes, attacks, board.kMS, board, discovers, batch);

        /*
            PAWN MOVES
        */
        const U64 pawnsAtk = board.pM & ~rPins;
        const U64 pawnsPush = board.pM & ~bPins;

        const U64 pawnsLeftAll = pawnsAtkLeft<side>(pawnsAtk & ~bPins) | (pawnsAtkLeft<side>(pawnsAtk & bPins) & bPins);
        U64 pawnsLeft = pawnsLeftAll & board.occE;
        const U64 pawnsRightAll = pawnsAtkRight<side>(pawnsAtk & ~bPins) | (pawnsAtkRight<side>(pawnsAtk & bPins) & bPins);
        U64 pawnsRight = pawnsRightAll & board.occE;
        U64 pawnsFwd = 0ULL;
        U64 pawnsDbl = 0ULL;
        if constexpr (!capsOnly) {
            const U64 pawnsFwdAll = pawnsAtkForward<side>(pawnsPush & ~rPins) | (pawnsAtkForward<side>(pawnsPush & rPins) & rPins);
            pawnsFwd = pawnsFwdAll & ~board.occB;
            const U64 pawnsDblAll = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]);
            pawnsDbl = pawnsDblAll & ~board.occB;
        }

        if (board.eP != noSquare) [[unlikely]] {
            const U64 ePBit = (1ULL << board.eP);
            U64 ePP = pawnsAtkRight<!side>(pawnsLeftAll & ePBit) | pawnsAtkLeft<!side>(pawnsRightAll & ePBit);

            if (!passantPinned<side>(board, SquareOf(ePP))) [[likely]] {
                if constexpr (count) nodes += Bitcount(ePP);
                else {
                    Bitloop(ePP) {
                        makeEnPassant<side, kMoved>(nodes, SquareOf(ePP), board, discovers, batch);
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
                    makePromotionMoves<side, kMoved, true>(nodes, from, to, board, discovers, batch);
                }
                Bitloop(promosRight) {
                    to = SquareOf(promosRight);
                    from = to + PAWN_LEFT[!side];
                    makePromotionMoves<side, kMoved, true>(nodes, from, to, board, discovers, batch);
                }
                if constexpr (!capsOnly) {
                    Bitloop(promosFwd) {
                        to = SquareOf(promosFwd);
                        from = to + PAWN_PUSH[!side];
                        makePromotionMoves<side, kMoved, false>(nodes, from, to, board, discovers, batch);
                    }
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
                makeMove<side, kMoved, true, Piece::Pawn>(nodes, from, to, board, discovers, batch);
            }

            Bitloop(pawnsRight) {
                to = SquareOf(pawnsRight);
                from = to + PAWN_LEFT[!side];
                makeMove<side, kMoved, true, Piece::Pawn>(nodes, from, to, board, discovers, batch);
            }

            if constexpr (!capsOnly) {
                Bitloop(pawnsFwd) {
                    to = SquareOf(pawnsFwd);
                    from = to + PAWN_PUSH[!side];
                    makeMove<side, kMoved, false, Piece::Pawn>(nodes, from, to, board, discovers, batch);
                }

                Bitloop(pawnsDbl) {
                    to = SquareOf(pawnsDbl);
                    from = to + PAWN_DOUBLE_PUSH[!side];
                    makeDoublePush<side, kMoved>(nodes, from, to, board, discovers, batch);
                }
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
            if constexpr (capsOnly) attacks &= board.occE;

            if constexpr (count) nodes += Bitcount(attacks);
            else makeMoves<side, kMoved, Piece::Knight, capsOnly>(nodes, attacks, from, board, discovers, batch);
        }

        /*
            BISHOP MOVES
        */
        bitboard = board.bM & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getBishopAttacks(from, board.occB) & ~board.occM;
            if constexpr (capsOnly) attacks &= board.occE;

            if constexpr (count) nodes += Bitcount(attacks);
            else makeMoves<side, kMoved, Piece::Bishop, capsOnly>(nodes, attacks, from, board, discovers, batch);
        }

        bitboard = (board.bM | board.qM) & bPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = bPins & PIN_RAYS[board.kMS][from];
            if constexpr (capsOnly) attacks &= board.occE;

            if constexpr (count) nodes += Bitcount(attacks);
            else {
                const bool isQueen = ((1ULL << from) & board.qM) != 0ULL;

                if (isQueen)    makeMoves<side, kMoved, Piece::Queen, capsOnly>(nodes, attacks, from, board, 0ULL, batch);
                else            makeMoves<side, kMoved, Piece::Bishop, capsOnly>(nodes, attacks, from, board, discovers, batch);
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
            if constexpr (capsOnly) attacks &= board.occE;

            if constexpr (count) nodes += Bitcount(attacks);
            else makeMoves<side, kMoved, Piece::Rook, capsOnly>(nodes, attacks, from, board, discovers, batch);
        }

        bitboard = (board.rM | board.qM) & rPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = rPins & PIN_RAYS[board.kMS][from];
            if constexpr (capsOnly) attacks &= board.occE;

            if constexpr (count) nodes += Bitcount(attacks);
            else {
                const bool isQueen = ((1ULL << from) & board.qM) != 0ULL;

                if (isQueen)    makeMoves<side, kMoved, Piece::Queen, capsOnly>(nodes, attacks, from, board, 0ULL, batch);
                else            makeMoves<side, kMoved, Piece::Rook, capsOnly>(nodes, attacks, from, board, discovers, batch);
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
            if constexpr (capsOnly) attacks &= board.occE;

            if constexpr (count) nodes += Bitcount(attacks);
            else makeMoves<side, kMoved, Piece::Queen, capsOnly>(nodes, attacks, from, board, 0ULL, batch);
        }

        /*
            CASTLING
        */
        if constexpr (!kMMoved && !capsOnly) {
            constexpr int kSide = CASTLING_SIDE_K[side];
            constexpr int qSide = CASTLING_SIDE_Q[side];

            if (castle<kSide>(board, eAttacks)) {
                if constexpr (count) nodes++;
                else makeCastling<side, kMoved, kSide>(nodes, board, batch);
            }
            if (castle<qSide>(board, eAttacks)) {
                if constexpr (count) nodes++;
                else makeCastling<side, kMoved, qSide>(nodes, board, batch);
            }
        }

        return nodes;
    }
}

#endif