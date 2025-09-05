#ifndef MOVEARRAY_H
#define MOVEARRAY_H

#include "Game.h"

using namespace game;

class MoveArray {

public:
	MoveArray();
	~MoveArray();

	inline int size() {
		return _size;
	}

	inline MoveInfo** moves() {
		return _moves;
	}
	inline void reset() {
		_size = 0;
	}

	void pawn(int from, int to, int victimeType);
	void knight(int from, int to, int victimeType);
	void bishop(int from, int to, int victimeType);
	void rook(int from, int to, int victimeType);
	void queen(int from, int to, int victimeType);
	void king(int from, int to, int victimeType);
	void enPassant(int from, int to);
	void castling(int to);
	void promotion(int from, int to, int victimeType);

	void sort();

private:
	MoveInfo** _moves;
	int _size;
};

#endif