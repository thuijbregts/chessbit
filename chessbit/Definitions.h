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
#define BitReset(X) _blsr_u64(X))
#define SquareOf(X) _tzcnt_u64(X)
#define Ms1b(X) (63 - __lzcnt64(X))
#define Bitcount(X) __popcnt64(X)
#define GetBit(X, S) (X & 1ULL << S)//SQUARE_BITS[S]) same perf
#define SetBit(X, S) (X |= 1ULL << S)//SQUARE_BITS[S]) same perf
#define PopBit(X, S) (X ^= 1ULL << S)//SQUARE_BITS[S]) same perf
#define ClearBit(X, S) (X &= ~(1ULL << S))//SQUARE_BITS[S]) same perf
#define MoveBit(X, F, T) (X ^= 1ULL << F | 1ULL << T)// faster than U64 matrix

//#define ForceInline inline static
#define ForceInline __forceinline static
#define Inline inline static

	enum Pieces { p, n, b, r, q, k, noPiece };

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

	constexpr U64 EN_PASSANT_RANK[2] = {
		0xff000000, 0xff00000000
	};

	constexpr U64 FIRST_PUSH_RANK[2] = { 0xff0000000000, 0xff0000 };

	constexpr int FILES[64] = {
		1, 2, 3, 4, 5, 6, 7, 8,
		1, 2, 3, 4, 5, 6, 7, 8,
		1, 2, 3, 4, 5, 6, 7, 8,
		1, 2, 3, 4, 5, 6, 7, 8,
		1, 2, 3, 4, 5, 6, 7, 8,
		1, 2, 3, 4, 5, 6, 7, 8,
		1, 2, 3, 4, 5, 6, 7, 8,
		1, 2, 3, 4, 5, 6, 7, 8
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

	constexpr U64 PASSANT_PIN_RESULT[65] = {
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

	constexpr int CASTLING_KING_TARGET_SQUARE[4] = {
		g1, c1,
		g8, c8
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

	__forceinline constexpr U64 _pext_u64_emulated(U64 val, U64 mask) {
		U64 res = 0;
		for (U64 bb = 1; mask != 0; bb += bb) {
			if (val & mask & (0ull - mask)) {
				res |= bb;
			}
			mask &= (mask - 1);
		}
		return res;
	}

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

	struct RookPin
	{
		const U64* AttackPtr;
		const U64 Mask;

		constexpr RookPin(int offset, U64 mask) : AttackPtr(ROOK_PINS + offset), Mask(mask) {

		}

		__forceinline constexpr U64 operator[](const U64 blocker) const
		{
			return AttackPtr[_pext_u64(blocker, Mask)];
		}
	};

	struct BishopPin
	{
		const U64* AttackPtr;
		const U64 Mask;

		constexpr BishopPin(int offset, U64 mask) : AttackPtr(BISHOP_PINS + offset), Mask(mask) {

		}

		__forceinline constexpr U64 operator[](const U64 blocker) const
		{
			return AttackPtr[_pext_u64(blocker, Mask)];
		}
	};

	struct RookPinMask
	{
		const U64* AttackPtr;
		const U64 Mask;

		constexpr RookPinMask(int offset, U64 mask) : AttackPtr(ROOK_PIN_MASKS + offset), Mask(mask) {

		}

		__forceinline constexpr U64 operator[](const U64 blocker) const
		{
			return AttackPtr[_pext_u64(blocker, Mask)];
		}
	};

	struct BishopPinMask
	{
		const U64* AttackPtr;
		const U64 Mask;

		constexpr BishopPinMask(int offset, U64 mask) : AttackPtr(BISHOP_PIN_MASKS + offset), Mask(mask) {

		}

		__forceinline constexpr U64 operator[](const U64 blocker) const
		{
			return AttackPtr[_pext_u64(blocker, Mask)];
		}
	};

	struct PawnAttack
	{
		const U64* AttackPtr;
		const U64 Mask;

		constexpr PawnAttack(int offset, U64 mask) : AttackPtr(PAWN_ATTACKS + offset), Mask(mask) {

		}

		__forceinline constexpr U64 operator[](const U64 blocker) const
		{
			return AttackPtr[_pext_u64(blocker, Mask)];
		}
	};

	struct PawnAttackCount
	{
		const int* AttackPtr;
		const U64 Mask;

		constexpr PawnAttackCount(int offset, U64 mask) : AttackPtr(PAWN_ATTACKS_COUNT + offset), Mask(mask) {

		}

		__forceinline constexpr int operator[](const U64 blocker) const
		{
			return AttackPtr[_pext_u64(blocker, Mask)];
		}
	};

	struct RookCastleAttack
	{
		const U64* AttackPtr;
		const U64 Mask;

		constexpr RookCastleAttack(int square, U64 mask) : AttackPtr(CASTLE_ATTACKS_ROOK[square]), Mask(mask) {

		}

		__forceinline constexpr U64 operator[](const U64 blocker) const
		{
			return AttackPtr[_pext_u64(blocker, Mask)];
		}
	};

	struct BishopCastleAttack
	{
		const U64* AttackPtr;
		const U64 Mask;

		constexpr BishopCastleAttack(int square, U64 mask) : AttackPtr(CASTLE_ATTACKS_BISHOP[square]), Mask(mask) {

		}

		__forceinline constexpr U64 operator[](const U64 blocker) const
		{
			return AttackPtr[_pext_u64(blocker, Mask)];
		}
	};

	struct PawnKingAttack
	{
		const U64* AttackPtr;
		const U64 Mask;

		constexpr PawnKingAttack(U64 mask, bool side, int kingSquare) : AttackPtr(PAWN_KING_ATTACKS[side][kingSquare]), Mask(mask) {

		}

		__forceinline constexpr U64 operator[](const U64 blocker) const
		{
			return AttackPtr[_pext_u64(blocker, Mask)];
		}
	};

	struct RookAttackZone
	{
		const U64* AttackPtr;
		const U64 Mask;

		constexpr RookAttackZone(int square, U64 mask) : AttackPtr(ROOK_ATTACK_ZONES[square]), Mask(mask) {

		}

		__forceinline constexpr U64 operator[](const U64 blocker) const
		{
			return AttackPtr[_pext_u64(blocker, Mask)];
		}
	};

	struct BishopAttackZone
	{
		const U64* AttackPtr;
		const U64 Mask;

		constexpr BishopAttackZone(int square, U64 mask) : AttackPtr(BISHOP_ATTACK_ZONES[square]), Mask(mask) {

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

	static const RookPin ROOK_PINS_LOOKUP[64] = {
		RookPin(ROOK_OFFSETS[0], ROOK_MASKS[0]),
		RookPin(ROOK_OFFSETS[1], ROOK_MASKS[1]),
		RookPin(ROOK_OFFSETS[2], ROOK_MASKS[2]),
		RookPin(ROOK_OFFSETS[3], ROOK_MASKS[3]),
		RookPin(ROOK_OFFSETS[4], ROOK_MASKS[4]),
		RookPin(ROOK_OFFSETS[5], ROOK_MASKS[5]),
		RookPin(ROOK_OFFSETS[6], ROOK_MASKS[6]),
		RookPin(ROOK_OFFSETS[7], ROOK_MASKS[7]),
		RookPin(ROOK_OFFSETS[8], ROOK_MASKS[8]),
		RookPin(ROOK_OFFSETS[9], ROOK_MASKS[9]),
		RookPin(ROOK_OFFSETS[10], ROOK_MASKS[10]),
		RookPin(ROOK_OFFSETS[11], ROOK_MASKS[11]),
		RookPin(ROOK_OFFSETS[12], ROOK_MASKS[12]),
		RookPin(ROOK_OFFSETS[13], ROOK_MASKS[13]),
		RookPin(ROOK_OFFSETS[14], ROOK_MASKS[14]),
		RookPin(ROOK_OFFSETS[15], ROOK_MASKS[15]),
		RookPin(ROOK_OFFSETS[16], ROOK_MASKS[16]),
		RookPin(ROOK_OFFSETS[17], ROOK_MASKS[17]),
		RookPin(ROOK_OFFSETS[18], ROOK_MASKS[18]),
		RookPin(ROOK_OFFSETS[19], ROOK_MASKS[19]),
		RookPin(ROOK_OFFSETS[20], ROOK_MASKS[20]),
		RookPin(ROOK_OFFSETS[21], ROOK_MASKS[21]),
		RookPin(ROOK_OFFSETS[22], ROOK_MASKS[22]),
		RookPin(ROOK_OFFSETS[23], ROOK_MASKS[23]),
		RookPin(ROOK_OFFSETS[24], ROOK_MASKS[24]),
		RookPin(ROOK_OFFSETS[25], ROOK_MASKS[25]),
		RookPin(ROOK_OFFSETS[26], ROOK_MASKS[26]),
		RookPin(ROOK_OFFSETS[27], ROOK_MASKS[27]),
		RookPin(ROOK_OFFSETS[28], ROOK_MASKS[28]),
		RookPin(ROOK_OFFSETS[29], ROOK_MASKS[29]),
		RookPin(ROOK_OFFSETS[30], ROOK_MASKS[30]),
		RookPin(ROOK_OFFSETS[31], ROOK_MASKS[31]),
		RookPin(ROOK_OFFSETS[32], ROOK_MASKS[32]),
		RookPin(ROOK_OFFSETS[33], ROOK_MASKS[33]),
		RookPin(ROOK_OFFSETS[34], ROOK_MASKS[34]),
		RookPin(ROOK_OFFSETS[35], ROOK_MASKS[35]),
		RookPin(ROOK_OFFSETS[36], ROOK_MASKS[36]),
		RookPin(ROOK_OFFSETS[37], ROOK_MASKS[37]),
		RookPin(ROOK_OFFSETS[38], ROOK_MASKS[38]),
		RookPin(ROOK_OFFSETS[39], ROOK_MASKS[39]),
		RookPin(ROOK_OFFSETS[40], ROOK_MASKS[40]),
		RookPin(ROOK_OFFSETS[41], ROOK_MASKS[41]),
		RookPin(ROOK_OFFSETS[42], ROOK_MASKS[42]),
		RookPin(ROOK_OFFSETS[43], ROOK_MASKS[43]),
		RookPin(ROOK_OFFSETS[44], ROOK_MASKS[44]),
		RookPin(ROOK_OFFSETS[45], ROOK_MASKS[45]),
		RookPin(ROOK_OFFSETS[46], ROOK_MASKS[46]),
		RookPin(ROOK_OFFSETS[47], ROOK_MASKS[47]),
		RookPin(ROOK_OFFSETS[48], ROOK_MASKS[48]),
		RookPin(ROOK_OFFSETS[49], ROOK_MASKS[49]),
		RookPin(ROOK_OFFSETS[50], ROOK_MASKS[50]),
		RookPin(ROOK_OFFSETS[51], ROOK_MASKS[51]),
		RookPin(ROOK_OFFSETS[52], ROOK_MASKS[52]),
		RookPin(ROOK_OFFSETS[53], ROOK_MASKS[53]),
		RookPin(ROOK_OFFSETS[54], ROOK_MASKS[54]),
		RookPin(ROOK_OFFSETS[55], ROOK_MASKS[55]),
		RookPin(ROOK_OFFSETS[56], ROOK_MASKS[56]),
		RookPin(ROOK_OFFSETS[57], ROOK_MASKS[57]),
		RookPin(ROOK_OFFSETS[58], ROOK_MASKS[58]),
		RookPin(ROOK_OFFSETS[59], ROOK_MASKS[59]),
		RookPin(ROOK_OFFSETS[60], ROOK_MASKS[60]),
		RookPin(ROOK_OFFSETS[61], ROOK_MASKS[61]),
		RookPin(ROOK_OFFSETS[62], ROOK_MASKS[62]),
		RookPin(ROOK_OFFSETS[63], ROOK_MASKS[63])
	};

	static const BishopPin BISHOP_PINS_LOOKUP[64] = {
		BishopPin(BISHOP_OFFSETS[0], BISHOP_MASKS[0]),
		BishopPin(BISHOP_OFFSETS[1], BISHOP_MASKS[1]),
		BishopPin(BISHOP_OFFSETS[2], BISHOP_MASKS[2]),
		BishopPin(BISHOP_OFFSETS[3], BISHOP_MASKS[3]),
		BishopPin(BISHOP_OFFSETS[4], BISHOP_MASKS[4]),
		BishopPin(BISHOP_OFFSETS[5], BISHOP_MASKS[5]),
		BishopPin(BISHOP_OFFSETS[6], BISHOP_MASKS[6]),
		BishopPin(BISHOP_OFFSETS[7], BISHOP_MASKS[7]),
		BishopPin(BISHOP_OFFSETS[8], BISHOP_MASKS[8]),
		BishopPin(BISHOP_OFFSETS[9], BISHOP_MASKS[9]),
		BishopPin(BISHOP_OFFSETS[10], BISHOP_MASKS[10]),
		BishopPin(BISHOP_OFFSETS[11], BISHOP_MASKS[11]),
		BishopPin(BISHOP_OFFSETS[12], BISHOP_MASKS[12]),
		BishopPin(BISHOP_OFFSETS[13], BISHOP_MASKS[13]),
		BishopPin(BISHOP_OFFSETS[14], BISHOP_MASKS[14]),
		BishopPin(BISHOP_OFFSETS[15], BISHOP_MASKS[15]),
		BishopPin(BISHOP_OFFSETS[16], BISHOP_MASKS[16]),
		BishopPin(BISHOP_OFFSETS[17], BISHOP_MASKS[17]),
		BishopPin(BISHOP_OFFSETS[18], BISHOP_MASKS[18]),
		BishopPin(BISHOP_OFFSETS[19], BISHOP_MASKS[19]),
		BishopPin(BISHOP_OFFSETS[20], BISHOP_MASKS[20]),
		BishopPin(BISHOP_OFFSETS[21], BISHOP_MASKS[21]),
		BishopPin(BISHOP_OFFSETS[22], BISHOP_MASKS[22]),
		BishopPin(BISHOP_OFFSETS[23], BISHOP_MASKS[23]),
		BishopPin(BISHOP_OFFSETS[24], BISHOP_MASKS[24]),
		BishopPin(BISHOP_OFFSETS[25], BISHOP_MASKS[25]),
		BishopPin(BISHOP_OFFSETS[26], BISHOP_MASKS[26]),
		BishopPin(BISHOP_OFFSETS[27], BISHOP_MASKS[27]),
		BishopPin(BISHOP_OFFSETS[28], BISHOP_MASKS[28]),
		BishopPin(BISHOP_OFFSETS[29], BISHOP_MASKS[29]),
		BishopPin(BISHOP_OFFSETS[30], BISHOP_MASKS[30]),
		BishopPin(BISHOP_OFFSETS[31], BISHOP_MASKS[31]),
		BishopPin(BISHOP_OFFSETS[32], BISHOP_MASKS[32]),
		BishopPin(BISHOP_OFFSETS[33], BISHOP_MASKS[33]),
		BishopPin(BISHOP_OFFSETS[34], BISHOP_MASKS[34]),
		BishopPin(BISHOP_OFFSETS[35], BISHOP_MASKS[35]),
		BishopPin(BISHOP_OFFSETS[36], BISHOP_MASKS[36]),
		BishopPin(BISHOP_OFFSETS[37], BISHOP_MASKS[37]),
		BishopPin(BISHOP_OFFSETS[38], BISHOP_MASKS[38]),
		BishopPin(BISHOP_OFFSETS[39], BISHOP_MASKS[39]),
		BishopPin(BISHOP_OFFSETS[40], BISHOP_MASKS[40]),
		BishopPin(BISHOP_OFFSETS[41], BISHOP_MASKS[41]),
		BishopPin(BISHOP_OFFSETS[42], BISHOP_MASKS[42]),
		BishopPin(BISHOP_OFFSETS[43], BISHOP_MASKS[43]),
		BishopPin(BISHOP_OFFSETS[44], BISHOP_MASKS[44]),
		BishopPin(BISHOP_OFFSETS[45], BISHOP_MASKS[45]),
		BishopPin(BISHOP_OFFSETS[46], BISHOP_MASKS[46]),
		BishopPin(BISHOP_OFFSETS[47], BISHOP_MASKS[47]),
		BishopPin(BISHOP_OFFSETS[48], BISHOP_MASKS[48]),
		BishopPin(BISHOP_OFFSETS[49], BISHOP_MASKS[49]),
		BishopPin(BISHOP_OFFSETS[50], BISHOP_MASKS[50]),
		BishopPin(BISHOP_OFFSETS[51], BISHOP_MASKS[51]),
		BishopPin(BISHOP_OFFSETS[52], BISHOP_MASKS[52]),
		BishopPin(BISHOP_OFFSETS[53], BISHOP_MASKS[53]),
		BishopPin(BISHOP_OFFSETS[54], BISHOP_MASKS[54]),
		BishopPin(BISHOP_OFFSETS[55], BISHOP_MASKS[55]),
		BishopPin(BISHOP_OFFSETS[56], BISHOP_MASKS[56]),
		BishopPin(BISHOP_OFFSETS[57], BISHOP_MASKS[57]),
		BishopPin(BISHOP_OFFSETS[58], BISHOP_MASKS[58]),
		BishopPin(BISHOP_OFFSETS[59], BISHOP_MASKS[59]),
		BishopPin(BISHOP_OFFSETS[60], BISHOP_MASKS[60]),
		BishopPin(BISHOP_OFFSETS[61], BISHOP_MASKS[61]),
		BishopPin(BISHOP_OFFSETS[62], BISHOP_MASKS[62]),
		BishopPin(BISHOP_OFFSETS[63], BISHOP_MASKS[63])
	};

	static const RookPinMask ROOK_PIN_MASKS_LOOKUP[64] = {
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[0], ROOK_PIN_MASKS_MASKS[0]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[1], ROOK_PIN_MASKS_MASKS[1]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[2], ROOK_PIN_MASKS_MASKS[2]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[3], ROOK_PIN_MASKS_MASKS[3]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[4], ROOK_PIN_MASKS_MASKS[4]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[5], ROOK_PIN_MASKS_MASKS[5]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[6], ROOK_PIN_MASKS_MASKS[6]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[7], ROOK_PIN_MASKS_MASKS[7]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[8], ROOK_PIN_MASKS_MASKS[8]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[9], ROOK_PIN_MASKS_MASKS[9]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[10], ROOK_PIN_MASKS_MASKS[10]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[11], ROOK_PIN_MASKS_MASKS[11]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[12], ROOK_PIN_MASKS_MASKS[12]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[13], ROOK_PIN_MASKS_MASKS[13]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[14], ROOK_PIN_MASKS_MASKS[14]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[15], ROOK_PIN_MASKS_MASKS[15]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[16], ROOK_PIN_MASKS_MASKS[16]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[17], ROOK_PIN_MASKS_MASKS[17]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[18], ROOK_PIN_MASKS_MASKS[18]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[19], ROOK_PIN_MASKS_MASKS[19]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[20], ROOK_PIN_MASKS_MASKS[20]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[21], ROOK_PIN_MASKS_MASKS[21]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[22], ROOK_PIN_MASKS_MASKS[22]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[23], ROOK_PIN_MASKS_MASKS[23]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[24], ROOK_PIN_MASKS_MASKS[24]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[25], ROOK_PIN_MASKS_MASKS[25]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[26], ROOK_PIN_MASKS_MASKS[26]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[27], ROOK_PIN_MASKS_MASKS[27]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[28], ROOK_PIN_MASKS_MASKS[28]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[29], ROOK_PIN_MASKS_MASKS[29]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[30], ROOK_PIN_MASKS_MASKS[30]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[31], ROOK_PIN_MASKS_MASKS[31]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[32], ROOK_PIN_MASKS_MASKS[32]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[33], ROOK_PIN_MASKS_MASKS[33]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[34], ROOK_PIN_MASKS_MASKS[34]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[35], ROOK_PIN_MASKS_MASKS[35]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[36], ROOK_PIN_MASKS_MASKS[36]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[37], ROOK_PIN_MASKS_MASKS[37]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[38], ROOK_PIN_MASKS_MASKS[38]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[39], ROOK_PIN_MASKS_MASKS[39]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[40], ROOK_PIN_MASKS_MASKS[40]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[41], ROOK_PIN_MASKS_MASKS[41]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[42], ROOK_PIN_MASKS_MASKS[42]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[43], ROOK_PIN_MASKS_MASKS[43]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[44], ROOK_PIN_MASKS_MASKS[44]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[45], ROOK_PIN_MASKS_MASKS[45]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[46], ROOK_PIN_MASKS_MASKS[46]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[47], ROOK_PIN_MASKS_MASKS[47]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[48], ROOK_PIN_MASKS_MASKS[48]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[49], ROOK_PIN_MASKS_MASKS[49]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[50], ROOK_PIN_MASKS_MASKS[50]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[51], ROOK_PIN_MASKS_MASKS[51]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[52], ROOK_PIN_MASKS_MASKS[52]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[53], ROOK_PIN_MASKS_MASKS[53]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[54], ROOK_PIN_MASKS_MASKS[54]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[55], ROOK_PIN_MASKS_MASKS[55]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[56], ROOK_PIN_MASKS_MASKS[56]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[57], ROOK_PIN_MASKS_MASKS[57]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[58], ROOK_PIN_MASKS_MASKS[58]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[59], ROOK_PIN_MASKS_MASKS[59]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[60], ROOK_PIN_MASKS_MASKS[60]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[61], ROOK_PIN_MASKS_MASKS[61]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[62], ROOK_PIN_MASKS_MASKS[62]),
		RookPinMask(ROOK_PIN_MASKS_OFFSETS[63], ROOK_PIN_MASKS_MASKS[63])
	};

	static const BishopPinMask BISHOP_PIN_MASKS_LOOKUP[64] = {
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[0], BISHOP_PIN_MASKS_MASKS[0]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[1], BISHOP_PIN_MASKS_MASKS[1]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[2], BISHOP_PIN_MASKS_MASKS[2]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[3], BISHOP_PIN_MASKS_MASKS[3]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[4], BISHOP_PIN_MASKS_MASKS[4]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[5], BISHOP_PIN_MASKS_MASKS[5]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[6], BISHOP_PIN_MASKS_MASKS[6]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[7], BISHOP_PIN_MASKS_MASKS[7]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[8], BISHOP_PIN_MASKS_MASKS[8]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[9], BISHOP_PIN_MASKS_MASKS[9]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[10], BISHOP_PIN_MASKS_MASKS[10]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[11], BISHOP_PIN_MASKS_MASKS[11]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[12], BISHOP_PIN_MASKS_MASKS[12]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[13], BISHOP_PIN_MASKS_MASKS[13]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[14], BISHOP_PIN_MASKS_MASKS[14]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[15], BISHOP_PIN_MASKS_MASKS[15]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[16], BISHOP_PIN_MASKS_MASKS[16]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[17], BISHOP_PIN_MASKS_MASKS[17]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[18], BISHOP_PIN_MASKS_MASKS[18]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[19], BISHOP_PIN_MASKS_MASKS[19]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[20], BISHOP_PIN_MASKS_MASKS[20]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[21], BISHOP_PIN_MASKS_MASKS[21]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[22], BISHOP_PIN_MASKS_MASKS[22]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[23], BISHOP_PIN_MASKS_MASKS[23]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[24], BISHOP_PIN_MASKS_MASKS[24]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[25], BISHOP_PIN_MASKS_MASKS[25]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[26], BISHOP_PIN_MASKS_MASKS[26]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[27], BISHOP_PIN_MASKS_MASKS[27]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[28], BISHOP_PIN_MASKS_MASKS[28]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[29], BISHOP_PIN_MASKS_MASKS[29]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[30], BISHOP_PIN_MASKS_MASKS[30]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[31], BISHOP_PIN_MASKS_MASKS[31]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[32], BISHOP_PIN_MASKS_MASKS[32]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[33], BISHOP_PIN_MASKS_MASKS[33]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[34], BISHOP_PIN_MASKS_MASKS[34]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[35], BISHOP_PIN_MASKS_MASKS[35]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[36], BISHOP_PIN_MASKS_MASKS[36]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[37], BISHOP_PIN_MASKS_MASKS[37]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[38], BISHOP_PIN_MASKS_MASKS[38]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[39], BISHOP_PIN_MASKS_MASKS[39]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[40], BISHOP_PIN_MASKS_MASKS[40]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[41], BISHOP_PIN_MASKS_MASKS[41]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[42], BISHOP_PIN_MASKS_MASKS[42]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[43], BISHOP_PIN_MASKS_MASKS[43]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[44], BISHOP_PIN_MASKS_MASKS[44]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[45], BISHOP_PIN_MASKS_MASKS[45]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[46], BISHOP_PIN_MASKS_MASKS[46]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[47], BISHOP_PIN_MASKS_MASKS[47]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[48], BISHOP_PIN_MASKS_MASKS[48]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[49], BISHOP_PIN_MASKS_MASKS[49]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[50], BISHOP_PIN_MASKS_MASKS[50]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[51], BISHOP_PIN_MASKS_MASKS[51]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[52], BISHOP_PIN_MASKS_MASKS[52]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[53], BISHOP_PIN_MASKS_MASKS[53]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[54], BISHOP_PIN_MASKS_MASKS[54]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[55], BISHOP_PIN_MASKS_MASKS[55]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[56], BISHOP_PIN_MASKS_MASKS[56]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[57], BISHOP_PIN_MASKS_MASKS[57]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[58], BISHOP_PIN_MASKS_MASKS[58]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[59], BISHOP_PIN_MASKS_MASKS[59]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[60], BISHOP_PIN_MASKS_MASKS[60]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[61], BISHOP_PIN_MASKS_MASKS[61]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[62], BISHOP_PIN_MASKS_MASKS[62]),
		BishopPinMask(BISHOP_PIN_MASKS_OFFSETS[63], BISHOP_PIN_MASKS_MASKS[63])
	};

	static const BishopCastleAttack BISHOP_CASTLE_LOOKUP[4] = {
		BishopCastleAttack(0, CASTLE_MASKS_BISHOP[0]),
		BishopCastleAttack(1, CASTLE_MASKS_BISHOP[1]),
		BishopCastleAttack(2, CASTLE_MASKS_BISHOP[2]),
		BishopCastleAttack(3, CASTLE_MASKS_BISHOP[3])
	};

	static const RookCastleAttack ROOK_CASTLE_LOOKUP[4] = {
		RookCastleAttack(0, CASTLE_MASKS_ROOK[0]),
		RookCastleAttack(1, CASTLE_MASKS_ROOK[1]),
		RookCastleAttack(2, CASTLE_MASKS_ROOK[2]),
		RookCastleAttack(3, CASTLE_MASKS_ROOK[3])
	};

	static const PawnAttack PAWN_ATTACKS_LOOKUP[2][64] = {
		{
			PawnAttack(PAWN_OFFSETS[0], PAWN_MASKS[0]),
			PawnAttack(PAWN_OFFSETS[1], PAWN_MASKS[1]),
			PawnAttack(PAWN_OFFSETS[2], PAWN_MASKS[2]),
			PawnAttack(PAWN_OFFSETS[3], PAWN_MASKS[3]),
			PawnAttack(PAWN_OFFSETS[4], PAWN_MASKS[4]),
			PawnAttack(PAWN_OFFSETS[5], PAWN_MASKS[5]),
			PawnAttack(PAWN_OFFSETS[6], PAWN_MASKS[6]),
			PawnAttack(PAWN_OFFSETS[7], PAWN_MASKS[7]),
			PawnAttack(PAWN_OFFSETS[8], PAWN_MASKS[8]),
			PawnAttack(PAWN_OFFSETS[9], PAWN_MASKS[9]),
			PawnAttack(PAWN_OFFSETS[10], PAWN_MASKS[10]),
			PawnAttack(PAWN_OFFSETS[11], PAWN_MASKS[11]),
			PawnAttack(PAWN_OFFSETS[12], PAWN_MASKS[12]),
			PawnAttack(PAWN_OFFSETS[13], PAWN_MASKS[13]),
			PawnAttack(PAWN_OFFSETS[14], PAWN_MASKS[14]),
			PawnAttack(PAWN_OFFSETS[15], PAWN_MASKS[15]),
			PawnAttack(PAWN_OFFSETS[16], PAWN_MASKS[16]),
			PawnAttack(PAWN_OFFSETS[17], PAWN_MASKS[17]),
			PawnAttack(PAWN_OFFSETS[18], PAWN_MASKS[18]),
			PawnAttack(PAWN_OFFSETS[19], PAWN_MASKS[19]),
			PawnAttack(PAWN_OFFSETS[20], PAWN_MASKS[20]),
			PawnAttack(PAWN_OFFSETS[21], PAWN_MASKS[21]),
			PawnAttack(PAWN_OFFSETS[22], PAWN_MASKS[22]),
			PawnAttack(PAWN_OFFSETS[23], PAWN_MASKS[23]),
			PawnAttack(PAWN_OFFSETS[24], PAWN_MASKS[24]),
			PawnAttack(PAWN_OFFSETS[25], PAWN_MASKS[25]),
			PawnAttack(PAWN_OFFSETS[26], PAWN_MASKS[26]),
			PawnAttack(PAWN_OFFSETS[27], PAWN_MASKS[27]),
			PawnAttack(PAWN_OFFSETS[28], PAWN_MASKS[28]),
			PawnAttack(PAWN_OFFSETS[29], PAWN_MASKS[29]),
			PawnAttack(PAWN_OFFSETS[30], PAWN_MASKS[30]),
			PawnAttack(PAWN_OFFSETS[31], PAWN_MASKS[31]),
			PawnAttack(PAWN_OFFSETS[32], PAWN_MASKS[32]),
			PawnAttack(PAWN_OFFSETS[33], PAWN_MASKS[33]),
			PawnAttack(PAWN_OFFSETS[34], PAWN_MASKS[34]),
			PawnAttack(PAWN_OFFSETS[35], PAWN_MASKS[35]),
			PawnAttack(PAWN_OFFSETS[36], PAWN_MASKS[36]),
			PawnAttack(PAWN_OFFSETS[37], PAWN_MASKS[37]),
			PawnAttack(PAWN_OFFSETS[38], PAWN_MASKS[38]),
			PawnAttack(PAWN_OFFSETS[39], PAWN_MASKS[39]),
			PawnAttack(PAWN_OFFSETS[40], PAWN_MASKS[40]),
			PawnAttack(PAWN_OFFSETS[41], PAWN_MASKS[41]),
			PawnAttack(PAWN_OFFSETS[42], PAWN_MASKS[42]),
			PawnAttack(PAWN_OFFSETS[43], PAWN_MASKS[43]),
			PawnAttack(PAWN_OFFSETS[44], PAWN_MASKS[44]),
			PawnAttack(PAWN_OFFSETS[45], PAWN_MASKS[45]),
			PawnAttack(PAWN_OFFSETS[46], PAWN_MASKS[46]),
			PawnAttack(PAWN_OFFSETS[47], PAWN_MASKS[47]),
			PawnAttack(PAWN_OFFSETS[48], PAWN_MASKS[48]),
			PawnAttack(PAWN_OFFSETS[49], PAWN_MASKS[49]),
			PawnAttack(PAWN_OFFSETS[50], PAWN_MASKS[50]),
			PawnAttack(PAWN_OFFSETS[51], PAWN_MASKS[51]),
			PawnAttack(PAWN_OFFSETS[52], PAWN_MASKS[52]),
			PawnAttack(PAWN_OFFSETS[53], PAWN_MASKS[53]),
			PawnAttack(PAWN_OFFSETS[54], PAWN_MASKS[54]),
			PawnAttack(PAWN_OFFSETS[55], PAWN_MASKS[55]),
			PawnAttack(PAWN_OFFSETS[56], PAWN_MASKS[56]),
			PawnAttack(PAWN_OFFSETS[57], PAWN_MASKS[57]),
			PawnAttack(PAWN_OFFSETS[58], PAWN_MASKS[58]),
			PawnAttack(PAWN_OFFSETS[59], PAWN_MASKS[59]),
			PawnAttack(PAWN_OFFSETS[60], PAWN_MASKS[60]),
			PawnAttack(PAWN_OFFSETS[61], PAWN_MASKS[61]),
			PawnAttack(PAWN_OFFSETS[62], PAWN_MASKS[62]),
			PawnAttack(PAWN_OFFSETS[63], PAWN_MASKS[63])
		},
		{
			PawnAttack(PAWN_OFFSETS[64], PAWN_MASKS[64]),
			PawnAttack(PAWN_OFFSETS[65], PAWN_MASKS[65]),
			PawnAttack(PAWN_OFFSETS[66], PAWN_MASKS[66]),
			PawnAttack(PAWN_OFFSETS[67], PAWN_MASKS[67]),
			PawnAttack(PAWN_OFFSETS[68], PAWN_MASKS[68]),
			PawnAttack(PAWN_OFFSETS[69], PAWN_MASKS[69]),
			PawnAttack(PAWN_OFFSETS[70], PAWN_MASKS[70]),
			PawnAttack(PAWN_OFFSETS[71], PAWN_MASKS[71]),
			PawnAttack(PAWN_OFFSETS[72], PAWN_MASKS[72]),
			PawnAttack(PAWN_OFFSETS[73], PAWN_MASKS[73]),
			PawnAttack(PAWN_OFFSETS[74], PAWN_MASKS[74]),
			PawnAttack(PAWN_OFFSETS[75], PAWN_MASKS[75]),
			PawnAttack(PAWN_OFFSETS[76], PAWN_MASKS[76]),
			PawnAttack(PAWN_OFFSETS[77], PAWN_MASKS[77]),
			PawnAttack(PAWN_OFFSETS[78], PAWN_MASKS[78]),
			PawnAttack(PAWN_OFFSETS[79], PAWN_MASKS[79]),
			PawnAttack(PAWN_OFFSETS[80], PAWN_MASKS[80]),
			PawnAttack(PAWN_OFFSETS[81], PAWN_MASKS[81]),
			PawnAttack(PAWN_OFFSETS[82], PAWN_MASKS[82]),
			PawnAttack(PAWN_OFFSETS[83], PAWN_MASKS[83]),
			PawnAttack(PAWN_OFFSETS[84], PAWN_MASKS[84]),
			PawnAttack(PAWN_OFFSETS[85], PAWN_MASKS[85]),
			PawnAttack(PAWN_OFFSETS[86], PAWN_MASKS[86]),
			PawnAttack(PAWN_OFFSETS[87], PAWN_MASKS[87]),
			PawnAttack(PAWN_OFFSETS[88], PAWN_MASKS[88]),
			PawnAttack(PAWN_OFFSETS[89], PAWN_MASKS[89]),
			PawnAttack(PAWN_OFFSETS[90], PAWN_MASKS[90]),
			PawnAttack(PAWN_OFFSETS[91], PAWN_MASKS[91]),
			PawnAttack(PAWN_OFFSETS[92], PAWN_MASKS[92]),
			PawnAttack(PAWN_OFFSETS[93], PAWN_MASKS[93]),
			PawnAttack(PAWN_OFFSETS[94], PAWN_MASKS[94]),
			PawnAttack(PAWN_OFFSETS[95], PAWN_MASKS[95]),
			PawnAttack(PAWN_OFFSETS[96], PAWN_MASKS[96]),
			PawnAttack(PAWN_OFFSETS[97], PAWN_MASKS[97]),
			PawnAttack(PAWN_OFFSETS[98], PAWN_MASKS[98]),
			PawnAttack(PAWN_OFFSETS[99], PAWN_MASKS[99]),
			PawnAttack(PAWN_OFFSETS[100], PAWN_MASKS[100]),
			PawnAttack(PAWN_OFFSETS[101], PAWN_MASKS[101]),
			PawnAttack(PAWN_OFFSETS[102], PAWN_MASKS[102]),
			PawnAttack(PAWN_OFFSETS[103], PAWN_MASKS[103]),
			PawnAttack(PAWN_OFFSETS[104], PAWN_MASKS[104]),
			PawnAttack(PAWN_OFFSETS[105], PAWN_MASKS[105]),
			PawnAttack(PAWN_OFFSETS[106], PAWN_MASKS[106]),
			PawnAttack(PAWN_OFFSETS[107], PAWN_MASKS[107]),
			PawnAttack(PAWN_OFFSETS[108], PAWN_MASKS[108]),
			PawnAttack(PAWN_OFFSETS[109], PAWN_MASKS[109]),
			PawnAttack(PAWN_OFFSETS[110], PAWN_MASKS[110]),
			PawnAttack(PAWN_OFFSETS[111], PAWN_MASKS[111]),
			PawnAttack(PAWN_OFFSETS[112], PAWN_MASKS[112]),
			PawnAttack(PAWN_OFFSETS[113], PAWN_MASKS[113]),
			PawnAttack(PAWN_OFFSETS[114], PAWN_MASKS[114]),
			PawnAttack(PAWN_OFFSETS[115], PAWN_MASKS[115]),
			PawnAttack(PAWN_OFFSETS[116], PAWN_MASKS[116]),
			PawnAttack(PAWN_OFFSETS[117], PAWN_MASKS[117]),
			PawnAttack(PAWN_OFFSETS[118], PAWN_MASKS[118]),
			PawnAttack(PAWN_OFFSETS[119], PAWN_MASKS[119]),
			PawnAttack(PAWN_OFFSETS[120], PAWN_MASKS[120]),
			PawnAttack(PAWN_OFFSETS[121], PAWN_MASKS[121]),
			PawnAttack(PAWN_OFFSETS[122], PAWN_MASKS[122]),
			PawnAttack(PAWN_OFFSETS[123], PAWN_MASKS[123]),
			PawnAttack(PAWN_OFFSETS[124], PAWN_MASKS[124]),
			PawnAttack(PAWN_OFFSETS[125], PAWN_MASKS[125]),
			PawnAttack(PAWN_OFFSETS[126], PAWN_MASKS[126]),
			PawnAttack(PAWN_OFFSETS[127], PAWN_MASKS[127])
		}
	};

	static const PawnAttackCount PAWN_ATTACKS_COUNT_LOOKUP[2][64] = {
		{
			PawnAttackCount(PAWN_OFFSETS[0], PAWN_MASKS[0]),
			PawnAttackCount(PAWN_OFFSETS[1], PAWN_MASKS[1]),
			PawnAttackCount(PAWN_OFFSETS[2], PAWN_MASKS[2]),
			PawnAttackCount(PAWN_OFFSETS[3], PAWN_MASKS[3]),
			PawnAttackCount(PAWN_OFFSETS[4], PAWN_MASKS[4]),
			PawnAttackCount(PAWN_OFFSETS[5], PAWN_MASKS[5]),
			PawnAttackCount(PAWN_OFFSETS[6], PAWN_MASKS[6]),
			PawnAttackCount(PAWN_OFFSETS[7], PAWN_MASKS[7]),
			PawnAttackCount(PAWN_OFFSETS[8], PAWN_MASKS[8]),
			PawnAttackCount(PAWN_OFFSETS[9], PAWN_MASKS[9]),
			PawnAttackCount(PAWN_OFFSETS[10], PAWN_MASKS[10]),
			PawnAttackCount(PAWN_OFFSETS[11], PAWN_MASKS[11]),
			PawnAttackCount(PAWN_OFFSETS[12], PAWN_MASKS[12]),
			PawnAttackCount(PAWN_OFFSETS[13], PAWN_MASKS[13]),
			PawnAttackCount(PAWN_OFFSETS[14], PAWN_MASKS[14]),
			PawnAttackCount(PAWN_OFFSETS[15], PAWN_MASKS[15]),
			PawnAttackCount(PAWN_OFFSETS[16], PAWN_MASKS[16]),
			PawnAttackCount(PAWN_OFFSETS[17], PAWN_MASKS[17]),
			PawnAttackCount(PAWN_OFFSETS[18], PAWN_MASKS[18]),
			PawnAttackCount(PAWN_OFFSETS[19], PAWN_MASKS[19]),
			PawnAttackCount(PAWN_OFFSETS[20], PAWN_MASKS[20]),
			PawnAttackCount(PAWN_OFFSETS[21], PAWN_MASKS[21]),
			PawnAttackCount(PAWN_OFFSETS[22], PAWN_MASKS[22]),
			PawnAttackCount(PAWN_OFFSETS[23], PAWN_MASKS[23]),
			PawnAttackCount(PAWN_OFFSETS[24], PAWN_MASKS[24]),
			PawnAttackCount(PAWN_OFFSETS[25], PAWN_MASKS[25]),
			PawnAttackCount(PAWN_OFFSETS[26], PAWN_MASKS[26]),
			PawnAttackCount(PAWN_OFFSETS[27], PAWN_MASKS[27]),
			PawnAttackCount(PAWN_OFFSETS[28], PAWN_MASKS[28]),
			PawnAttackCount(PAWN_OFFSETS[29], PAWN_MASKS[29]),
			PawnAttackCount(PAWN_OFFSETS[30], PAWN_MASKS[30]),
			PawnAttackCount(PAWN_OFFSETS[31], PAWN_MASKS[31]),
			PawnAttackCount(PAWN_OFFSETS[32], PAWN_MASKS[32]),
			PawnAttackCount(PAWN_OFFSETS[33], PAWN_MASKS[33]),
			PawnAttackCount(PAWN_OFFSETS[34], PAWN_MASKS[34]),
			PawnAttackCount(PAWN_OFFSETS[35], PAWN_MASKS[35]),
			PawnAttackCount(PAWN_OFFSETS[36], PAWN_MASKS[36]),
			PawnAttackCount(PAWN_OFFSETS[37], PAWN_MASKS[37]),
			PawnAttackCount(PAWN_OFFSETS[38], PAWN_MASKS[38]),
			PawnAttackCount(PAWN_OFFSETS[39], PAWN_MASKS[39]),
			PawnAttackCount(PAWN_OFFSETS[40], PAWN_MASKS[40]),
			PawnAttackCount(PAWN_OFFSETS[41], PAWN_MASKS[41]),
			PawnAttackCount(PAWN_OFFSETS[42], PAWN_MASKS[42]),
			PawnAttackCount(PAWN_OFFSETS[43], PAWN_MASKS[43]),
			PawnAttackCount(PAWN_OFFSETS[44], PAWN_MASKS[44]),
			PawnAttackCount(PAWN_OFFSETS[45], PAWN_MASKS[45]),
			PawnAttackCount(PAWN_OFFSETS[46], PAWN_MASKS[46]),
			PawnAttackCount(PAWN_OFFSETS[47], PAWN_MASKS[47]),
			PawnAttackCount(PAWN_OFFSETS[48], PAWN_MASKS[48]),
			PawnAttackCount(PAWN_OFFSETS[49], PAWN_MASKS[49]),
			PawnAttackCount(PAWN_OFFSETS[50], PAWN_MASKS[50]),
			PawnAttackCount(PAWN_OFFSETS[51], PAWN_MASKS[51]),
			PawnAttackCount(PAWN_OFFSETS[52], PAWN_MASKS[52]),
			PawnAttackCount(PAWN_OFFSETS[53], PAWN_MASKS[53]),
			PawnAttackCount(PAWN_OFFSETS[54], PAWN_MASKS[54]),
			PawnAttackCount(PAWN_OFFSETS[55], PAWN_MASKS[55]),
			PawnAttackCount(PAWN_OFFSETS[56], PAWN_MASKS[56]),
			PawnAttackCount(PAWN_OFFSETS[57], PAWN_MASKS[57]),
			PawnAttackCount(PAWN_OFFSETS[58], PAWN_MASKS[58]),
			PawnAttackCount(PAWN_OFFSETS[59], PAWN_MASKS[59]),
			PawnAttackCount(PAWN_OFFSETS[60], PAWN_MASKS[60]),
			PawnAttackCount(PAWN_OFFSETS[61], PAWN_MASKS[61]),
			PawnAttackCount(PAWN_OFFSETS[62], PAWN_MASKS[62]),
			PawnAttackCount(PAWN_OFFSETS[63], PAWN_MASKS[63])
		},
		{
			PawnAttackCount(PAWN_OFFSETS[64], PAWN_MASKS[64]),
			PawnAttackCount(PAWN_OFFSETS[65], PAWN_MASKS[65]),
			PawnAttackCount(PAWN_OFFSETS[66], PAWN_MASKS[66]),
			PawnAttackCount(PAWN_OFFSETS[67], PAWN_MASKS[67]),
			PawnAttackCount(PAWN_OFFSETS[68], PAWN_MASKS[68]),
			PawnAttackCount(PAWN_OFFSETS[69], PAWN_MASKS[69]),
			PawnAttackCount(PAWN_OFFSETS[70], PAWN_MASKS[70]),
			PawnAttackCount(PAWN_OFFSETS[71], PAWN_MASKS[71]),
			PawnAttackCount(PAWN_OFFSETS[72], PAWN_MASKS[72]),
			PawnAttackCount(PAWN_OFFSETS[73], PAWN_MASKS[73]),
			PawnAttackCount(PAWN_OFFSETS[74], PAWN_MASKS[74]),
			PawnAttackCount(PAWN_OFFSETS[75], PAWN_MASKS[75]),
			PawnAttackCount(PAWN_OFFSETS[76], PAWN_MASKS[76]),
			PawnAttackCount(PAWN_OFFSETS[77], PAWN_MASKS[77]),
			PawnAttackCount(PAWN_OFFSETS[78], PAWN_MASKS[78]),
			PawnAttackCount(PAWN_OFFSETS[79], PAWN_MASKS[79]),
			PawnAttackCount(PAWN_OFFSETS[80], PAWN_MASKS[80]),
			PawnAttackCount(PAWN_OFFSETS[81], PAWN_MASKS[81]),
			PawnAttackCount(PAWN_OFFSETS[82], PAWN_MASKS[82]),
			PawnAttackCount(PAWN_OFFSETS[83], PAWN_MASKS[83]),
			PawnAttackCount(PAWN_OFFSETS[84], PAWN_MASKS[84]),
			PawnAttackCount(PAWN_OFFSETS[85], PAWN_MASKS[85]),
			PawnAttackCount(PAWN_OFFSETS[86], PAWN_MASKS[86]),
			PawnAttackCount(PAWN_OFFSETS[87], PAWN_MASKS[87]),
			PawnAttackCount(PAWN_OFFSETS[88], PAWN_MASKS[88]),
			PawnAttackCount(PAWN_OFFSETS[89], PAWN_MASKS[89]),
			PawnAttackCount(PAWN_OFFSETS[90], PAWN_MASKS[90]),
			PawnAttackCount(PAWN_OFFSETS[91], PAWN_MASKS[91]),
			PawnAttackCount(PAWN_OFFSETS[92], PAWN_MASKS[92]),
			PawnAttackCount(PAWN_OFFSETS[93], PAWN_MASKS[93]),
			PawnAttackCount(PAWN_OFFSETS[94], PAWN_MASKS[94]),
			PawnAttackCount(PAWN_OFFSETS[95], PAWN_MASKS[95]),
			PawnAttackCount(PAWN_OFFSETS[96], PAWN_MASKS[96]),
			PawnAttackCount(PAWN_OFFSETS[97], PAWN_MASKS[97]),
			PawnAttackCount(PAWN_OFFSETS[98], PAWN_MASKS[98]),
			PawnAttackCount(PAWN_OFFSETS[99], PAWN_MASKS[99]),
			PawnAttackCount(PAWN_OFFSETS[100], PAWN_MASKS[100]),
			PawnAttackCount(PAWN_OFFSETS[101], PAWN_MASKS[101]),
			PawnAttackCount(PAWN_OFFSETS[102], PAWN_MASKS[102]),
			PawnAttackCount(PAWN_OFFSETS[103], PAWN_MASKS[103]),
			PawnAttackCount(PAWN_OFFSETS[104], PAWN_MASKS[104]),
			PawnAttackCount(PAWN_OFFSETS[105], PAWN_MASKS[105]),
			PawnAttackCount(PAWN_OFFSETS[106], PAWN_MASKS[106]),
			PawnAttackCount(PAWN_OFFSETS[107], PAWN_MASKS[107]),
			PawnAttackCount(PAWN_OFFSETS[108], PAWN_MASKS[108]),
			PawnAttackCount(PAWN_OFFSETS[109], PAWN_MASKS[109]),
			PawnAttackCount(PAWN_OFFSETS[110], PAWN_MASKS[110]),
			PawnAttackCount(PAWN_OFFSETS[111], PAWN_MASKS[111]),
			PawnAttackCount(PAWN_OFFSETS[112], PAWN_MASKS[112]),
			PawnAttackCount(PAWN_OFFSETS[113], PAWN_MASKS[113]),
			PawnAttackCount(PAWN_OFFSETS[114], PAWN_MASKS[114]),
			PawnAttackCount(PAWN_OFFSETS[115], PAWN_MASKS[115]),
			PawnAttackCount(PAWN_OFFSETS[116], PAWN_MASKS[116]),
			PawnAttackCount(PAWN_OFFSETS[117], PAWN_MASKS[117]),
			PawnAttackCount(PAWN_OFFSETS[118], PAWN_MASKS[118]),
			PawnAttackCount(PAWN_OFFSETS[119], PAWN_MASKS[119]),
			PawnAttackCount(PAWN_OFFSETS[120], PAWN_MASKS[120]),
			PawnAttackCount(PAWN_OFFSETS[121], PAWN_MASKS[121]),
			PawnAttackCount(PAWN_OFFSETS[122], PAWN_MASKS[122]),
			PawnAttackCount(PAWN_OFFSETS[123], PAWN_MASKS[123]),
			PawnAttackCount(PAWN_OFFSETS[124], PAWN_MASKS[124]),
			PawnAttackCount(PAWN_OFFSETS[125], PAWN_MASKS[125]),
			PawnAttackCount(PAWN_OFFSETS[126], PAWN_MASKS[126]),
			PawnAttackCount(PAWN_OFFSETS[127], PAWN_MASKS[127])
		}
	};

	static const PawnKingAttack PAWN_KING_ATTACKS_LOOKUP[2][64] = {
		{
			PawnKingAttack(PAWN_KING_MASKS[0], white, 0),
			PawnKingAttack(PAWN_KING_MASKS[1], white, 1),
			PawnKingAttack(PAWN_KING_MASKS[2], white, 2),
			PawnKingAttack(PAWN_KING_MASKS[3], white, 3),
			PawnKingAttack(PAWN_KING_MASKS[4], white, 4),
			PawnKingAttack(PAWN_KING_MASKS[5], white, 5),
			PawnKingAttack(PAWN_KING_MASKS[6], white, 6),
			PawnKingAttack(PAWN_KING_MASKS[7], white, 7),
			PawnKingAttack(PAWN_KING_MASKS[8], white, 8),
			PawnKingAttack(PAWN_KING_MASKS[9], white, 9),
			PawnKingAttack(PAWN_KING_MASKS[10], white, 10),
			PawnKingAttack(PAWN_KING_MASKS[11], white, 11),
			PawnKingAttack(PAWN_KING_MASKS[12], white, 12),
			PawnKingAttack(PAWN_KING_MASKS[13], white, 13),
			PawnKingAttack(PAWN_KING_MASKS[14], white, 14),
			PawnKingAttack(PAWN_KING_MASKS[15], white, 15),
			PawnKingAttack(PAWN_KING_MASKS[16], white, 16),
			PawnKingAttack(PAWN_KING_MASKS[17], white, 17),
			PawnKingAttack(PAWN_KING_MASKS[18], white, 18),
			PawnKingAttack(PAWN_KING_MASKS[19], white, 19),
			PawnKingAttack(PAWN_KING_MASKS[20], white, 20),
			PawnKingAttack(PAWN_KING_MASKS[21], white, 21),
			PawnKingAttack(PAWN_KING_MASKS[22], white, 22),
			PawnKingAttack(PAWN_KING_MASKS[23], white, 23),
			PawnKingAttack(PAWN_KING_MASKS[24], white, 24),
			PawnKingAttack(PAWN_KING_MASKS[25], white, 25),
			PawnKingAttack(PAWN_KING_MASKS[26], white, 26),
			PawnKingAttack(PAWN_KING_MASKS[27], white, 27),
			PawnKingAttack(PAWN_KING_MASKS[28], white, 28),
			PawnKingAttack(PAWN_KING_MASKS[29], white, 29),
			PawnKingAttack(PAWN_KING_MASKS[30], white, 30),
			PawnKingAttack(PAWN_KING_MASKS[31], white, 31),
			PawnKingAttack(PAWN_KING_MASKS[32], white, 32),
			PawnKingAttack(PAWN_KING_MASKS[33], white, 33),
			PawnKingAttack(PAWN_KING_MASKS[34], white, 34),
			PawnKingAttack(PAWN_KING_MASKS[35], white, 35),
			PawnKingAttack(PAWN_KING_MASKS[36], white, 36),
			PawnKingAttack(PAWN_KING_MASKS[37], white, 37),
			PawnKingAttack(PAWN_KING_MASKS[38], white, 38),
			PawnKingAttack(PAWN_KING_MASKS[39], white, 39),
			PawnKingAttack(PAWN_KING_MASKS[40], white, 40),
			PawnKingAttack(PAWN_KING_MASKS[41], white, 41),
			PawnKingAttack(PAWN_KING_MASKS[42], white, 42),
			PawnKingAttack(PAWN_KING_MASKS[43], white, 43),
			PawnKingAttack(PAWN_KING_MASKS[44], white, 44),
			PawnKingAttack(PAWN_KING_MASKS[45], white, 45),
			PawnKingAttack(PAWN_KING_MASKS[46], white, 46),
			PawnKingAttack(PAWN_KING_MASKS[47], white, 47),
			PawnKingAttack(PAWN_KING_MASKS[48], white, 48),
			PawnKingAttack(PAWN_KING_MASKS[49], white, 49),
			PawnKingAttack(PAWN_KING_MASKS[50], white, 50),
			PawnKingAttack(PAWN_KING_MASKS[51], white, 51),
			PawnKingAttack(PAWN_KING_MASKS[52], white, 52),
			PawnKingAttack(PAWN_KING_MASKS[53], white, 53),
			PawnKingAttack(PAWN_KING_MASKS[54], white, 54),
			PawnKingAttack(PAWN_KING_MASKS[55], white, 55),
			PawnKingAttack(PAWN_KING_MASKS[56], white, 56),
			PawnKingAttack(PAWN_KING_MASKS[57], white, 57),
			PawnKingAttack(PAWN_KING_MASKS[58], white, 58),
			PawnKingAttack(PAWN_KING_MASKS[59], white, 59),
			PawnKingAttack(PAWN_KING_MASKS[60], white, 60),
			PawnKingAttack(PAWN_KING_MASKS[61], white, 61),
			PawnKingAttack(PAWN_KING_MASKS[62], white, 62),
			PawnKingAttack(PAWN_KING_MASKS[63], white, 63)
		},
		{
			PawnKingAttack(PAWN_KING_MASKS[64], black, 0),
			PawnKingAttack(PAWN_KING_MASKS[65], black, 1),
			PawnKingAttack(PAWN_KING_MASKS[66], black, 2),
			PawnKingAttack(PAWN_KING_MASKS[67], black, 3),
			PawnKingAttack(PAWN_KING_MASKS[68], black, 4),
			PawnKingAttack(PAWN_KING_MASKS[69], black, 5),
			PawnKingAttack(PAWN_KING_MASKS[70], black, 6),
			PawnKingAttack(PAWN_KING_MASKS[71], black, 7),
			PawnKingAttack(PAWN_KING_MASKS[72], black, 8),
			PawnKingAttack(PAWN_KING_MASKS[73], black, 9),
			PawnKingAttack(PAWN_KING_MASKS[74], black, 10),
			PawnKingAttack(PAWN_KING_MASKS[75], black, 11),
			PawnKingAttack(PAWN_KING_MASKS[76], black, 12),
			PawnKingAttack(PAWN_KING_MASKS[77], black, 13),
			PawnKingAttack(PAWN_KING_MASKS[78], black, 14),
			PawnKingAttack(PAWN_KING_MASKS[79], black, 15),
			PawnKingAttack(PAWN_KING_MASKS[80], black, 16),
			PawnKingAttack(PAWN_KING_MASKS[81], black, 17),
			PawnKingAttack(PAWN_KING_MASKS[82], black, 18),
			PawnKingAttack(PAWN_KING_MASKS[83], black, 19),
			PawnKingAttack(PAWN_KING_MASKS[84], black, 20),
			PawnKingAttack(PAWN_KING_MASKS[85], black, 21),
			PawnKingAttack(PAWN_KING_MASKS[86], black, 22),
			PawnKingAttack(PAWN_KING_MASKS[87], black, 23),
			PawnKingAttack(PAWN_KING_MASKS[88], black, 24),
			PawnKingAttack(PAWN_KING_MASKS[89], black, 25),
			PawnKingAttack(PAWN_KING_MASKS[90], black, 26),
			PawnKingAttack(PAWN_KING_MASKS[91], black, 27),
			PawnKingAttack(PAWN_KING_MASKS[92], black, 28),
			PawnKingAttack(PAWN_KING_MASKS[93], black, 29),
			PawnKingAttack(PAWN_KING_MASKS[94], black, 30),
			PawnKingAttack(PAWN_KING_MASKS[95], black, 31),
			PawnKingAttack(PAWN_KING_MASKS[96], black, 32),
			PawnKingAttack(PAWN_KING_MASKS[97], black, 33),
			PawnKingAttack(PAWN_KING_MASKS[98], black, 34),
			PawnKingAttack(PAWN_KING_MASKS[99], black, 35),
			PawnKingAttack(PAWN_KING_MASKS[100], black, 36),
			PawnKingAttack(PAWN_KING_MASKS[101], black, 37),
			PawnKingAttack(PAWN_KING_MASKS[102], black, 38),
			PawnKingAttack(PAWN_KING_MASKS[103], black, 39),
			PawnKingAttack(PAWN_KING_MASKS[104], black, 40),
			PawnKingAttack(PAWN_KING_MASKS[105], black, 41),
			PawnKingAttack(PAWN_KING_MASKS[106], black, 42),
			PawnKingAttack(PAWN_KING_MASKS[107], black, 43),
			PawnKingAttack(PAWN_KING_MASKS[108], black, 44),
			PawnKingAttack(PAWN_KING_MASKS[109], black, 45),
			PawnKingAttack(PAWN_KING_MASKS[110], black, 46),
			PawnKingAttack(PAWN_KING_MASKS[111], black, 47),
			PawnKingAttack(PAWN_KING_MASKS[112], black, 48),
			PawnKingAttack(PAWN_KING_MASKS[113], black, 49),
			PawnKingAttack(PAWN_KING_MASKS[114], black, 50),
			PawnKingAttack(PAWN_KING_MASKS[115], black, 51),
			PawnKingAttack(PAWN_KING_MASKS[116], black, 52),
			PawnKingAttack(PAWN_KING_MASKS[117], black, 53),
			PawnKingAttack(PAWN_KING_MASKS[118], black, 54),
			PawnKingAttack(PAWN_KING_MASKS[119], black, 55),
			PawnKingAttack(PAWN_KING_MASKS[120], black, 56),
			PawnKingAttack(PAWN_KING_MASKS[121], black, 57),
			PawnKingAttack(PAWN_KING_MASKS[122], black, 58),
			PawnKingAttack(PAWN_KING_MASKS[123], black, 59),
			PawnKingAttack(PAWN_KING_MASKS[124], black, 60),
			PawnKingAttack(PAWN_KING_MASKS[125], black, 61),
			PawnKingAttack(PAWN_KING_MASKS[126], black, 62),
			PawnKingAttack(PAWN_KING_MASKS[127], black, 63)
		}
	};

	static const RookAttackZone ROOK_ATTACK_ZONES_LOOKUP[64] = {
		RookAttackZone(0, KING_ATTACKS[0]),
		RookAttackZone(1, KING_ATTACKS[1]),
		RookAttackZone(2, KING_ATTACKS[2]),
		RookAttackZone(3, KING_ATTACKS[3]),
		RookAttackZone(4, KING_ATTACKS[4]),
		RookAttackZone(5, KING_ATTACKS[5]),
		RookAttackZone(6, KING_ATTACKS[6]),
		RookAttackZone(7, KING_ATTACKS[7]),
		RookAttackZone(8, KING_ATTACKS[8]),
		RookAttackZone(9, KING_ATTACKS[9]),
		RookAttackZone(10, KING_ATTACKS[10]),
		RookAttackZone(11, KING_ATTACKS[11]),
		RookAttackZone(12, KING_ATTACKS[12]),
		RookAttackZone(13, KING_ATTACKS[13]),
		RookAttackZone(14, KING_ATTACKS[14]),
		RookAttackZone(15, KING_ATTACKS[15]),
		RookAttackZone(16, KING_ATTACKS[16]),
		RookAttackZone(17, KING_ATTACKS[17]),
		RookAttackZone(18, KING_ATTACKS[18]),
		RookAttackZone(19, KING_ATTACKS[19]),
		RookAttackZone(20, KING_ATTACKS[20]),
		RookAttackZone(21, KING_ATTACKS[21]),
		RookAttackZone(22, KING_ATTACKS[22]),
		RookAttackZone(23, KING_ATTACKS[23]),
		RookAttackZone(24, KING_ATTACKS[24]),
		RookAttackZone(25, KING_ATTACKS[25]),
		RookAttackZone(26, KING_ATTACKS[26]),
		RookAttackZone(27, KING_ATTACKS[27]),
		RookAttackZone(28, KING_ATTACKS[28]),
		RookAttackZone(29, KING_ATTACKS[29]),
		RookAttackZone(30, KING_ATTACKS[30]),
		RookAttackZone(31, KING_ATTACKS[31]),
		RookAttackZone(32, KING_ATTACKS[32]),
		RookAttackZone(33, KING_ATTACKS[33]),
		RookAttackZone(34, KING_ATTACKS[34]),
		RookAttackZone(35, KING_ATTACKS[35]),
		RookAttackZone(36, KING_ATTACKS[36]),
		RookAttackZone(37, KING_ATTACKS[37]),
		RookAttackZone(38, KING_ATTACKS[38]),
		RookAttackZone(39, KING_ATTACKS[39]),
		RookAttackZone(40, KING_ATTACKS[40]),
		RookAttackZone(41, KING_ATTACKS[41]),
		RookAttackZone(42, KING_ATTACKS[42]),
		RookAttackZone(43, KING_ATTACKS[43]),
		RookAttackZone(44, KING_ATTACKS[44]),
		RookAttackZone(45, KING_ATTACKS[45]),
		RookAttackZone(46, KING_ATTACKS[46]),
		RookAttackZone(47, KING_ATTACKS[47]),
		RookAttackZone(48, KING_ATTACKS[48]),
		RookAttackZone(49, KING_ATTACKS[49]),
		RookAttackZone(50, KING_ATTACKS[50]),
		RookAttackZone(51, KING_ATTACKS[51]),
		RookAttackZone(52, KING_ATTACKS[52]),
		RookAttackZone(53, KING_ATTACKS[53]),
		RookAttackZone(54, KING_ATTACKS[54]),
		RookAttackZone(55, KING_ATTACKS[55]),
		RookAttackZone(56, KING_ATTACKS[56]),
		RookAttackZone(57, KING_ATTACKS[57]),
		RookAttackZone(58, KING_ATTACKS[58]),
		RookAttackZone(59, KING_ATTACKS[59]),
		RookAttackZone(60, KING_ATTACKS[60]),
		RookAttackZone(61, KING_ATTACKS[61]),
		RookAttackZone(62, KING_ATTACKS[62]),
		RookAttackZone(63, KING_ATTACKS[63])
	};

	static const BishopAttackZone BISHOP_ATTACK_ZONES_LOOKUP[64] = {
		BishopAttackZone(0, KING_ATTACKS[0]),
		BishopAttackZone(1, KING_ATTACKS[1]),
		BishopAttackZone(2, KING_ATTACKS[2]),
		BishopAttackZone(3, KING_ATTACKS[3]),
		BishopAttackZone(4, KING_ATTACKS[4]),
		BishopAttackZone(5, KING_ATTACKS[5]),
		BishopAttackZone(6, KING_ATTACKS[6]),
		BishopAttackZone(7, KING_ATTACKS[7]),
		BishopAttackZone(8, KING_ATTACKS[8]),
		BishopAttackZone(9, KING_ATTACKS[9]),
		BishopAttackZone(10, KING_ATTACKS[10]),
		BishopAttackZone(11, KING_ATTACKS[11]),
		BishopAttackZone(12, KING_ATTACKS[12]),
		BishopAttackZone(13, KING_ATTACKS[13]),
		BishopAttackZone(14, KING_ATTACKS[14]),
		BishopAttackZone(15, KING_ATTACKS[15]),
		BishopAttackZone(16, KING_ATTACKS[16]),
		BishopAttackZone(17, KING_ATTACKS[17]),
		BishopAttackZone(18, KING_ATTACKS[18]),
		BishopAttackZone(19, KING_ATTACKS[19]),
		BishopAttackZone(20, KING_ATTACKS[20]),
		BishopAttackZone(21, KING_ATTACKS[21]),
		BishopAttackZone(22, KING_ATTACKS[22]),
		BishopAttackZone(23, KING_ATTACKS[23]),
		BishopAttackZone(24, KING_ATTACKS[24]),
		BishopAttackZone(25, KING_ATTACKS[25]),
		BishopAttackZone(26, KING_ATTACKS[26]),
		BishopAttackZone(27, KING_ATTACKS[27]),
		BishopAttackZone(28, KING_ATTACKS[28]),
		BishopAttackZone(29, KING_ATTACKS[29]),
		BishopAttackZone(30, KING_ATTACKS[30]),
		BishopAttackZone(31, KING_ATTACKS[31]),
		BishopAttackZone(32, KING_ATTACKS[32]),
		BishopAttackZone(33, KING_ATTACKS[33]),
		BishopAttackZone(34, KING_ATTACKS[34]),
		BishopAttackZone(35, KING_ATTACKS[35]),
		BishopAttackZone(36, KING_ATTACKS[36]),
		BishopAttackZone(37, KING_ATTACKS[37]),
		BishopAttackZone(38, KING_ATTACKS[38]),
		BishopAttackZone(39, KING_ATTACKS[39]),
		BishopAttackZone(40, KING_ATTACKS[40]),
		BishopAttackZone(41, KING_ATTACKS[41]),
		BishopAttackZone(42, KING_ATTACKS[42]),
		BishopAttackZone(43, KING_ATTACKS[43]),
		BishopAttackZone(44, KING_ATTACKS[44]),
		BishopAttackZone(45, KING_ATTACKS[45]),
		BishopAttackZone(46, KING_ATTACKS[46]),
		BishopAttackZone(47, KING_ATTACKS[47]),
		BishopAttackZone(48, KING_ATTACKS[48]),
		BishopAttackZone(49, KING_ATTACKS[49]),
		BishopAttackZone(50, KING_ATTACKS[50]),
		BishopAttackZone(51, KING_ATTACKS[51]),
		BishopAttackZone(52, KING_ATTACKS[52]),
		BishopAttackZone(53, KING_ATTACKS[53]),
		BishopAttackZone(54, KING_ATTACKS[54]),
		BishopAttackZone(55, KING_ATTACKS[55]),
		BishopAttackZone(56, KING_ATTACKS[56]),
		BishopAttackZone(57, KING_ATTACKS[57]),
		BishopAttackZone(58, KING_ATTACKS[58]),
		BishopAttackZone(59, KING_ATTACKS[59]),
		BishopAttackZone(60, KING_ATTACKS[60]),
		BishopAttackZone(61, KING_ATTACKS[61]),
		BishopAttackZone(62, KING_ATTACKS[62]),
		BishopAttackZone(63, KING_ATTACKS[63])
	};

}