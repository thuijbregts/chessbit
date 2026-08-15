#pragma once
#include "Definitions.h"
#include "Utils.h"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <new>
#include <bit>

using namespace defs;

namespace tt {

    constexpr int MIN_HASH_DEPTH = 2;
    constexpr int MAX_HASH_DEPTH = 63;

    template <int depth>
    constexpr bool USE_HASH = (depth >= MIN_HASH_DEPTH && depth <= MAX_HASH_DEPTH);

    constexpr int MAX_ENTRIES = 4;
    constexpr int BUCKET_SIZE = MAX_ENTRIES * 16;

    struct alignas(BUCKET_SIZE) Bucket {
        U64 key[MAX_ENTRIES];
        U64 data[MAX_ENTRIES];
    };

    inline Bucket* TABLE = nullptr;
    inline U64     MASK  = 0;
    inline U64     BYTES = 0;
    inline uint8_t GENERATION = 0;

    enum Bound : uint8_t {
        BOUND_NONE  = 0,
        BOUND_UPPER = 1,
        BOUND_LOWER = 2,
        BOUND_EXACT = 3
    };

    constexpr U64 SCORE_MASK  = 0xFFFF;
    constexpr U64 MOVE_MASK  = 0xFFFF;
    constexpr U64 DEPTH_MASK  = 0xFF;
    constexpr U64 BOUND_MASK  = 0x3;
    constexpr U64 GEN_MASK = 0x3F;

    constexpr U64 MOVE_SHIFT  = 16;
    constexpr U64 DEPTH_SHIFT = 32;
    constexpr U64 BOUND_SHIFT = 40;
    constexpr U64 GEN_SHIFT   = 42;

    struct TTData {
        int      score;
        uint16_t move;
        uint8_t  depth;
        uint8_t  bound;
    };

    ForceInline U64 index(Zobrist z) noexcept {
        return z.low & MASK;
    }

    ForceInline Bucket& bucket(Zobrist z) noexcept {
        return TABLE[index(z)];
    }

    ForceInline bool probe(Bucket& b, Zobrist z, TTData& out) noexcept {
        for (int i = 0; i < MAX_ENTRIES; ++i) {
            const U64 d = b.data[i];
            if (b.key[i] == (z.high ^ d)) {
                out.score = (int16_t)(d & SCORE_MASK);
                out.move  = (d >> MOVE_SHIFT)  & MOVE_MASK;
                out.depth = (d >> DEPTH_SHIFT) & DEPTH_MASK;
                out.bound = (d >> BOUND_SHIFT) & BOUND_MASK;
                return true;
            }
        }
        return false;
    }

    ForceInline void write(int depth, Bucket& b, Zobrist z, int score, uint8_t bound, uint16_t move, uint8_t gen) noexcept {
        int v    = 0;
        int lowest = INT_MAX;

        for (int i = 0; i < MAX_ENTRIES; ++i) {
            const U64 di = b.data[i];

            if (di == 0) {
                v = i;
                break;
            }

            if (b.key[i] == (z.high ^ di)) {
                const int d = (di >> DEPTH_SHIFT) & DEPTH_MASK;
                if (depth < d) return;
                v = i;
                break;
            }

            const int d = (di >> DEPTH_SHIFT) & DEPTH_MASK;
            const int g = (di >> GEN_SHIFT) & GEN_MASK;
            const int age = gen - g;
            int val = d - 2 * age;

            if (val < lowest) { lowest = val; v = i; }
        }

        const U64 data =
              ((U64)(uint16_t)(int16_t)score)
            | ((U64)move                << MOVE_SHIFT)
            | ((U64)(uint8_t)depth       << DEPTH_SHIFT)
            | ((U64)(bound & 0x3u)       << BOUND_SHIFT)
            | ((U64)(gen   & GEN_MASK)   << GEN_SHIFT);

        b.key[v]  = z.high ^ data;
        b.data[v] = data;
    }

    ForceInline void prefetch(Zobrist z, int depth) noexcept {
        _mm_prefetch(reinterpret_cast<const char*>(&TABLE[index(z)]), _MM_HINT_T0);
    }

    inline void free() {
        if (TABLE) {
            ::operator delete(TABLE, std::align_val_t(64));
            TABLE = nullptr;
            MASK = 0;
            BYTES = 0;
        }
        printf("Transposition table cleared\n");
    }

    inline void clear() {
        std::memset(TABLE, 0, BYTES);
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
        clear();

        printf("Initialization complete\n\n");
    }
}