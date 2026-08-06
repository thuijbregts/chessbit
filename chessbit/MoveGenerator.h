#ifndef MOVEGENERATOR_H
#define MOVEGENERATOR_H

#include "MoveArray.h"
#include "BoardState.h"
#include "MoveInfo.h"
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

    struct Batch {
        static constexpr int MAX = 256;
        static constexpr int MAX_KING = 8;
        BoardState normal[MAX];
        BoardState king[MAX_KING];
        int nSize = 0;
        int kSize = 0;

        template <int depth, bool k>
        __forceinline void add(const BoardState& b) noexcept {
            tt::prefetch<depth - 1>(b.zobrist);

            if constexpr (k)    king[kSize++] = b;
            else                normal[nSize++] = b;
        }
    };

    struct NullEval {
        NullMaps maps{};
        U64 count;
        U64 quiet = 0ULL;

        U64 king;
        U64 pawn;
        U64 knight;
        U64 bishop;
        U64 rook;
        U64 queen;

        template <bool side>
        __forceinline void take(U64& nodes, U64& attacks, int from, U64 map, NullMaps& null) noexcept {
            const U64 f = (1ULL << from);
            if (f & map) return;

            const U64 q = attacks & ~map;
            attacks ^= q;
            const U64 cnt = Bitcount(q);
            quiet += cnt;

            U64 coef = 0;
            if (f & null.pFwdFrom1) coef += 1;
            if (f & null.capOne)    coef -= 1;
            if (f & null.capTwo)    coef -= 2;
            if (f & LAST_RANKS)     coef <<= 2;
            nodes += coef * cnt;

            if (f & null.pFwdFrom2) {
                nodes += cnt << 1;
                if (pawnsAtkForward<!side>(f) & q) nodes--;
            }
            else if ((f & null.pFwdFromDbl) && (pawnsAtkForward<side>(f) & q)) nodes--;

            nodes += Bitcount(q & null.capOne) + 2 * Bitcount(q & null.capTwo) - Bitcount(q & null.pFwdTo1) - 2 * Bitcount(q & null.pFwdTo2);

            if (null.promoOn) [[unlikely]]
                nodes += 3 * Bitcount(q & null.capOneLR) + 6 * Bitcount(q & null.capTwoLR) - 3 * Bitcount(q & null.t1LR);
        }

        template <bool side>
        __forceinline void takeKing(U64& nodes, U64& attacks, U64 kM, U64 map, NullMaps& null) noexcept {
            if (kM & map) return;

            const U64 q = attacks & ~map;
            attacks ^= q;
            const U64 cnt = Bitcount(q);
            quiet += cnt;

            if (q & null.cstlBit) nodes--;

            nodes += NULL_KING_COEFF[SquareOf(kM & null.pFwdFrom1)] * cnt;

            if (kM & null.pFwdFrom2) {
                nodes += cnt << 1;
                if (pawnsAtkForward<!side>(kM) & q) nodes--;
            }
            else if ((kM & null.pFwdFromDbl) && (pawnsAtkForward<side>(kM) & q)) nodes--;

            nodes -= Bitcount(q & null.pFwdTo1) + 2 * Bitcount(q & null.pFwdTo2);
            if (null.promoOn) [[unlikely]]
                nodes -= 3 * Bitcount(q & null.t1LR);
        }

        template <bool side>
        __forceinline void takePawn(U64& nodes, U64& pawnsFwd, U64& pawnsDbl, U64 map, NullMaps& null) noexcept {
            const U64 quietF = pawnsAtkForward<side>(pawnsAtkForward<!side>(pawnsFwd) & ~map) & ~map;
            const U64 quietD = pawnsAtkDouble<side>(pawnsAtkDouble<!side>(pawnsDbl) & ~map) & ~map;
            const U64 quietB = quietF | quietD;

            const U64 pF = pawnsAtkForward<!side>(quietF);
            const U64 pD = pawnsAtkDouble<!side>(quietD);

            quiet += Bitcount(quietB);

            const U64 ePL = left(quietD) & null.ePCR;
            const U64 ePR = right(quietD) & null.ePCL;
            if (ePL | ePR) nodes += Bitcount(ePL) + Bitcount(ePR);
            if (quietF & null.cstlBit) nodes--;

            nodes -= Bitcount(pF & null.capOne) + Bitcount(pD & null.capOne);
            nodes += Bitcount(pF & null.pFwdFrom1nDbl);
            nodes += Bitcount(quietB & null.capOne);
            nodes -= Bitcount(quietB & null.pFwdTo1);

            if (null.capTwo) [[unlikely]] {
                nodes -= 2 * (Bitcount(pF & null.capTwo) + Bitcount(pD & null.capTwo));
                nodes += 2 * Bitcount(quietB & null.capTwo);
            }

            pawnsFwd ^= quietF;
            pawnsDbl ^= quietD;
        }

        template <bool side>
        __forceinline void takePawnPromo(U64& promoFwd) noexcept {
            const U64 guard = queen | knight;

            const U64 q = pawnsAtkForward<side>(pawnsAtkForward<!side>(promoFwd) & ~guard) & ~guard;
            quiet += Bitcount(q) << 2;

            promoFwd ^= q;
        }
    };

    template <bool side, bool kMMoved, bool nullMove>
    ForceInline U64 enemyAttacks(const BoardState& board, NullMaps* maps) noexcept {
        U64 attacks = 0ULL;
        attacks |= pawnsAtkLeft<!side>(board.pE) | pawnsAtkRight<!side>(board.pE);
        attacks |= board.kEA;

        //remove king to avoid collisions, as it should not be considered when checking for threats
        U64 occB = board.occB ^ board.kM;

        int from;
        U64 bitboard, tmp, nZone, bZone, rZone;

        if constexpr (nullMove) {
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

    template <int castlingSide, bool nullMove>
    ForceInline bool castle(const BoardState& board, U64 attacks, NullMaps* maps = nullptr) noexcept {
        if (!(board.casPerms & CASTLING[castlingSide]) || (CASTLING_OCCUPIED_SQUARES[castlingSide] & board.occB)) {
            return false;
        }
        constexpr bool side = CASTLING_SIDE[castlingSide];
        if (attacks & CASTLING_PASSING_SQUARES[castlingSide]) {
            if constexpr (castlingSide == CASTLING_SIDE_Q[side] && nullMove) {
                maps->eMap |= CASTLE_NULL_BIT[side] & board.kE;
            }
            return false;
        }
        if constexpr (castlingSide == CASTLING_SIDE_Q[side] && nullMove) {
            maps->cstlBit |= CASTLE_NULL_BIT[side];
        }
        return true;
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

    template <int depth, bool nullMove = false>
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
                if constexpr (nullMove) {
                    if (Bitcount(pin) == 2 && Bitcount(pin & board.occE) == 1) [[unlikely]] {
                        maps->eMap |= pin & board.occE;
                    }
                }
            }
        }

        return pins;
    }

    template <int depth, bool nullMove = false>
    ForceInline U64 findBishopPins(const BoardState& board, NullMaps* maps = nullptr) noexcept {
        return findPins<depth, nullMove>((board.bE | board.qE) & BISHOP_XRAYS[board.kMS] & ~board.checks, board, maps);
    }

    template <int depth, bool nullMove = false>
    ForceInline U64 findRookPins(const BoardState& board, NullMaps* maps = nullptr) noexcept {
        return findPins<depth, nullMove>((board.rE | board.qE) & ROOK_XRAYS[board.kMS] & ~board.checks, board, maps);
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

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    struct PerftGenerator;

    template <int depth, bool side, uint8_t kMoved, bool useTT, bool nullMove = false>
    ForceInline U64 allMoves(const BoardState& board, NullMaps* maps = nullptr) noexcept;

    template <bool side, uint8_t kMoved>
    ForceInline void buildNullEval(const BoardState& board, NullEval& eval) noexcept {
        NullMaps& m = eval.maps;

        const BoardState nullBoard = board.makeNull(board);
        eval.count = allMoves<1, !side, kMoved, false, true>(nullBoard, &m);

        m.eMap |= board.occE;

        m.capOne = m.pAtksL ^ m.pAtksR;
        m.capTwo = m.pAtksL & m.pAtksR;
        m.pFwdFrom1nDbl = m.pFwdFrom1 & ~m.pFwdFromDbl;

        m.capOneLR = m.capOne & LAST_RANKS;
        m.capTwoLR = m.capTwo & LAST_RANKS;
        m.t1LR = m.pFwdTo1 & LAST_RANKS;
        m.promoOn = (m.capOneLR | m.capTwoLR | m.t1LR) != 0ULL;

        const U64 bBlockers = getBishopAttacks(board.kES, board.occB) & board.occE;
        const U64 rBlockers = getRookAttacks(board.kES, board.occB) & board.occE;
        m.bPins = getBishopAttacks(board.kES, board.occB & ~bBlockers);
        m.rPins = getRookAttacks(board.kES, board.occB & ~rBlockers);

        eval.king = m.eMap | m.kKZ;
        eval.pawn = m.eMap | m.pKZ;
        eval.knight = m.eMap | m.nKZ | getKnightAttacks(board.kES);
        eval.bishop = m.eMap | m.bKZ | m.bPins;
        eval.rook = m.eMap | m.rKZ | m.rPins;
        eval.queen = eval.bishop | eval.rook;
    }

    template <int depth, bool side, uint8_t kMoved, Piece piece, bool capture, bool useTT>
    ForceInline void enumMoves(U64& nodes, U64 moves, int from, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        Bitloop(moves) {
            int to = SquareOf(moves);
            const BoardState newBoard = board.make<piece, side, capture, kMoved, (useTT && depth != 2)>(from, to, board, discovers);
            if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, capture, newBoard));
            else if constexpr (depth >= 3 && useTT) {
                if constexpr (piece == Piece::King) batch->add<depth, true>(newBoard);
                else                                batch->add<depth, false>(newBoard);
            }
            else {
                if constexpr (piece == Piece::King) nodes += PerftGenerator<depth - 1, !side, (kMoved | KING_MOVED[side]), useTT>::generateMoves(newBoard);
                else                                nodes += PerftGenerator<depth - 1, !side, kMoved, useTT>::generateMoves(newBoard);
            }
        }
    }

    template <int depth, bool side, uint8_t kMoved, Piece piece, bool useTT>
    ForceInline void makeMoves(U64& nodes, U64 attacks, int from, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        enumMoves<depth, side, kMoved, piece, false, useTT>(nodes, attacks & ~board.occE, from, board, discovers, batch);
        enumMoves<depth, side, kMoved, piece, true, useTT>(nodes, attacks & board.occE, from, board, discovers, batch);
    }

    template <int depth, bool side, uint8_t kMoved, bool capture, Piece piece, bool useTT>
    ForceInline void makeMove(U64& nodes, int from, int to, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        const BoardState newBoard = board.make<piece, side, capture, kMoved, (useTT && depth != 2)>(from, to, board, discovers);

        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, capture, newBoard));
        else if constexpr (depth >= 3 && useTT) batch->add<depth, false>(newBoard);
        else nodes += PerftGenerator<depth - 1, !side, kMoved, useTT>::generateMoves(newBoard);
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    ForceInline void makeEnPassant(U64& nodes, int from, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        const BoardState newBoard = board.makeEnPassant<side, (useTT && depth != 2)>(from, board.eP, board);

        if constexpr (depth == 0) movesArray.add(MoveInfo(EnPassant, from, board.eP, true, newBoard));
        else if constexpr (depth >= 3 && useTT) batch->add<depth, false>(newBoard);
        else nodes += PerftGenerator<depth - 1, !side, kMoved, useTT>::generateMoves(newBoard);
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    ForceInline void makeDoublePush(U64& nodes, int from, int to, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        const BoardState newBoard = board.makeDoublePush<side, (useTT && depth != 2)>(from, to, board, discovers);

        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, false, newBoard));
        else if constexpr (depth >= 3 && useTT) batch->add<depth, false>(newBoard);
        else nodes += PerftGenerator<depth - 1, !side, kMoved, useTT>::generateMoves(newBoard);
    }

    template <int depth, bool side, uint8_t kMoved, bool capture, bool useTT>
    ForceInline void makePromotionMoves(U64& nodes, int from, int to, const BoardState& board, U64 discovers, Batch* batch) noexcept {
        const BoardState newBoardN = board.makePromotion<Piece::Knight, side, capture, kMoved, (useTT && depth != 2)>(from, to, board, discovers);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, n, capture, newBoardN));
        else if constexpr (depth >= 3 && useTT) batch->add<depth, false>(newBoardN);
        else nodes += PerftGenerator<depth - 1, !side, kMoved, useTT>::generateMoves(newBoardN);

        const BoardState newBoardB = board.makePromotion<Piece::Bishop, side, capture, kMoved, (useTT && depth != 2)>(from, to, board, discovers);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, b, capture, newBoardB));
        else if constexpr (depth >= 3 && useTT) batch->add<depth, false>(newBoardB);
        else nodes += PerftGenerator<depth - 1, !side, kMoved, useTT>::generateMoves(newBoardB);

        const BoardState newBoardR = board.makePromotion<Piece::Rook, side, capture, kMoved, (useTT && depth != 2)>(from, to, board, discovers);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, r, capture, newBoardR));
        else if constexpr (depth >= 3 && useTT) batch->add<depth, false>(newBoardR);
        else nodes += PerftGenerator<depth - 1, !side, kMoved, useTT>::generateMoves(newBoardR);

        const BoardState newBoardQ = board.makePromotion<Piece::Queen, side, capture, kMoved, (useTT && depth != 2)>(from, to, board, discovers);
        if constexpr (depth == 0) movesArray.add(MoveInfo(from, to, q, capture, newBoardQ));
        else if constexpr (depth >= 3 && useTT) batch->add<depth, false>(newBoardQ);
        else nodes += PerftGenerator<depth - 1, !side, kMoved, useTT>::generateMoves(newBoardQ);
    }

    template <int depth, bool side, uint8_t kMoved, int castlingSide, bool useTT>
    ForceInline void makeCastling(U64& nodes, const BoardState& board, Batch* batch, NullEval& null, NullMaps* maps) noexcept {
        if constexpr (depth == 2) {
            constexpr U64 rBit = rookSwitch<castlingSide>();
            constexpr U64 kBit = kingSwitch<castlingSide>();

            U64 hit = (kBit & null.maps.kKZ) | (rBit & null.rook);
            if (!hit) {
                null.quiet++;
                if constexpr (castlingSide == CASTLING_SIDE_Q[side]) {
                    if (board.pE & CASTLING_PROMO_BIT[side]) [[unlikely]] nodes += 4;
                }
            }
            else {
                const BoardState newBoard = board.makeCastling<castlingSide, (useTT && depth != 2)>(board);
                nodes += PerftGenerator<depth - 1, !side, (kMoved | KING_MOVED[side]), useTT>::generateMoves(newBoard);
            }
        }
        else if constexpr (depth == 1) nodes++;
        else {
            const BoardState newBoard = board.makeCastling<castlingSide, (useTT && depth != 2)>(board);

            if constexpr (depth == 0) movesArray.add(MoveInfo(KING_SOURCE_SQUARE[side], CASTLING_KING_TARGET_SQUARE[castlingSide], false, newBoard));
            else if constexpr (depth >= 3 && useTT) batch->add<depth, true>(newBoard);
            else nodes += PerftGenerator<depth - 1, !side, (kMoved | KING_MOVED[side]), useTT>::generateMoves(newBoard);
        }
    }

    template <int depth, bool side, uint8_t kMoved>
    ForceInline void iterateBatch(U64& nodes, Batch* batch) noexcept {
        constexpr uint8_t kMovedK = kMoved | KING_MOVED[side];
        constexpr int nDepth = depth - 1;

        U64 val;
        for (int i = 0; i < batch->nSize; ++i) {
            const Zobrist& z = batch->normal[i].zobrist;
            Bucket& b = tt::bucket<nDepth>(z);

            if (tt::probe<nDepth>(b, z, val)) nodes += val;
            else {
                val = PerftGenerator<nDepth, !side, kMoved, true>::generateMoves(batch->normal[i]);
                tt::write<nDepth>(b, z, val);
                nodes += val;
            }
        }

        for (int i = 0; i < batch->kSize; ++i) {
            const Zobrist& z = batch->king[i].zobrist;
            Bucket& b = tt::bucket<nDepth>(z);

            if (tt::probe<nDepth>(b, z, val)) nodes += val;
            else {
                val = PerftGenerator<nDepth, !side, kMovedK, true>::generateMoves(batch->king[i]);
                tt::write<nDepth>(b, z, val);
                nodes += val;
            }
        }
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT, bool nullMove>
    ForceInline U64 allMoves(const BoardState& board, NullMaps* maps) noexcept {
        int from, to;
        U64 bitboard, attacks;

        constexpr bool kMMoved = kMoved & KING_MOVED[side];

        U64 nodes = 0ULL;
        U64 eAttacks = enemyAttacks<side, kMMoved, nullMove>(board, maps);

        U64 discovers = 0ULL;
        if constexpr (depth != 1) discovers = findDiscoverers<depth>(board);

        Batch batchStorage;
        Batch* batch = nullptr;
        if constexpr (depth >= 3 && useTT) batch = &batchStorage;

        if (board.checks) [[unlikely]] {
            /*

                KING MOVES

            */
            U64 pseudoAttacks = board.kMA & ~board.occM;
            attacks = pseudoAttacks & ~eAttacks;
            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else makeMoves<depth, side, kMoved, Piece::King, useTT>(nodes, attacks, board.kMS, board, discovers, batch);

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
                            makeEnPassant<depth, side, kMoved, useTT>(nodes, SquareOf(enPassant), board, discovers, batch);
                        }

                        Bitloop(promos) {
                            makePromotionMoves<depth, side, kMoved, true, useTT>(nodes, SquareOf(promos), to, board, discovers, batch);
                        }

                        Bitloop(caps) {
                            makeMove<depth, side, kMoved, true, Piece::Pawn, useTT>(nodes, SquareOf(caps), to, board, discovers, batch);
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
                            makeMove<depth, side, kMoved, true, Piece::Knight, useTT>(nodes, SquareOf(attacks), to, board, discovers, batch);
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

                                ((1ULL << from) & board.qM)
                                    ? makeMove<depth, side, kMoved, true, Piece::Queen, useTT>(nodes, from, to, board, discovers, batch)
                                    : makeMove<depth, side, kMoved, true, Piece::Bishop, useTT>(nodes, from, to, board, discovers, batch);
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

                                ((1ULL << from) & board.qM)
                                    ? makeMove<depth, side, kMoved, true, Piece::Queen, useTT>(nodes, from, to, board, discovers, batch)
                                    : makeMove<depth, side, kMoved, true, Piece::Rook, useTT>(nodes, from, to, board, discovers, batch);
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

                        if constexpr (depth == 1) nodes += (Bitcount(promosLeft) + Bitcount(promosRight | promosFwd)) << 2;
                        else {
                            Bitloop(promosLeft) {
                                to = SquareOf(promosLeft);
                                from = to + PAWN_RIGHT[!side];
                                makePromotionMoves<depth, side, kMoved, true, useTT>(nodes, from, to, board, discovers, batch);
                            }

                            Bitloop(promosRight) {
                                to = SquareOf(promosRight);
                                from = to + PAWN_LEFT[!side];
                                makePromotionMoves<depth, side, kMoved, true, useTT>(nodes, from, to, board, discovers, batch);
                            }

                            Bitloop(promosFwd) {
                                to = SquareOf(promosFwd);
                                from = to + PAWN_PUSH[!side];
                                makePromotionMoves<depth, side, kMoved, false, useTT>(nodes, from, to, board, discovers, batch);
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
                            makeMove<depth, side, kMoved, true, Piece::Pawn, useTT>(nodes, from, to, board, discovers, batch);
                        }

                        Bitloop(pawnsRight) {
                            to = SquareOf(pawnsRight);
                            from = to + PAWN_LEFT[!side];
                            makeMove<depth, side, kMoved, true, Piece::Pawn, useTT>(nodes, from, to, board, discovers, batch);
                        }

                        Bitloop(pawnsFwd) {
                            to = SquareOf(pawnsFwd);
                            from = to + PAWN_PUSH[!side];
                            makeMove<depth, side, kMoved, false, Piece::Pawn, useTT>(nodes, from, to, board, discovers, batch);
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
                        if constexpr (depth == 1) nodes += Bitcount(attacks);
                        else makeMoves<depth, side, kMoved, Piece::Knight, useTT>(nodes, attacks, from, board, discovers, batch);
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
                        else makeMoves<depth, side, kMoved, Piece::Bishop, useTT>(nodes, attacks, from, board, discovers, batch);
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
                        else makeMoves<depth, side, kMoved, Piece::Rook, useTT>(nodes, attacks, from, board, discovers, batch);
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
                        else makeMoves<depth, side, kMoved, Piece::Queen, useTT>(nodes, attacks, from, board, 0ULL, batch);
                    }
                }
            }

            if constexpr (depth >= 3 && useTT) iterateBatch<depth, side, kMoved>(nodes, batch);

            return nodes;
        }

        const U64 bPins = findBishopPins<depth, nullMove>(board, maps);
        const U64 rPins = findRookPins<depth, nullMove>(board, maps);
        const U64 allPins = bPins | rPins;

        if constexpr (nullMove) maps->eMap |= allPins;

        NullEval null;
        if constexpr (depth == 2) buildNullEval<side, kMoved>(board, null);

        /*
            KING MOVES
        */
        U64 pseudoAttacks = board.kMA & ~board.occM;
        attacks = pseudoAttacks & ~eAttacks;

        if constexpr (depth == 2) null.takeKing<side>(nodes, attacks, board.kM, null.king, null.maps);

        if constexpr (depth == 1) {
            nodes += Bitcount(attacks);
            if constexpr (nullMove) {
                maps->eMap |= attacks;
                maps->pKZ = pawnsAtkLeft<side>(pseudoAttacks | board.kM) | pawnsAtkRight<side>(pseudoAttacks | board.kM);
                maps->kKZ = KING_ZONES[board.kMS];
            }
        }
        else makeMoves<depth, side, kMoved, Piece::King, useTT>(nodes, attacks, board.kMS, board, discovers, batch);

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

        if constexpr (!nullMove) {
            if (board.eP != noSquare) [[unlikely]] {
                const U64 ePBit = (1ULL << board.eP);
                U64 ePP = pawnsAtkRight<!side>(pawnsLeftAll & ePBit) | pawnsAtkLeft<!side>(pawnsRightAll & ePBit);

                if (!passantPinned<side>(board, SquareOf(ePP))) [[likely]] {
                    if constexpr (depth == 1) nodes += Bitcount(ePP);
                    else {
                        Bitloop(ePP) {
                            makeEnPassant<depth, side, kMoved, useTT>(nodes, SquareOf(ePP), board, discovers, batch);
                        }
                    }
                }
            }
        }
        else {
            const U64 pawnsDblFrom = pawnsAtkForward<side>(pawnsFwdAll & FIRST_PUSH_RANK[side]) & ~board.occB;

            maps->pAtksL = pawnsLeftAll;
            maps->pAtksR = pawnsRightAll;
            maps->pFwdFrom2 = (pawnsFwdAll & pawnsAtkForward<!side>(pawnsDblFrom));
            maps->pFwdFrom1 = (pawnsFwdAll | pawnsDblAll) & ~maps->pFwdFrom2;
            maps->pFwdFromDbl = pawnsDblAll;
            maps->pFwdTo2 = (pawnsFwd & pawnsAtkForward<!side>(pawnsDbl));
            maps->pFwdTo1 = (pawnsFwd | pawnsDbl) & ~maps->pFwdTo2;
            maps->ePCL = pawnsAtkRight<!side>(pawnsLeftAll) & EN_PASSANT_RANK[side];
            maps->ePCR = pawnsAtkLeft<!side>(pawnsRightAll) & EN_PASSANT_RANK[side];
        }

        if ((pawnsLeft | pawnsRight | pawnsFwd) & LAST_RANK[side]) [[unlikely]] {
            U64 promosLeft = pawnsLeft & LAST_RANK[side];
            U64 promosRight = pawnsRight & LAST_RANK[side];
            U64 promosFwd = pawnsFwd & LAST_RANK[side];

            pawnsLeft ^= promosLeft;
            pawnsRight ^= promosRight;
            pawnsFwd ^= promosFwd;

            if constexpr (depth == 1) nodes += (Bitcount(promosLeft) + Bitcount(promosRight | promosFwd)) << 2;
            else {
                if constexpr (depth == 2) null.takePawnPromo<side>(promosFwd);

                Bitloop(promosLeft) {
                    to = SquareOf(promosLeft);
                    from = to + PAWN_RIGHT[!side];
                    makePromotionMoves<depth, side, kMoved, true, useTT>(nodes, from, to, board, discovers, batch);
                }
                Bitloop(promosRight) {
                    to = SquareOf(promosRight);
                    from = to + PAWN_LEFT[!side];
                    makePromotionMoves<depth, side, kMoved, true, useTT>(nodes, from, to, board, discovers, batch);
                }
                Bitloop(promosFwd) {
                    to = SquareOf(promosFwd);
                    from = to + PAWN_PUSH[!side];
                    makePromotionMoves<depth, side, kMoved, false, useTT>(nodes, from, to, board, discovers, batch);
                }
            }
        }

        if constexpr (depth == 1) {
            nodes += Bitcount(pawnsLeft);
            nodes += Bitcount(pawnsRight | pawnsFwd | pawnsDbl);
        }
        else {
            if constexpr (depth == 2) null.takePawn<side>(nodes, pawnsFwd, pawnsDbl, null.pawn, null.maps);

            Bitloop(pawnsLeft) {
                to = SquareOf(pawnsLeft);
                from = to + PAWN_RIGHT[!side];
                makeMove<depth, side, kMoved, true, Piece::Pawn, useTT>(nodes, from, to, board, discovers, batch);
            }

            Bitloop(pawnsRight) {
                to = SquareOf(pawnsRight);
                from = to + PAWN_LEFT[!side];
                makeMove<depth, side, kMoved, true, Piece::Pawn, useTT>(nodes, from, to, board, discovers, batch);
            }

            Bitloop(pawnsFwd) {
                to = SquareOf(pawnsFwd);
                from = to + PAWN_PUSH[!side];
                makeMove<depth, side, kMoved, false, Piece::Pawn, useTT>(nodes, from, to, board, discovers, batch);
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

            if constexpr (depth == 2) null.take<side>(nodes, attacks, from, null.knight, null.maps);

            if constexpr (depth == 1) nodes += Bitcount(attacks);
            else makeMoves<depth, side, kMoved, Piece::Knight, useTT>(nodes, attacks, from, board, discovers, batch);
        }

        /*
            BISHOP MOVES
        */
        bitboard = board.bM & ~allPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = getBishopAttacks(from, board.occB) & ~board.occM;

            if constexpr (depth == 2) null.take<side>(nodes, attacks, from, null.bishop, null.maps);

            if constexpr (depth == 1) {
                nodes += Bitcount(attacks);
                if constexpr (nullMove) maps->eMap |= attacks & NO_EDGES;
            }
            else makeMoves<depth, side, kMoved, Piece::Bishop, useTT>(nodes, attacks, from, board, discovers, batch);
        }

        bitboard = (board.bM | board.qM) & bPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            attacks = bPins & PIN_RAYS[board.kMS][from];

            if ((1ULL << from) & board.qM) {
                if constexpr (depth == 2) null.take<side>(nodes, attacks, from, null.queen, null.maps);

                if constexpr (depth == 1) {
                    nodes += Bitcount(attacks);
                    if constexpr (nullMove) maps->eMap |= attacks & NO_EDGES_ROOK[from];
                }
                else makeMoves<depth, side, kMoved, Piece::Queen, useTT>(nodes, attacks, from, board, 0ULL, batch);
            }
            else {
                if constexpr (depth == 2) null.take<side>(nodes, attacks, from, null.bishop, null.maps);

                if constexpr (depth == 1) {
                    nodes += Bitcount(attacks);
                    if constexpr (nullMove) maps->eMap |= attacks & NO_EDGES;
                }
                else makeMoves<depth, side, kMoved, Piece::Bishop, useTT>(nodes, attacks, from, board, discovers, batch);
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

            if constexpr (depth == 2) null.take<side>(nodes, attacks, from, null.rook, null.maps);

            if constexpr (depth == 1) {
                nodes += Bitcount(attacks);
                if constexpr (nullMove) maps->eMap |= attacks & NO_EDGES_ROOK[from];
            }
            else makeMoves<depth, side, kMoved, Piece::Rook, useTT>(nodes, attacks, from, board, discovers, batch);
        }

        bitboard = (board.rM | board.qM) & rPins;
        Bitloop(bitboard)
        {
            from = SquareOf(bitboard);

            const bool isQueen = ((1ULL << from) & board.qM) != 0ULL;
            attacks = rPins & PIN_RAYS[board.kMS][from];

            if constexpr (depth == 2) null.take<side>(nodes, attacks, from, isQueen ? null.queen : null.rook, null.maps);

            if constexpr (depth == 1) {
                nodes += Bitcount(attacks);
                if constexpr (nullMove) maps->eMap |= attacks & NO_EDGES_ROOK[from];
            }
            else {
                if (isQueen)    makeMoves<depth, side, kMoved, Piece::Queen, useTT>(nodes, attacks, from, board, 0ULL, batch);
                else            makeMoves<depth, side, kMoved, Piece::Rook, useTT>(nodes, attacks, from, board, discovers, batch);
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

            if constexpr (depth == 2) null.take<side>(nodes, attacks, from, null.queen, null.maps);

            if constexpr (depth == 1) {
                nodes += Bitcount(attacks);
                if constexpr (nullMove) maps->eMap |= attacks & NO_EDGES_ROOK[from];
            }
            else makeMoves<depth, side, kMoved, Piece::Queen, useTT>(nodes, attacks, from, board, 0ULL, batch);
        }

        /*
            CASTLING
        */
        if constexpr (!kMMoved) {
            constexpr int kSide = CASTLING_SIDE_K[side];
            constexpr int qSide = CASTLING_SIDE_Q[side];

            if (castle<kSide, nullMove>(board, eAttacks, maps)) {
                makeCastling<depth, side, kMoved, kSide, useTT>(nodes, board, batch, null, maps);
            }
            if (castle<qSide, nullMove>(board, eAttacks, maps)) {
                makeCastling<depth, side, kMoved, qSide, useTT>(nodes, board, batch, null, maps);
            }
        }

        if constexpr (depth == 2) nodes += null.quiet * null.count;

        if constexpr (depth >= 3 && useTT) iterateBatch<depth, side, kMoved>(nodes, batch);

        return nodes;
    }

    template <int depth, bool side, uint8_t kMoved, bool useTT>
    struct PerftGenerator {
        static __declspec(noinline) U64 generateMoves(const BoardState& board) {
            return allMoves<depth, side, kMoved, useTT>(board);
        }
    };

    template <bool side, uint8_t kMoved, bool useTT>
    struct PerftGenerator<1, side, kMoved, useTT> {
        ForceInline U64 generateMoves(const BoardState& board) {
            return allMoves<1, side, kMoved, useTT>(board);
        }
    };

    template <bool side, uint8_t kMoved, bool useTT>
    struct PerftGenerator<0, side, kMoved, useTT> {
        ForceInline U64 generateMoves(const BoardState& board) {
            return allMoves<0, side, kMoved, useTT>(board);
        }
    };

}

#endif