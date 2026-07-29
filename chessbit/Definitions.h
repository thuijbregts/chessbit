#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <immintrin.h>
#include <stdint.h>
#include <type_traits>
#include <cstdio>
#include <cstdio>
#include "AttackTables.h"

namespace defs {
#define StartPosition "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define KiwiPete "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"
#define EndGame "5nk1/pp3pp1/2p4p/q7/2PPB2P/P5P1/1P5K/3Q4 w - - 1 28"
#define Pos3 "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -"
#define Pos4 "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"
#define Pos5 "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
#define Pos6 "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"

#define Bitloop(X) for(;X; X = _blsr_u64(X))
#define BitReset(X) _blsr_u64(X)
#define SquareOf(X) _tzcnt_u64(X)
#define Ms1b(X) (63 - __lzcnt64(X))
#define Bitcount(X) __popcnt64(X)
#define GetBit(X, S) (X & 1ULL << S)//SQUARE_BITS[S]) same perf
#define SetBit(X, S) (X |= 1ULL << S)//SQUARE_BITS[S]) same perf
#define PopBit(X, S) (X ^= 1ULL << S)//SQUARE_BITS[S]) same perf
#define ClearBit(X, S) (X &= ~(1ULL << S))//SQUARE_BITS[S]) same perf
#define MoveBit(X, F, T) (X ^= 1ULL << F | 1ULL << T)// faster than U64 matrix

//#define ForceInline inline static constexpr
#define ForceInline __forceinline static constexpr
#define Inline inline static

	inline bool ttEnabled = true;

	enum Pieces { p, n, b, r, q, k, noPiece };

	enum class Piece { Pawn, Knight, Bishop, Rook, Queen, King };

	enum Sides { white, black, both };

	enum CastlingPerms { wk = 1, wq = 2, bk = 4, bq = 8 };

	enum Squares {
		a8, b8, c8, d8, e8, f8, g8, h8,
		a7, b7, c7, d7, e7, f7, g7, h7,
		a6, b6, c6, d6, e6, f6, g6, h6,
		a5, b5, c5, d5, e5, f5, g5, h5,
		a4, b4, c4, d4, e4, f4, g4, h4,
		a3, b3, c3, d3, e3, f3, g3, h3,
		a2, b2, c2, d2, e2, f2, g2, h2,
		a1, b1, c1, d1, e1, f1, g1, h1, noSquare
	};

	struct NullMaps {
		U64 eMap = 0; //squares that change move count
		U64 ePCL = 0; //en passant candidates left
		U64 ePCR = 0; //en passant candidates right
		U64 kKZ = 0; //king zone threatening king moves
		U64 pKZ = 0; //pawn zone threatening king moves
		U64 nKZ = 0; //knight zone threatening king moves
		U64 bKZ = 0; //bishop zone threatening king moves
		U64 rKZ = 0; //rook zone threatening king moves
		U64 bPins = 0; //squares that will cause bishop pins or check
		U64 rPins = 0; //squares that will cause rook pins or check
		U64 cstlBit = 0; //b2 or b7 square attacking castle passing square, otherwise unseen in other maps
	};

	struct Zobrist {
		U64 high;
		U64 low;

		bool operator==(const Zobrist& z) const noexcept {
			return high == z.high && low == z.low;
		}
	};

	constexpr char ASCII_PIECES[2][7] = { { 'P', 'N', 'B', 'R', 'Q', 'K', '.' }, { 'p', 'n', 'b', 'r', 'q', 'k', '.' } };

	inline constexpr int getPieceForCharacter(char c) {
		switch (c) {
		case 'P':
			return p;
		case 'N':
			return n;
		case 'B':
			return b;
		case 'R':
			return r;
		case 'Q':
			return q;
		case 'K':
			return k;
		case 'p':
			return p;
		case 'n':
			return n;
		case 'b':
			return b;
		case 'r':
			return r;
		case 'q':
			return q;
		case 'k':
			return k;
		}
		return -1;
	}

	constexpr U64 FULL_BOARD = ~0;

	constexpr int MAX_DEPTH = 18;

	constexpr int RANKS[64] = {
		8, 8, 8, 8, 8, 8, 8, 8,
		7, 7, 7, 7, 7, 7, 7, 7,
		6, 6, 6, 6, 6, 6, 6, 6,
		5, 5, 5, 5, 5, 5, 5, 5,
		4, 4, 4, 4, 4, 4, 4, 4,
		3, 3, 3, 3, 3, 3, 3, 3,
		2, 2, 2, 2, 2, 2, 2, 2,
		1, 1, 1, 1, 1, 1, 1, 1
	};

	constexpr U64 FIRST_COL = 0x101010101010101;
	constexpr U64 LAST_COL = 0x8080808080808080;

	constexpr U64 NO_EDGES = 0x007E7E7E7E7E7E00;

	constexpr U64 NO_EDGES_ROOK[64] = {
		0x017F7F7F7F7F7FFFULL, 0x007E7E7E7E7E7EFFULL, 0x007E7E7E7E7E7EFFULL, 0x007E7E7E7E7E7EFFULL,
		0x007E7E7E7E7E7EFFULL, 0x007E7E7E7E7E7EFFULL, 0x007E7E7E7E7E7EFFULL, 0x80FEFEFEFEFEFEFFULL,
		0x017F7F7F7F7F7F01ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL,
		0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x80FEFEFEFEFEFE80ULL,
		0x017F7F7F7F7F7F01ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL,
		0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x80FEFEFEFEFEFE80ULL,
		0x017F7F7F7F7F7F01ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL,
		0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x80FEFEFEFEFEFE80ULL,
		0x017F7F7F7F7F7F01ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL,
		0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x80FEFEFEFEFEFE80ULL,
		0x017F7F7F7F7F7F01ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL,
		0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x80FEFEFEFEFEFE80ULL,
		0x017F7F7F7F7F7F01ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL,
		0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL, 0x80FEFEFEFEFEFE80ULL,
		0xFF7F7F7F7F7F7F01ULL, 0xFF7E7E7E7E7E7E00ULL, 0xFF7E7E7E7E7E7E00ULL, 0xFF7E7E7E7E7E7E00ULL,
		0xFF7E7E7E7E7E7E00ULL, 0xFF7E7E7E7E7E7E00ULL, 0xFF7E7E7E7E7E7E00ULL, 0xFFFEFEFEFEFEFE80ULL
	};

	constexpr U64 RANK_BIT[64] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00, 0xff00,
		0xff0000, 0xff0000, 0xff0000, 0xff0000, 0xff0000, 0xff0000, 0xff0000, 0xff0000,
		0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000,
		0xff00000000, 0xff00000000, 0xff00000000, 0xff00000000, 0xff00000000, 0xff00000000, 0xff00000000, 0xff00000000,
		0xff0000000000, 0xff0000000000, 0xff0000000000, 0xff0000000000, 0xff0000000000, 0xff0000000000, 0xff0000000000, 0xff0000000000,
		0xff000000000000, 0xff000000000000, 0xff000000000000, 0xff000000000000, 0xff000000000000, 0xff000000000000, 0xff000000000000, 0xff000000000000,
		0xff00000000000000, 0xff00000000000000, 0xff00000000000000, 0xff00000000000000, 0xff00000000000000, 0xff00000000000000, 0xff00000000000000, 0xff00000000000000
	};

	constexpr U64 FILE_BIT[64] = {
		0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
		0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL,

		0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
		0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL,

		0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
		0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL,

		0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
		0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL,

		0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
		0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL,

		0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
		0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL,

		0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
		0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL,

		0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL, 0x0808080808080808ULL,
		0x1010101010101010ULL, 0x2020202020202020ULL, 0x4040404040404040ULL, 0x8080808080808080ULL,
	};

	constexpr U64 EN_PASSANT_RANK[2] = {
		0xff000000, 0xff00000000
	};

	constexpr U64 FIRST_PUSH_RANK[2] = { 0xff0000000000, 0xff0000 };

	constexpr int FILES[65] = {
		0, 1, 2, 3, 4, 5, 6, 7,
		0, 1, 2, 3, 4, 5, 6, 7,
		0, 1, 2, 3, 4, 5, 6, 7,
		0, 1, 2, 3, 4, 5, 6, 7,
		0, 1, 2, 3, 4, 5, 6, 7,
		0, 1, 2, 3, 4, 5, 6, 7,
		0, 1, 2, 3, 4, 5, 6, 7,
		0, 1, 2, 3, 4, 5, 6, 7, 8
	};


	constexpr bool RANK_7[64] = {
	   0, 0, 0, 0, 0, 0, 0, 0,
	   1, 1, 1, 1, 1, 1, 1, 1,
	   0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0,
	   0, 0, 0, 0, 0, 0, 0, 0
	};

	constexpr bool RANK_2[64] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 0
	};

	constexpr bool EDGES[64] = {
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1
	};

	constexpr bool INNER_RANKS[64] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 0
	};

	constexpr bool INNER_FILES[64] = {
		0, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 0,
		0, 1, 1, 1, 1, 1, 1, 0
	};

	//just to move counts, to avoid if "promo"
	constexpr int RANK_MULTIPLIER[2][64] = {
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			4, 4, 4, 4, 4, 4, 4, 4,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 1, 1, 1,
			4, 4, 4, 4, 4, 4, 4, 4,
			0, 0, 0, 0, 0, 0, 0, 0
		}
	};

	constexpr int PAWN_ATTACK_COUNT_CHECK[65] = {
		4, 4, 4, 4, 4, 4, 4, 4,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		4, 4, 4, 4, 4, 4, 4, 4, 0
	};

	constexpr U64 NULL_MASK[65] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, FULL_BOARD
	};

	constexpr int PAWN_PUSH[2] = { -8, 8 };
	constexpr int PAWN_DOUBLE_PUSH[2] = { -16, 16 };
	constexpr int PAWN_LEFT[2] = { -9, 7 };
	constexpr int PAWN_RIGHT[2] = { -7, 9 };
	const bool* const PROMO_RANK[2] = { RANK_7, RANK_2 };
	const bool* const DOUBLE_PUSH_RANK[2] = { RANK_2, RANK_7 };

	constexpr bool CAPTURE_ONLY[] = { true, true, false, false, false, false };

	constexpr uint8_t KING_MOVED[] = { 1, 2, 3 };

	constexpr int CASTLE_K = 0;
	constexpr int CASTLE_Q = 1;

	constexpr int NO_CASTLE[2] = { bk | bq, wk | wq };
	constexpr int NO_CASTLE_ROOK[64] = {
		7, 15, 15, 15, 15, 15, 15, 11,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		13, 15, 15, 15, 15, 15, 15, 14
	};

	constexpr U64 CASTLE_ROOK_INITIAL[2][2] = { { 0x8000000000000000, 0x100000000000000 }, { 0x80, 0x1 } };

	constexpr U64 ROOKS_INITIAL[4] = { CASTLE_ROOK_INITIAL[white][0], CASTLE_ROOK_INITIAL[white][1], CASTLE_ROOK_INITIAL[black][0], CASTLE_ROOK_INITIAL[black][1] };

	constexpr int CASTLE_ROOK_KING[2] = { h1, h8 };

	constexpr int CASTLE_ROOK_QUEEN[2] = { a1, a8 };

	constexpr U64 PROMO_RANKS[2] = {
		65280ULL, 71776119061217280ULL
	};

	constexpr U64 FIRST_RANKS[2] = {
		71776119061217280ULL, 65280ULL
	};

	constexpr U64 LAST_RANKS[2] = {
		0xff, 0xff00000000000000
	};

	constexpr U64 MIDDLE_RANKS = 0xffffffff0000;

	constexpr int CASTLING_SIDE_K[2] = { 0, 2 };

	constexpr int CASTLING_SIDE_Q[2] = { 1, 3 };

	constexpr int CASTLING_BIT_K[2] = { 1, 4 };

	constexpr int CASTLING_BIT_Q[2] = { 2, 8 };

	constexpr U64 CASTLING_OCCUPIED_SQUARES[4] = {
		6917529027641081856ULL, 1008806316530991104ULL,
		96ULL, 14ULL
	};

	//test with rook square also in
	/*constexpr U64 CASTLING_OCCUPIED_SQUARES[4] = {
		0xe000000000000000, 0xf00000000000000,
		0xe0, 0xf
	};*/

	constexpr int CASTLING_ATTACK_SQUARES[4][2] = {
		{ f1, g1 }, { c1, d1 }, { f8, g8 }, { c8, d8 }
	};

	constexpr U64 CASTLING_PASSING_SQUARES[4] = {
		0x6000000000000000, 0xc00000000000000, 0x60, 0xc
	};

	constexpr U64 CASTLING_PASSING_SQUARES_NULL[2][16] = {
		{
			0x0ULL,                          // 0000
			0x6000000000000000ULL,           // 0001
			0x0C00000000000000ULL,           // 0010
			0x6C00000000000000ULL,           // 0011
			0x0ULL,                          // 0100
			0x6000000000000000ULL,           // 0101
			0x0C00000000000000ULL,           // 0110
			0x6C00000000000000ULL,           // 0111
			0x0ULL,                          // 1000
			0x6000000000000000ULL,           // 1001
			0x0C00000000000000ULL,           // 1010
			0x6C00000000000000ULL,           // 1011
			0x0ULL,                          // 1100
			0x6000000000000000ULL,           // 1101
			0x0C00000000000000ULL,           // 1110
			0x6C00000000000000ULL            // 1111
		},
		{
			0x0ULL,                          // 0000
			0x0ULL,                          // 0001
			0x0ULL,                          // 0010
			0x0ULL,                          // 0011
			0x60ULL,                         // 0100
			0x60ULL,                         // 0101
			0x60ULL,                         // 0110
			0x60ULL,                         // 0111
			0x0CULL,                         // 1000
			0x0CULL,                         // 1001
			0x0CULL,                         // 1010
			0x0CULL,                         // 1011
			0x6CULL,                         // 1100
			0x6CULL,                         // 1101
			0x6CULL,                         // 1110
			0x6CULL                          // 1111
		}
	};

	constexpr int CASTLING[4] = {
		wk, wq,
		bk, bq
	};

	constexpr int CASTLING_KING[2] = {
		wk, bk
	};

	constexpr int CASTLING_QUEEN[2] = {
		wq, bq
	};

	constexpr U64 CASTLING_ROOK_KING[2] = {
		0x8000000000000000, 0x80
	};

	constexpr U64 CASTLING_ROOK_QUEEN[2] = {
		0x100000000000000, 0x1
	};

	constexpr U64 CASTLING_ROOK[4] = {
		0x8000000000000000, 0x100000000000000,
		0x80, 0x1
	};

	constexpr int NO_CASTLE_ROOK_KING[2] = {
		14, 11
	};

	constexpr int NO_CASTLE_ROOK_QUEEN[2] = {
		13, 7
	};

	constexpr int CASTLING_BOTH[2] = { wk | wq, bk | bq };

	constexpr int KING_SOURCE_SQUARE[2] = {
		e1, e8
	};

	constexpr int CASTLING_KING_SOURCE_SQUARE[4] = {
		e1, e1,
		e8, e8
	};

	constexpr int CASTLING_ROOK_SOURCE_SQUARE[4] = {
		h1, a1,
		h8, a8
	};

	constexpr int CASTLING_KING_TARGET_SQUARE[4] = {
		g1, c1,
		g8, c8
	};

	constexpr int CASTLING_ROOK_TARGET_SQUARE[4] = {
		f1, d1,
		f8, d8
	};

	constexpr int CASTLING_SIDE[4] = {
		white, white,
		black, black
	};

	//squares where the enemy pieces can't be in order to castle
	//for white: king castle: e2, g2  queen castle: c2, e2
	constexpr U64 CASTLING_FORBIDDEN_SQUARES[4] = {
		0xf0000000000000, 0x1e000000000000,
		0xf000, 0x1e00
	};

	//square where the enemy pieces can't be in order to castle, except the knight
	//for white: king castle: f2  queen castle: d2
	constexpr U64 CASTLING_FORBIDDEN_SQUARE_EXCEPT_KNIGHT[4] = {
		9007199254740992ULL, 2251799813685248ULL,
		8192ULL, 2048ULL
	};
	//square where the enemy pieces can't be in order to castle, except the rook
	//for white: king castle: h2  queen castle: b2
	constexpr U64 CASTLING_FORBIDDEN_SQUARE_EXCEPT_ROOK[4] = {
		36028797018963968ULL, 562949953421312ULL,
		32768ULL, 512ULL
	};
	//squares where the knights can't be in order to castle
	/*constexpr U64 CASTLING_FORBIDDEN_KNIGHT_SQUARES[4] = {
		43048079250685952ULL, 14388209161076736ULL,
		15767552ULL, 1979136ULL
	};*/

	constexpr U64 CASTLING_FORBIDDEN_KNIGHT_SQUARES[4] = {
		0x98f00000000000, 0x331e0000000000,
		0xf09800, 0x1e3300
	};

	//for null move; square that needs to be considered because attacking castle passing square
	constexpr U64 CASTLE_NULL_BIT[2] = {
		(1ULL << b2), (1ULL << b7)
	};

	constexpr int CASTLE_ROOK_FROM[64] = {
		0,0,0,0,0,0,7,0,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,56,0,0,0,63,0
	};

	constexpr int CASTLE_ROOK_TO[64] = {
		0,0,3,0,0,0,5,0,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		0,0,59,0,0,0,61,0
	};

	constexpr int EN_PASSANT_SQUARES[64][64] = {
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,16,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,17,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,18,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,19,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,20,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,21,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,22,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,23,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,40,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,41,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,42,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,43,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,44,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,45,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,46,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,47,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,},
		{noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,noSquare,}
	};

	const char* const SQUARE_NAMES[65] = {
		"a8","b8","c8","d8","e8","f8","g8","h8",
		"a7","b7","c7","d7","e7","f7","g7","h7",
		"a6","b6","c6","d6","e6","f6","g6","h6",
		"a5","b5","c5","d5","e5","f5","g5","h5",
		"a4","b4","c4","d4","e4","f4","g4","h4",
		"a3","b3","c3","d3","e3","f3","g3","h3",
		"a2","b2","c2","d2","e2","f2","g2","h2",
		"a1","b1","c1","d1","e1","f1","g1","h1", "no"
	};

	constexpr int ROOK_OCCUPANCY_BITS[64] = {
		12, 11, 11, 11, 11, 11, 11, 12,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		12, 11, 11, 11, 11, 11, 11, 12
	};

	constexpr int BISHOP_OCCUPANCY_BITS[64] = {
		6, 5, 5, 5, 5, 5, 5, 6,
		5, 5, 5, 5, 5, 5, 5, 5,
		5, 5, 7, 7, 7, 7, 5, 5,
		5, 5, 7, 9, 9, 7, 5, 5,
		5, 5, 7, 9, 9, 7, 5, 5,
		5, 5, 7, 7, 7, 7, 5, 5,
		5, 5, 5, 5, 5, 5, 5, 5,
		6, 5, 5, 5, 5, 5, 5, 6
	};

	constexpr int ROOK_SHIFT[64] = {
		52, 53, 53, 53, 53, 53, 53, 52,
		53, 54, 54, 54, 54, 54, 54, 53,
		53, 54, 54, 54, 54, 54, 54, 53,
		53, 54, 54, 54, 54, 54, 54, 53,
		53, 54, 54, 54, 54, 54, 54, 53,
		53, 54, 54, 54, 54, 54, 54, 53,
		53, 54, 54, 54, 54, 54, 54, 53,
		52, 53, 53, 53, 53, 53, 53, 52
	};

	constexpr int BISHOP_SHIFT[64] = {
		58, 59, 59, 59, 59, 59, 59, 58,
		59, 59, 59, 59, 59, 59, 59, 59,
		59, 59, 57, 57, 57, 57, 59, 59,
		59, 59, 57, 55, 55, 57, 59, 59,
		59, 59, 57, 55, 55, 57, 59, 59,
		59, 59, 57, 57, 57, 57, 59, 59,
		59, 59, 59, 59, 59, 59, 59, 59,
		58, 59, 59, 59, 59, 59, 59, 58
	};

	constexpr int PAWN_OCCUPANCY_BITS[2][64] = {
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			2, 3, 3, 3, 3, 3, 3, 2,
			2, 3, 3, 3, 3, 3, 3, 2,
			2, 3, 3, 3, 3, 3, 3, 2,
			2, 3, 3, 3, 3, 3, 3, 2,
			2, 3, 3, 3, 3, 3, 3, 2,
			3, 4, 4, 4, 4, 4, 4, 3,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			3, 4, 4, 4, 4, 4, 4, 3,
			2, 3, 3, 3, 3, 3, 3, 2,
			2, 3, 3, 3, 3, 3, 3, 2,
			2, 3, 3, 3, 3, 3, 3, 2,
			2, 3, 3, 3, 3, 3, 3, 2,
			2, 3, 3, 3, 3, 3, 3, 2,
			0, 0, 0, 0, 0, 0, 0, 0
		}
	};

	constexpr int PAWN_SHIFT[2][64] = {
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			62, 61, 61, 61, 61, 61, 61, 62,
			62, 61, 61, 61, 61, 61, 61, 62,
			62, 61, 61, 61, 61, 61, 61, 62,
			62, 61, 61, 61, 61, 61, 61, 62,
			62, 61, 61, 61, 61, 61, 61, 62,
			61, 60, 60, 60, 60, 60, 60, 61,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		{
			0, 0, 0, 0, 0, 0, 0, 0,
			61, 60, 60, 60, 60, 60, 60, 61,
			62, 61, 61, 61, 61, 61, 61, 62,
			62, 61, 61, 61, 61, 61, 61, 62,
			62, 61, 61, 61, 61, 61, 61, 62,
			62, 61, 61, 61, 61, 61, 61, 62,
			62, 61, 61, 61, 61, 61, 61, 62,
			0, 0, 0, 0, 0, 0, 0, 0
		}
	};

	constexpr int KING_ATTACKS_BIT_SHIFTS[64] = {
		0,  0,  1,  2,  3,  4,  5,  6,
		0,  0,  1,  2,  3,  4,  5,  6,
		8,  8,  9,  10, 11, 12, 13, 14,
		16, 16, 17, 18, 19, 20, 21, 22,
		24, 24, 25, 26, 27, 28, 29, 30,
		32, 32, 33, 34, 35, 36, 37, 38,
		40, 40, 41, 42, 43, 44, 45, 46,
		48, 48, 49, 50, 51, 52, 53, 54
	};

	constexpr U64 SQUARE_BITS[65] = {
		0x1ULL,0x2ULL,0x4ULL,0x8ULL,0x10ULL,0x20ULL,0x40ULL,0x80ULL,
		0x100ULL,0x200ULL,0x400ULL,0x800ULL,0x1000ULL,0x2000ULL,0x4000ULL,0x8000ULL,
		0x10000ULL,0x20000ULL,0x40000ULL,0x80000ULL,0x100000ULL,0x200000ULL,0x400000ULL,0x800000ULL,
		0x1000000ULL,0x2000000ULL,0x4000000ULL,0x8000000ULL,0x10000000ULL,0x20000000ULL,0x40000000ULL,0x80000000ULL,
		0x100000000ULL,0x200000000ULL,0x400000000ULL,0x800000000ULL,0x1000000000ULL,0x2000000000ULL,0x4000000000ULL,0x8000000000ULL,
		0x10000000000ULL,0x20000000000ULL,0x40000000000ULL,0x80000000000ULL,0x100000000000ULL,0x200000000000ULL,0x400000000000ULL,0x800000000000ULL,
		0x1000000000000ULL,0x2000000000000ULL,0x4000000000000ULL,0x8000000000000ULL,0x10000000000000ULL,0x20000000000000ULL,0x40000000000000ULL,0x80000000000000ULL,
		0x100000000000000ULL,0x200000000000000ULL,0x400000000000000ULL,0x800000000000000ULL,0x1000000000000000ULL,0x2000000000000000ULL,0x4000000000000000ULL,0x8000000000000000ULL,0x0ULL
	};

	constexpr int MVV_LVA[6] = { 100, 200, 300, 400, 500, 600 };

	constexpr U64 KING_ATTACKS[] = {
		0x0000000000000302, 0x0000000000000705, 0x0000000000000E0A, 0x0000000000001C14, 0x0000000000003828, 0x0000000000007050, 0x000000000000E0A0, 0x000000000000C040,
		0x0000000000030203, 0x0000000000070507, 0x00000000000E0A0E, 0x00000000001C141C, 0x0000000000382838, 0x0000000000705070, 0x0000000000E0A0E0, 0x0000000000C040C0,
		0x0000000003020300, 0x0000000007050700, 0x000000000E0A0E00, 0x000000001C141C00, 0x0000000038283800, 0x0000000070507000, 0x00000000E0A0E000, 0x00000000C040C000,
		0x0000000302030000, 0x0000000705070000, 0x0000000E0A0E0000, 0x0000001C141C0000, 0x0000003828380000, 0x0000007050700000, 0x000000E0A0E00000, 0x000000C040C00000,
		0x0000030203000000, 0x0000070507000000, 0x00000E0A0E000000, 0x00001C141C000000, 0x0000382838000000, 0x0000705070000000, 0x0000E0A0E0000000, 0x0000C040C0000000,
		0x0003020300000000, 0x0007050700000000, 0x000E0A0E00000000, 0x001C141C00000000, 0x0038283800000000, 0x0070507000000000, 0x00E0A0E000000000, 0x00C040C000000000,
		0x0302030000000000, 0x0705070000000000, 0x0E0A0E0000000000, 0x1C141C0000000000, 0x3828380000000000, 0x7050700000000000, 0xE0A0E00000000000, 0xC040C00000000000,
		0x0203000000000000, 0x0507000000000000, 0x0A0E000000000000, 0x141C000000000000, 0x2838000000000000, 0x5070000000000000, 0xA0E0000000000000, 0x40C0000000000000,
	};

	constexpr U64 KING_ZONES[64] = {
		0x70404,0xf0808,0x1f1111,0x3e2222,0x7c4444,0xf88888,0xf01010,0xe02020,
		0x7040404,0xf080808,0x1f111111,0x3e222222,0x7c444444,0xf8888888,0xf0101010,0xe0202020,
		0x704040407,0xf0808080f,0x1f1111111f,0x3e2222223e,0x7c4444447c,0xf8888888f8,0xf0101010f0,0xe0202020e0,
		0x70404040700,0xf0808080f00,0x1f1111111f00,0x3e2222223e00,0x7c4444447c00,0xf8888888f800,0xf0101010f000,0xe0202020e000,
		0x7040404070000,0xf0808080f0000,0x1f1111111f0000,0x3e2222223e0000,0x7c4444447c0000,0xf8888888f80000,0xf0101010f00000,0xe0202020e00000,0x704040407000000,
		0xf0808080f000000,0x1f1111111f000000,0x3e2222223e000000,0x7c4444447c000000,0xf8888888f8000000,0xf0101010f0000000,0xe0202020e0000000,0x404040700000000,
		0x808080f00000000,0x1111111f00000000,0x2222223e00000000,0x4444447c00000000,0x888888f800000000,0x101010f000000000,0x202020e000000000,0x404070000000000,
		0x8080f0000000000,0x11111f0000000000,0x22223e0000000000,0x44447c0000000000,0x8888f80000000000,0x1010f00000000000,0x2020e00000000000
	};

	constexpr U64 KNIGHT_ATTACKS[] = {
		0x0000000000020400, 0x0000000000050800, 0x00000000000A1100, 0x0000000000142200, 0x0000000000284400, 0x0000000000508800, 0x0000000000A01000, 0x0000000000402000,
		0x0000000002040004, 0x0000000005080008, 0x000000000A110011, 0x0000000014220022, 0x0000000028440044, 0x0000000050880088, 0x00000000A0100010, 0x0000000040200020,
		0x0000000204000402, 0x0000000508000805, 0x0000000A1100110A, 0x0000001422002214, 0x0000002844004428, 0x0000005088008850, 0x000000A0100010A0, 0x0000004020002040,
		0x0000020400040200, 0x0000050800080500, 0x00000A1100110A00, 0x0000142200221400, 0x0000284400442800, 0x0000508800885000, 0x0000A0100010A000, 0x0000402000204000,
		0x0002040004020000, 0x0005080008050000, 0x000A1100110A0000, 0x0014220022140000, 0x0028440044280000, 0x0050880088500000, 0x00A0100010A00000, 0x0040200020400000,
		0x0204000402000000, 0x0508000805000000, 0x0A1100110A000000, 0x1422002214000000, 0x2844004428000000, 0x5088008850000000, 0xA0100010A0000000, 0x4020002040000000,
		0x0400040200000000, 0x0800080500000000, 0x1100110A00000000, 0x2200221400000000, 0x4400442800000000, 0x8800885000000000, 0x100010A000000000, 0x2000204000000000,
		0x0004020000000000, 0x0008050000000000, 0x00110A0000000000, 0x0022140000000000, 0x0044280000000000, 0x0088500000000000, 0x0010A00000000000, 0x0020400000000000,
	};

	constexpr U64 CASTLE_MASKS_BISHOP[4] = {
		0xf0980c06030100, 0x1e3361c0800000,
		0x103060c98f000, 0x80c061331e00
	};

	constexpr U64 CASTLE_MASKS_ROOK[4] = {
		0x60606060606060, 0xc0c0c0c0c0c0c,
		0x6060606060606000, 0xc0c0c0c0c0c0c00
	};

	constexpr int PAWN_OFFSETS[128] = {
		0,0,0,0,0,0,0,0,
		0,4,12,20,28,36,44,52,
		56,60,68,76,84,92,100,108,
		112,116,124,132,140,148,156,164,
		168,172,180,188,196,204,212,220,
		224,228,236,244,252,260,268,276,
		280,288,304,320,336,352,368,384,
		0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,
		392,400,416,432,448,464,480,496,
		504,508,516,524,532,540,548,556,
		560,564,572,580,588,596,604,612,
		616,620,628,636,644,652,660,668,
		672,676,684,692,700,708,716,724,
		728,732,740,748,756,764,772,780,
		0,0,0,0,0,0,0,0
	};

	constexpr int PAWN_OFFSETS1[2][64] = {
		{
			0,0,0,0,0,0,0,0,
			0,4,12,20,28,36,44,52,
			56,60,68,76,84,92,100,108,
			112,116,124,132,140,148,156,164,
			168,172,180,188,196,204,212,220,
			224,228,236,244,252,260,268,276,
			280,288,304,320,336,352,368,384,
			0,0,0,0,0,0,0,0
		},
		{
			0,0,0,0,0,0,0,0,
			392,400,416,432,448,464,480,496,
			504,508,516,524,532,540,548,556,
			560,564,572,580,588,596,604,612,
			616,620,628,636,644,652,660,668,
			672,676,684,692,700,708,716,724,
			728,732,740,748,756,764,772,780,
			0,0,0,0,0,0,0,0
		}
	};

	constexpr U64 PAWN_MASKS[128] = {
		0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,
		0x3ULL,0x7ULL,0xeULL,0x1cULL,0x38ULL,0x70ULL,0xe0ULL,0xc0ULL,
		0x300ULL,0x700ULL,0xe00ULL,0x1c00ULL,0x3800ULL,0x7000ULL,0xe000ULL,0xc000ULL,
		0x30000ULL,0x70000ULL,0xe0000ULL,0x1c0000ULL,0x380000ULL,0x700000ULL,0xe00000ULL,0xc00000ULL,
		0x3000000ULL,0x7000000ULL,0xe000000ULL,0x1c000000ULL,0x38000000ULL,0x70000000ULL,0xe0000000ULL,0xc0000000ULL,
		0x300000000ULL,0x700000000ULL,0xe00000000ULL,0x1c00000000ULL,0x3800000000ULL,0x7000000000ULL,0xe000000000ULL,0xc000000000ULL,
		0x30100000000ULL,0x70200000000ULL,0xe0400000000ULL,0x1c0800000000ULL,0x381000000000ULL,0x702000000000ULL,0xe04000000000ULL,0xc08000000000ULL,
		0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,
		0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,
		0x1030000ULL,0x2070000ULL,0x40e0000ULL,0x81c0000ULL,0x10380000ULL,0x20700000ULL,0x40e00000ULL,0x80c00000ULL,
		0x3000000ULL,0x7000000ULL,0xe000000ULL,0x1c000000ULL,0x38000000ULL,0x70000000ULL,0xe0000000ULL,0xc0000000ULL,
		0x300000000ULL,0x700000000ULL,0xe00000000ULL,0x1c00000000ULL,0x3800000000ULL,0x7000000000ULL,0xe000000000ULL,0xc000000000ULL,
		0x30000000000ULL,0x70000000000ULL,0xe0000000000ULL,0x1c0000000000ULL,0x380000000000ULL,0x700000000000ULL,0xe00000000000ULL,0xc00000000000ULL,
		0x3000000000000ULL,0x7000000000000ULL,0xe000000000000ULL,0x1c000000000000ULL,0x38000000000000ULL,0x70000000000000ULL,0xe0000000000000ULL,0xc0000000000000ULL,
		0x300000000000000ULL,0x700000000000000ULL,0xe00000000000000ULL,0x1c00000000000000ULL,0x3800000000000000ULL,0x7000000000000000ULL,0xe000000000000000ULL,0xc000000000000000ULL,
		0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL
	};

	constexpr U64 PAWN_MASKS1[2][64] = {
		{
			0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,
			0x3ULL,0x7ULL,0xeULL,0x1cULL,0x38ULL,0x70ULL,0xe0ULL,0xc0ULL,
			0x300ULL,0x700ULL,0xe00ULL,0x1c00ULL,0x3800ULL,0x7000ULL,0xe000ULL,0xc000ULL,
			0x30000ULL,0x70000ULL,0xe0000ULL,0x1c0000ULL,0x380000ULL,0x700000ULL,0xe00000ULL,0xc00000ULL,
			0x3000000ULL,0x7000000ULL,0xe000000ULL,0x1c000000ULL,0x38000000ULL,0x70000000ULL,0xe0000000ULL,0xc0000000ULL,
			0x300000000ULL,0x700000000ULL,0xe00000000ULL,0x1c00000000ULL,0x3800000000ULL,0x7000000000ULL,0xe000000000ULL,0xc000000000ULL,
			0x30100000000ULL,0x70200000000ULL,0xe0400000000ULL,0x1c0800000000ULL,0x381000000000ULL,0x702000000000ULL,0xe04000000000ULL,0xc08000000000ULL,
			0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL
		},
		{
			0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,
			0x1030000ULL,0x2070000ULL,0x40e0000ULL,0x81c0000ULL,0x10380000ULL,0x20700000ULL,0x40e00000ULL,0x80c00000ULL,
			0x3000000ULL,0x7000000ULL,0xe000000ULL,0x1c000000ULL,0x38000000ULL,0x70000000ULL,0xe0000000ULL,0xc0000000ULL,
			0x300000000ULL,0x700000000ULL,0xe00000000ULL,0x1c00000000ULL,0x3800000000ULL,0x7000000000ULL,0xe000000000ULL,0xc000000000ULL,
			0x30000000000ULL,0x70000000000ULL,0xe0000000000ULL,0x1c0000000000ULL,0x380000000000ULL,0x700000000000ULL,0xe00000000000ULL,0xc00000000000ULL,
			0x3000000000000ULL,0x7000000000000ULL,0xe000000000000ULL,0x1c000000000000ULL,0x38000000000000ULL,0x70000000000000ULL,0xe0000000000000ULL,0xc0000000000000ULL,
			0x300000000000000ULL,0x700000000000000ULL,0xe00000000000000ULL,0x1c00000000000000ULL,0x3800000000000000ULL,0x7000000000000000ULL,0xe000000000000000ULL,0xc000000000000000ULL,
			0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL
		}
	};

	constexpr U64 PAWN_FRONT_MASKS[2][64] = {
		{
			0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,
			0x1ULL,0x2ULL,0x4ULL,0x8ULL,0x10ULL,0x20ULL,0x40ULL,0x80ULL,
			0x100ULL,0x200ULL,0x400ULL,0x800ULL,0x1000ULL,0x2000ULL,0x4000ULL,0x8000ULL,
			0x10000ULL,0x20000ULL,0x40000ULL,0x80000ULL,0x100000ULL,0x200000ULL,0x400000ULL,0x800000ULL,
			0x1000000ULL,0x2000000ULL,0x4000000ULL,0x8000000ULL,0x10000000ULL,0x20000000ULL,0x40000000ULL,0x80000000ULL,
			0x100000000ULL,0x200000000ULL,0x400000000ULL,0x800000000ULL,0x1000000000ULL,0x2000000000ULL,0x4000000000ULL,0x8000000000ULL,
			0x10100000000ULL,0x20200000000ULL,0x40400000000ULL,0x80800000000ULL,0x101000000000ULL,0x202000000000ULL,0x404000000000ULL,0x808000000000ULL,
			0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL
		},
		{
			0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,
			0x1010000ULL,0x2020000ULL,0x4040000ULL,0x8080000ULL,0x10100000ULL,0x20200000ULL,0x40400000ULL,0x80800000ULL,
			0x1000000ULL,0x2000000ULL,0x4000000ULL,0x8000000ULL,0x10000000ULL,0x20000000ULL,0x40000000ULL,0x80000000ULL,
			0x100000000ULL,0x200000000ULL,0x400000000ULL,0x800000000ULL,0x1000000000ULL,0x2000000000ULL,0x4000000000ULL,0x8000000000ULL,
			0x10000000000ULL,0x20000000000ULL,0x40000000000ULL,0x80000000000ULL,0x100000000000ULL,0x200000000000ULL,0x400000000000ULL,0x800000000000ULL,
			0x1000000000000ULL,0x2000000000000ULL,0x4000000000000ULL,0x8000000000000ULL,0x10000000000000ULL,0x20000000000000ULL,0x40000000000000ULL,0x80000000000000ULL,
			0x100000000000000ULL,0x200000000000000ULL,0x400000000000000ULL,0x800000000000000ULL,0x1000000000000000ULL,0x2000000000000000ULL,0x4000000000000000ULL,0x8000000000000000ULL,
			0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL,0x0ULL
		}
	};

	struct RookAttack
	{
		const U64* AttackPtr;
		const U64 Mask;

		constexpr RookAttack(int offset, U64 mask) : AttackPtr(ROOK_ATTACKS + offset), Mask(mask) {

		}

		__forceinline constexpr U64 operator[](const U64 blocker) const
		{
			return AttackPtr[_pext_u64(blocker, Mask)];
		}
	};

	struct BishopAttack
	{
		const U64* AttackPtr;
		const U64 Mask;

		constexpr BishopAttack(int offset, U64 mask) : AttackPtr(BISHOP_ATTACKS + offset), Mask(mask) {

		}

		__forceinline constexpr U64 operator[](const U64 blocker) const
		{
			return AttackPtr[_pext_u64(blocker, Mask)];
		}
	};

	static const RookAttack ROOK_ATTACKS_LOOKUP[64] = {
		RookAttack(ROOK_OFFSETS[0], ROOK_MASKS[0]),
		RookAttack(ROOK_OFFSETS[1], ROOK_MASKS[1]),
		RookAttack(ROOK_OFFSETS[2], ROOK_MASKS[2]),
		RookAttack(ROOK_OFFSETS[3], ROOK_MASKS[3]),
		RookAttack(ROOK_OFFSETS[4], ROOK_MASKS[4]),
		RookAttack(ROOK_OFFSETS[5], ROOK_MASKS[5]),
		RookAttack(ROOK_OFFSETS[6], ROOK_MASKS[6]),
		RookAttack(ROOK_OFFSETS[7], ROOK_MASKS[7]),
		RookAttack(ROOK_OFFSETS[8], ROOK_MASKS[8]),
		RookAttack(ROOK_OFFSETS[9], ROOK_MASKS[9]),
		RookAttack(ROOK_OFFSETS[10], ROOK_MASKS[10]),
		RookAttack(ROOK_OFFSETS[11], ROOK_MASKS[11]),
		RookAttack(ROOK_OFFSETS[12], ROOK_MASKS[12]),
		RookAttack(ROOK_OFFSETS[13], ROOK_MASKS[13]),
		RookAttack(ROOK_OFFSETS[14], ROOK_MASKS[14]),
		RookAttack(ROOK_OFFSETS[15], ROOK_MASKS[15]),
		RookAttack(ROOK_OFFSETS[16], ROOK_MASKS[16]),
		RookAttack(ROOK_OFFSETS[17], ROOK_MASKS[17]),
		RookAttack(ROOK_OFFSETS[18], ROOK_MASKS[18]),
		RookAttack(ROOK_OFFSETS[19], ROOK_MASKS[19]),
		RookAttack(ROOK_OFFSETS[20], ROOK_MASKS[20]),
		RookAttack(ROOK_OFFSETS[21], ROOK_MASKS[21]),
		RookAttack(ROOK_OFFSETS[22], ROOK_MASKS[22]),
		RookAttack(ROOK_OFFSETS[23], ROOK_MASKS[23]),
		RookAttack(ROOK_OFFSETS[24], ROOK_MASKS[24]),
		RookAttack(ROOK_OFFSETS[25], ROOK_MASKS[25]),
		RookAttack(ROOK_OFFSETS[26], ROOK_MASKS[26]),
		RookAttack(ROOK_OFFSETS[27], ROOK_MASKS[27]),
		RookAttack(ROOK_OFFSETS[28], ROOK_MASKS[28]),
		RookAttack(ROOK_OFFSETS[29], ROOK_MASKS[29]),
		RookAttack(ROOK_OFFSETS[30], ROOK_MASKS[30]),
		RookAttack(ROOK_OFFSETS[31], ROOK_MASKS[31]),
		RookAttack(ROOK_OFFSETS[32], ROOK_MASKS[32]),
		RookAttack(ROOK_OFFSETS[33], ROOK_MASKS[33]),
		RookAttack(ROOK_OFFSETS[34], ROOK_MASKS[34]),
		RookAttack(ROOK_OFFSETS[35], ROOK_MASKS[35]),
		RookAttack(ROOK_OFFSETS[36], ROOK_MASKS[36]),
		RookAttack(ROOK_OFFSETS[37], ROOK_MASKS[37]),
		RookAttack(ROOK_OFFSETS[38], ROOK_MASKS[38]),
		RookAttack(ROOK_OFFSETS[39], ROOK_MASKS[39]),
		RookAttack(ROOK_OFFSETS[40], ROOK_MASKS[40]),
		RookAttack(ROOK_OFFSETS[41], ROOK_MASKS[41]),
		RookAttack(ROOK_OFFSETS[42], ROOK_MASKS[42]),
		RookAttack(ROOK_OFFSETS[43], ROOK_MASKS[43]),
		RookAttack(ROOK_OFFSETS[44], ROOK_MASKS[44]),
		RookAttack(ROOK_OFFSETS[45], ROOK_MASKS[45]),
		RookAttack(ROOK_OFFSETS[46], ROOK_MASKS[46]),
		RookAttack(ROOK_OFFSETS[47], ROOK_MASKS[47]),
		RookAttack(ROOK_OFFSETS[48], ROOK_MASKS[48]),
		RookAttack(ROOK_OFFSETS[49], ROOK_MASKS[49]),
		RookAttack(ROOK_OFFSETS[50], ROOK_MASKS[50]),
		RookAttack(ROOK_OFFSETS[51], ROOK_MASKS[51]),
		RookAttack(ROOK_OFFSETS[52], ROOK_MASKS[52]),
		RookAttack(ROOK_OFFSETS[53], ROOK_MASKS[53]),
		RookAttack(ROOK_OFFSETS[54], ROOK_MASKS[54]),
		RookAttack(ROOK_OFFSETS[55], ROOK_MASKS[55]),
		RookAttack(ROOK_OFFSETS[56], ROOK_MASKS[56]),
		RookAttack(ROOK_OFFSETS[57], ROOK_MASKS[57]),
		RookAttack(ROOK_OFFSETS[58], ROOK_MASKS[58]),
		RookAttack(ROOK_OFFSETS[59], ROOK_MASKS[59]),
		RookAttack(ROOK_OFFSETS[60], ROOK_MASKS[60]),
		RookAttack(ROOK_OFFSETS[61], ROOK_MASKS[61]),
		RookAttack(ROOK_OFFSETS[62], ROOK_MASKS[62]),
		RookAttack(ROOK_OFFSETS[63], ROOK_MASKS[63])
	};

	static const BishopAttack BISHOP_ATTACKS_LOOKUP[64] = {
		BishopAttack(BISHOP_OFFSETS[0], BISHOP_MASKS[0]),
		BishopAttack(BISHOP_OFFSETS[1], BISHOP_MASKS[1]),
		BishopAttack(BISHOP_OFFSETS[2], BISHOP_MASKS[2]),
		BishopAttack(BISHOP_OFFSETS[3], BISHOP_MASKS[3]),
		BishopAttack(BISHOP_OFFSETS[4], BISHOP_MASKS[4]),
		BishopAttack(BISHOP_OFFSETS[5], BISHOP_MASKS[5]),
		BishopAttack(BISHOP_OFFSETS[6], BISHOP_MASKS[6]),
		BishopAttack(BISHOP_OFFSETS[7], BISHOP_MASKS[7]),
		BishopAttack(BISHOP_OFFSETS[8], BISHOP_MASKS[8]),
		BishopAttack(BISHOP_OFFSETS[9], BISHOP_MASKS[9]),
		BishopAttack(BISHOP_OFFSETS[10], BISHOP_MASKS[10]),
		BishopAttack(BISHOP_OFFSETS[11], BISHOP_MASKS[11]),
		BishopAttack(BISHOP_OFFSETS[12], BISHOP_MASKS[12]),
		BishopAttack(BISHOP_OFFSETS[13], BISHOP_MASKS[13]),
		BishopAttack(BISHOP_OFFSETS[14], BISHOP_MASKS[14]),
		BishopAttack(BISHOP_OFFSETS[15], BISHOP_MASKS[15]),
		BishopAttack(BISHOP_OFFSETS[16], BISHOP_MASKS[16]),
		BishopAttack(BISHOP_OFFSETS[17], BISHOP_MASKS[17]),
		BishopAttack(BISHOP_OFFSETS[18], BISHOP_MASKS[18]),
		BishopAttack(BISHOP_OFFSETS[19], BISHOP_MASKS[19]),
		BishopAttack(BISHOP_OFFSETS[20], BISHOP_MASKS[20]),
		BishopAttack(BISHOP_OFFSETS[21], BISHOP_MASKS[21]),
		BishopAttack(BISHOP_OFFSETS[22], BISHOP_MASKS[22]),
		BishopAttack(BISHOP_OFFSETS[23], BISHOP_MASKS[23]),
		BishopAttack(BISHOP_OFFSETS[24], BISHOP_MASKS[24]),
		BishopAttack(BISHOP_OFFSETS[25], BISHOP_MASKS[25]),
		BishopAttack(BISHOP_OFFSETS[26], BISHOP_MASKS[26]),
		BishopAttack(BISHOP_OFFSETS[27], BISHOP_MASKS[27]),
		BishopAttack(BISHOP_OFFSETS[28], BISHOP_MASKS[28]),
		BishopAttack(BISHOP_OFFSETS[29], BISHOP_MASKS[29]),
		BishopAttack(BISHOP_OFFSETS[30], BISHOP_MASKS[30]),
		BishopAttack(BISHOP_OFFSETS[31], BISHOP_MASKS[31]),
		BishopAttack(BISHOP_OFFSETS[32], BISHOP_MASKS[32]),
		BishopAttack(BISHOP_OFFSETS[33], BISHOP_MASKS[33]),
		BishopAttack(BISHOP_OFFSETS[34], BISHOP_MASKS[34]),
		BishopAttack(BISHOP_OFFSETS[35], BISHOP_MASKS[35]),
		BishopAttack(BISHOP_OFFSETS[36], BISHOP_MASKS[36]),
		BishopAttack(BISHOP_OFFSETS[37], BISHOP_MASKS[37]),
		BishopAttack(BISHOP_OFFSETS[38], BISHOP_MASKS[38]),
		BishopAttack(BISHOP_OFFSETS[39], BISHOP_MASKS[39]),
		BishopAttack(BISHOP_OFFSETS[40], BISHOP_MASKS[40]),
		BishopAttack(BISHOP_OFFSETS[41], BISHOP_MASKS[41]),
		BishopAttack(BISHOP_OFFSETS[42], BISHOP_MASKS[42]),
		BishopAttack(BISHOP_OFFSETS[43], BISHOP_MASKS[43]),
		BishopAttack(BISHOP_OFFSETS[44], BISHOP_MASKS[44]),
		BishopAttack(BISHOP_OFFSETS[45], BISHOP_MASKS[45]),
		BishopAttack(BISHOP_OFFSETS[46], BISHOP_MASKS[46]),
		BishopAttack(BISHOP_OFFSETS[47], BISHOP_MASKS[47]),
		BishopAttack(BISHOP_OFFSETS[48], BISHOP_MASKS[48]),
		BishopAttack(BISHOP_OFFSETS[49], BISHOP_MASKS[49]),
		BishopAttack(BISHOP_OFFSETS[50], BISHOP_MASKS[50]),
		BishopAttack(BISHOP_OFFSETS[51], BISHOP_MASKS[51]),
		BishopAttack(BISHOP_OFFSETS[52], BISHOP_MASKS[52]),
		BishopAttack(BISHOP_OFFSETS[53], BISHOP_MASKS[53]),
		BishopAttack(BISHOP_OFFSETS[54], BISHOP_MASKS[54]),
		BishopAttack(BISHOP_OFFSETS[55], BISHOP_MASKS[55]),
		BishopAttack(BISHOP_OFFSETS[56], BISHOP_MASKS[56]),
		BishopAttack(BISHOP_OFFSETS[57], BISHOP_MASKS[57]),
		BishopAttack(BISHOP_OFFSETS[58], BISHOP_MASKS[58]),
		BishopAttack(BISHOP_OFFSETS[59], BISHOP_MASKS[59]),
		BishopAttack(BISHOP_OFFSETS[60], BISHOP_MASKS[60]),
		BishopAttack(BISHOP_OFFSETS[61], BISHOP_MASKS[61]),
		BishopAttack(BISHOP_OFFSETS[62], BISHOP_MASKS[62]),
		BishopAttack(BISHOP_OFFSETS[63], BISHOP_MASKS[63])
	};

	ForceInline U64 getBishopAttacks(int square, U64 occupancy) {
		return BISHOP_ATTACKS_LOOKUP[square][occupancy];
	}

	ForceInline U64 getRookAttacks(int square, U64 occupancy) {
		return ROOK_ATTACKS_LOOKUP[square][occupancy];
	}

	ForceInline U64 getQueenAttacks(int square, U64 occupancy) {
		return getBishopAttacks(square, occupancy) | getRookAttacks(square, occupancy);
	}

	ForceInline U64 getKingAttacks(int square) {
		return KING_ATTACKS[square];
	}

	ForceInline U64 getKnightAttacks(int square) {
		return KNIGHT_ATTACKS[square];
	}

	ForceInline U64 getBishopAttackZone(int square, U64 occupancy, U64 mask) {
		return BISHOP_ATTACK_ZONES[square][_pext_u64(occupancy, mask)];
	}

	ForceInline U64 getRookAttackZone(int square, U64 occupancy, U64 mask) {
		return ROOK_ATTACK_ZONES[square][_pext_u64(occupancy, mask)];
	}

	template <bool side>
	ForceInline U64 getBishopAttackZoneCastle(U64 occupancy) {
		return BISHOP_ATTACK_ZONE_CASTLE[side][_pext_u64(occupancy, BISHOP_ATTACK_ZONE_CASTLE_MASK[side])];
	}

	template <bool side>
	ForceInline U64 getRookAttackZoneCastle(U64 occupancy) {
		return ROOK_ATTACK_ZONE_CASTLE[side][_pext_u64(occupancy, ROOK_ATTACK_ZONE_CASTLE_MASK[side])];
	}

	template <bool side>
	ForceInline U64 pawnsAtkLeft(U64 pM) {
		pM &= ~FIRST_COL;
		if constexpr (side == white) return pM >> 9;
		return pM << 7;
	}

	template <bool side>
	ForceInline U64 pawnsAtkRight(U64 pM) {
		pM &= ~LAST_COL;
		if constexpr (side == white) return pM >> 7;
		return pM << 9;
	}

	template <bool side>
	ForceInline U64 pawnsAtkForward(U64 pM) {
		if constexpr (side == white) return pM >> 8;
		return pM << 8;
	}

	template <bool side>
	ForceInline U64 pawnsAtkDouble(U64 pM) {
		if constexpr (side == white) return pM >> 16;
		return pM << 16;
	}

	ForceInline U64 left(U64 pM) {
		pM &= ~FIRST_COL;
		return pM >> 1;
	}

	ForceInline U64 right(U64 pM) {
		pM &= ~LAST_COL;
		return pM << 1;
	}

	template <int castlingSide>
	ForceInline U64 rookSwitch() noexcept {
		if constexpr (castlingSide == 0) return 0xa000000000000000;
		if constexpr (castlingSide == 1) return 0x900000000000000;
		if constexpr (castlingSide == 2) return 0xa0;
		if constexpr (castlingSide == 3) return 0x9;
	}

	template <int castlingSide>
	ForceInline U64 kingSwitch() noexcept {
		if constexpr (castlingSide == 0) return 0x5000000000000000;
		if constexpr (castlingSide == 1) return 0x1400000000000000;
		if constexpr (castlingSide == 2) return 0x50;
		if constexpr (castlingSide == 3) return 0x14;
	}

	template <int castlingSide>
	ForceInline U64 bothSwitch() noexcept {
		if constexpr (castlingSide == 0) return 0xa000000000000000 | 0x5000000000000000;
		if constexpr (castlingSide == 1) return 0x900000000000000 | 0x1400000000000000;
		if constexpr (castlingSide == 2) return 0xa0 | 0x50;
		if constexpr (castlingSide == 3) return 0x9 | 0x14;
	}
}

#endif