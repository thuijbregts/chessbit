#include "MoveArray.h"
#include <algorithm>

namespace movarray {
	MoveArray movesArray = movesArrayPool[0];

	MoveArray::MoveArray() :
		_size{ 0 }
	{
		_moves = new moveinfo::MoveInfo[256];
	}

	MoveArray::~MoveArray()
	{
		//delete[] _moves;
		_size = 0;
	}
}