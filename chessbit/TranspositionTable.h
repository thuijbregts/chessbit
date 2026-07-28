#pragma once
#include "Definitions.h"
#include "Utils.h"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <new>
#include <bit>

using namespace defs;

namespace tt {
    constexpr int MIN_HASH_DEPTH = 2;
    constexpr int MAX_HASH_DEPTH = 14;

    constexpr int MAX_ENTRIES = 4;
    constexpr int BUCKET_SIZE = MAX_ENTRIES * 16;

    constexpr U64 DEPTH_BITS = 4;
    constexpr U64 DEPTH_MASK = (1ULL << DEPTH_BITS) - 1;
    constexpr U64 FREQ_SHIFT = DEPTH_BITS;
    constexpr U64 FREQ_BITS = 2;
    constexpr U64 FREQ_MASK = ((1ULL << FREQ_BITS) - 1) << FREQ_SHIFT;
    constexpr U64 FREQ_ONE = 1ULL << FREQ_SHIFT;
    constexpr U64 FREQ_MAX = FREQ_MASK;                        
    constexpr U64 META_BITS = 6;                                 
    constexpr U64 COUNT_SHIFT = META_BITS;

    template <int depth>
    constexpr bool USE_HASH = (depth >= MIN_HASH_DEPTH && depth <= MAX_HASH_DEPTH);

    struct alignas(BUCKET_SIZE) Bucket {
        U64 key[MAX_ENTRIES];
        U64 data[MAX_ENTRIES];
    };

    inline Bucket* TABLE = nullptr;
    inline U64     MASK = 0;
    inline U64     BYTES = 0;

    __forceinline static U64 index(Zobrist z, int depth) noexcept {
        return (z.low + static_cast<U64>(depth)) & MASK;
    }

    template <int depth>
    ForceInline bool probe(Zobrist z, U64& nodes) noexcept {
        const Bucket& b = TABLE[index(z, depth)];

        for (int i = 0; i < MAX_ENTRIES; ++i) {
            const U64 d = b.data[i];
            if (b.key[i] == (z.high ^ d) && (d & DEPTH_MASK) == static_cast<U64>(depth)) {
                nodes = d >> COUNT_SHIFT;
                return true;
            }
        }
        return false;
    }

    ForceInline U64 score(U64 data) noexcept {
        const U64 count = data >> COUNT_SHIFT;
        const U64 freq = (data & FREQ_MASK) >> FREQ_SHIFT;

        return count + count * freq;
    }

    template <int depth>
    ForceInline void write(Zobrist z, U64 nodes) noexcept {
        Bucket& b = TABLE[index(z, depth)];

        int v = 0;
        U64 minScore = ~0ULL;
        for (int i = 0; i < MAX_ENTRIES; ++i) {
            const U64 di = b.data[i];

            if (b.key[i] == (z.high ^ di) && (di & DEPTH_MASK) == static_cast<U64>(depth)) {
                if ((di & FREQ_MASK) != FREQ_MAX) {
                    const U64 nd = di + FREQ_ONE;
                    b.key[i] = z.high ^ nd;
                    b.data[i] = nd;
                }
                return;
            }

            const U64 sc = score(di);
            if (sc < minScore) { minScore = sc; v = i; }
        }

        const U64 data = (nodes << COUNT_SHIFT) | static_cast<U64>(depth);

        b.key[v] = z.high ^ data;
        b.data[v] = data;
    }

    template <int depth>
    ForceInline void prefetch(Zobrist z) noexcept {
        if constexpr (USE_HASH<depth>) {
            _mm_prefetch(reinterpret_cast<const char*>(&TABLE[index(z, depth)]), _MM_HINT_T0);
        }
    }

    inline void free() {
        if (TABLE) {
            ::operator delete(TABLE, std::align_val_t(64));
            TABLE = nullptr;
            MASK = 0;
            BYTES = 0;
        }
    }

    inline void init(size_t mb = 0) {
        printf("\n");
        printf("Initializing transposition table\n");

        U64 bytes = utils::availableMemory();
        printf("Available memory: %llu MB\n", bytes >> 20);

        bool custom = false;

        U64 b;
        if (mb > 0) {
            b = U64(mb) << 20;

            if (b > bytes)  printf("Requested size (%llu MB) is more than available memory\n", mb);
            else            custom = true;
        }

        if (custom) {
            bytes = b;
            printf("Using requested TT size: %llu MB\n", mb);
        }
        else {
            bytes *= 0.9;
            printf("Using 90%% of available memory\n");
        }

        size_t n = bytes / sizeof(Bucket);
        n = std::bit_floor(n);

        BYTES = n * sizeof(Bucket);
        MASK = n - 1;

        printf("Final size: %llu MB\n", BYTES >> 20);

        TABLE = static_cast<Bucket*>(::operator new(BYTES, std::align_val_t(64)));
        std::memset(TABLE, 0, BYTES);

        printf("Initialization complete\n\n");
    }
}