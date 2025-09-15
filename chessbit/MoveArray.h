#ifndef MOVEARRAY_H
#define MOVEARRAY_H

#include "MoveInfo.h"

namespace movarray {
	class MoveArray {

	public:
		MoveArray();
		~MoveArray();

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

	inline static MoveArray movesArrayPool[50];
	extern MoveArray movesArray;
}

#endif