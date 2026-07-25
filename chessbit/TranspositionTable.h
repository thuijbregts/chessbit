#pragma once
#include "Definitions.h"
#include <atomic>
#include <cstring>
#include <cstdlib>

#if defined(_WIN32)
#include <malloc.h>
#include <intrin.h>
#endif

using namespace defs;

namespace tt {
    constexpr int MIN_HASH_DEPTH = 2;
    constexpr int MAX_HASH_DEPTH = 14;

    constexpr int MAX_ENTRIES = 4;
    constexpr int BUCKET_SIZE = MAX_ENTRIES * 16;

    template <int depth>
    constexpr bool USE_HASH = (depth >= MIN_HASH_DEPTH && depth <= MAX_HASH_DEPTH);

    struct alignas(BUCKET_SIZE) Bucket {
        U64 key[MAX_ENTRIES];
        U64 data[MAX_ENTRIES];
    };

    inline Bucket* TABLE = nullptr;
    inline U64     MASK = 0;
    inline U64     BYTES = 0;

    __forceinline static U64 index(Zobrist z, int depth) {
        return (z.low + static_cast<U64>(depth)) & MASK;
    }

    template <int depth>
    ForceInline bool probe(Zobrist z, U64& nodes) {
        const Bucket& b = TABLE[index(z, depth)];
        for (int i = 0; i < MAX_ENTRIES; ++i) {
            const U64 d = b.data[i];
            if (b.key[i] == (z.high ^ d) && (d & 63ULL) == static_cast<U64>(depth)) {
                nodes = d >> 6;
                return true;
            }
        }
        return false;
    }

    template <int depth>
    ForceInline void write(Zobrist z, U64 nodes) {
        Bucket& b = TABLE[index(z, depth)];
        const U64 data = (nodes << 6) | static_cast<U64>(depth);
        int v = 0;
        for (int i = 0; i < MAX_ENTRIES; ++i) {
            if (b.key[i] == (z.high ^ b.data[i]) && (b.data[i] & 63ULL) == static_cast<U64>(depth)) {
                return;
            }
            if (b.data[i] < b.data[v]) v = i;
        }
        b.key[v] = z.high ^ data;
        b.data[v] = data;
    }

    template <int depth>
    ForceInline void prefetch(Zobrist z) {
        if constexpr (USE_HASH<depth>) {
            _mm_prefetch(reinterpret_cast<const char*>(&TABLE[index(z, depth)]), _MM_HINT_T0);
        }
    }

    inline void free() {
        if (TABLE) {
#if defined(_WIN32)
            _aligned_free(TABLE);
#else
            std::free(TABLE);
#endif
            TABLE = nullptr;
            MASK = 0;
            BYTES = 0;
        }
    }

    inline void clear() {
        if (TABLE) std::memset(TABLE, 0, BYTES);
    }

    inline void init(size_t megabytes = 4096) {
        free();

        size_t n = (megabytes << 20) / sizeof(Bucket);
        if (n < 1) n = 1;

#if defined(_MSC_VER)
        n = 1ULL << (63 - _lzcnt_u64(n));
#else
        n = 1ULL << (63 - __builtin_clzll(n));
#endif

        BYTES = n * sizeof(Bucket);
        MASK = n - 1;

#if defined(_WIN32)
        TABLE = static_cast<Bucket*>(_aligned_malloc(BYTES, 64));
#else
        if (posix_memalign(reinterpret_cast<void**>(&TABLE), 64, BYTES) != 0) TABLE = nullptr;
#endif
        clear();
    }
}