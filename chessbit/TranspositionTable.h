#pragma once
#include "Definitions.h"
#include <utility>
#include <type_traits>

using namespace defs;

namespace tt {

	constexpr int SIZE[MAX_DEPTH] = {
		//   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17
			 0,  0,  0, 30, 29, 28, 27, 26, 23, 22, 21, 20, 19, 18, 17,  0,  0,  0
	};

	template <int depth>
	constexpr U64 MASK = (1ULL << SIZE[depth]) - 1;

	constexpr int NODES_BITS(int depth) {
		return (depth == 2) ? 16 : (depth == 3) ? 24 : 0;
	}

	template <int depth>
	constexpr bool COMPACT = NODES_BITS(depth) != 0;

	template <int depth>
	constexpr U64 NODES_MASK = (1ULL << NODES_BITS(depth)) - 1;

	struct alignas(16) Entry { U64 key; U64 nodes; };

	template <int depth>
	using EntryType = std::conditional_t<COMPACT<depth>, U64, Entry>;

	inline void* TT[MAX_DEPTH];

	template <int depth>
	ForceInline EntryType<depth>* table() {
		return static_cast<EntryType<depth>*>(TT[depth]);
	}

	template <int depth>
	ForceInline bool probe(Zobrist zobrist, U64& nodes) {
		if constexpr (COMPACT<depth>) {
			const U64 e = table<depth>()[zobrist.low & MASK<depth>];
			if ((e ^ zobrist.high) <= NODES_MASK<depth>) {
				nodes = e & NODES_MASK<depth>;
				return true;
			}
		}
		else {
			const Entry& e = table<depth>()[zobrist.low & MASK<depth>];
			if ((e.key ^ e.nodes) == zobrist.high) {
				nodes = e.nodes;
				return true;
			}
		}
		return false;
	}

	template <int depth>
	ForceInline void write(Zobrist zobrist, U64 nodes) {
		if constexpr (COMPACT<depth>) {
			table<depth>()[zobrist.low & MASK<depth>] = (zobrist.high & ~NODES_MASK<depth>) | nodes;
		}
		else {
			Entry& e = table<depth>()[zobrist.low & MASK<depth>];
			e.key = zobrist.high ^ nodes;
			e.nodes = nodes;
		}
	}

	template <int depth>
	ForceInline void prefetch(Zobrist zobrist) {
		if constexpr (SIZE[depth] != 0) {
			_mm_prefetch(reinterpret_cast<const char*>(&table<depth>()[zobrist.low & MASK<depth>]), _MM_HINT_T2);
		}
	}

	template <int depth>
	inline void initTable() {
		if constexpr (SIZE[depth] != 0)
			TT[depth] = new EntryType<depth>[1ULL << SIZE[depth]];
	}

	template <int... depths>
	inline void initTables(std::integer_sequence<int, depths...>) {
		(initTable<depths>(), ...);
	}

	static inline void init() {
		initTables(std::make_integer_sequence<int, MAX_DEPTH>{});
	}
}