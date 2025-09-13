#include "MoveArray.h"
#include <algorithm>

namespace movarray {
	MoveArray movesArray;

	MoveArray::MoveArray() :
		_size{ 0 }
	{
		_moves = new moveinfo::MoveInfo[120];
	}

	MoveArray::~MoveArray()
	{
		//delete[] _moves;
		_size = 0;
	}

	void MoveArray::add(moveinfo::MoveInfo& moveInfo) {
		_moves[_size++] = moveInfo;
	}
}