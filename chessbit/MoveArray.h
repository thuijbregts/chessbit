#ifndef MOVEARRAY_H
#define MOVEARRAY_H

#include "MoveInfo.h"

namespace movarray {
	class MoveArray {

	public:
		MoveArray();
		~MoveArray();

		inline int size() {
			return _size;
		}

		inline moveinfo::MoveInfo* moves() {
			return _moves;
		}
		inline void reset() {
			_size = 0;
		}

		void add(moveinfo::MoveInfo& moveInfo);

	private:
		moveinfo::MoveInfo* _moves;
		int _size;
	};

	extern MoveArray movesArray;
}

#endif