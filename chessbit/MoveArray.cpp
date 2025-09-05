#include "MoveGenerator.h"
#include <algorithm>

MoveArray::MoveArray() :
	_moves{ new MoveInfo* [120] },
	_size{ 0 }
{
}

MoveArray::~MoveArray()
{
	//delete[] _moves;
	_size = 0;
}

namespace {
	bool compareTo(MoveInfo const* m1, MoveInfo const* m2) {
		return m1->mvv_lva > m2->mvv_lva;
	}
}

void MoveArray::pawn(int from, int to, int victimeType) {
	_moves[_size++] = &PAWN_MOVES[victimeType][from][to];
}

void MoveArray::knight(int from, int to, int victimeType) {
	_moves[_size++] = &KNIGHT_MOVES[victimeType][from][to];
}

void MoveArray::bishop(int from, int to, int victimeType) {
	_moves[_size++] = &BISHOP_MOVES[victimeType][from][to];
}

void MoveArray::rook(int from, int to, int victimeType) {
	_moves[_size++] = &ROOK_MOVES[victimeType][from][to];
}

void MoveArray::queen(int from, int to, int victimeType) {
	_moves[_size++] = &QUEEN_MOVES[victimeType][from][to];
}

void MoveArray::king(int from, int to, int victimeType) {
	_moves[_size++] = &KING_MOVES[victimeType][from][to];
}

void MoveArray::enPassant(int from, int to) {
	_moves[_size++] = &EN_PASSANT_MOVES[from][to];
}

void MoveArray::castling(int to) {
	_moves[_size++] = &CASTLING_MOVES[to];
}

void MoveArray::promotion(int from, int to, int victimeType) {
	_moves[_size++] = &PROMO_KNIGHT_MOVES[victimeType][from][to];
	_moves[_size++] = &PROMO_BISHOP_MOVES[victimeType][from][to];
	_moves[_size++] = &PROMO_ROOK_MOVES[victimeType][from][to];
	_moves[_size++] = &PROMO_QUEEN_MOVES[victimeType][from][to];
}

void MoveArray::sort() {
	std::sort(_moves, _moves + _size, compareTo);
}