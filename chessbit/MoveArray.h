#ifndef MOVEARRAY_H
#define MOVEARRAY_H

#include "MoveInfo.h"

namespace movarray {
	class MoveArray {

	public:
		static constexpr int MAX_MOVES = 256;

		MoveArray()
			: _moves(new moveinfo::MoveInfo[MAX_MOVES]), _size(0)
		{
		}

		~MoveArray() {
			delete[] _moves;
		}

		MoveArray(const MoveArray& other)
			: _moves(new moveinfo::MoveInfo[MAX_MOVES]),
			_size(other._size)
		{
			std::copy(other._moves, other._moves + other._size, _moves);
		}

		MoveArray& operator=(const MoveArray& other)
		{
			if (this != &other) {
				_size = other._size;
				std::copy(other._moves, other._moves + other._size, _moves);
			}
			return *this;
		}

		__forceinline  int size() {
			return _size;
		}

		__forceinline  moveinfo::MoveInfo* moves() {
			return _moves;
		}
		__forceinline  void reset() {
			_size = 0;
		}

		__forceinline void add(const moveinfo::MoveInfo& moveInfo) {
			_moves[_size++] = moveInfo;
		}

	private:
		moveinfo::MoveInfo* _moves;
		int _size;
	};

	inline MoveArray movesArray;
}

#endif