#pragma once

#include "Definitions.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include <fstream>
#if defined(__AVX512F__) || defined(__AVX2__)
#include <immintrin.h>
#endif

namespace nnue {

    using namespace defs;

#if defined(__AVX512F__) && defined(__AVX512BW__)
#define NNUE_SIMD 512
    using vi16 = __m512i;
    using vi32 = __m512i;
    static constexpr int W16 = 32;
    ForceInline vi16 v_load16(const int16_t* p) { return _mm512_load_si512((const void*)p); }
    ForceInline void v_store16(int16_t* p, vi16 x) { _mm512_store_si512((void*)p, x); }
    ForceInline vi16 v_add16(vi16 a, vi16 b) { return _mm512_add_epi16(a, b); }
    ForceInline vi16 v_sub16(vi16 a, vi16 b) { return _mm512_sub_epi16(a, b); }
    ForceInline vi16 v_min16(vi16 a, vi16 b) { return _mm512_min_epi16(a, b); }
    ForceInline vi16 v_max16(vi16 a, vi16 b) { return _mm512_max_epi16(a, b); }
    ForceInline vi16 v_set1_16(int16_t x) { return _mm512_set1_epi16(x); }
    ForceInline vi16 v_mullo16(vi16 a, vi16 b) { return _mm512_mullo_epi16(a, b); }
    ForceInline vi32 v_madd16(vi16 a, vi16 b) { return _mm512_madd_epi16(a, b); }
    ForceInline vi32 v_add32(vi32 a, vi32 b) { return _mm512_add_epi32(a, b); }
    ForceInline vi32 v_zero32() { return _mm512_setzero_si512(); }
    ForceInline int32_t v_reduce32(vi32 v) { return _mm512_reduce_add_epi32(v); }
#elif defined(__AVX2__)
#define NNUE_SIMD 256
    using vi16 = __m256i;
    using vi32 = __m256i;
    static constexpr int W16 = 16;
    Inline vi16 v_load16(const int16_t* p) { return _mm256_load_si256((const __m256i*)p); }
    Inline void v_store16(int16_t* p, vi16 x) { _mm256_store_si256((__m256i*)p, x); }
    Inline vi16 v_add16(vi16 a, vi16 b) { return _mm256_add_epi16(a, b); }
    Inline vi16 v_sub16(vi16 a, vi16 b) { return _mm256_sub_epi16(a, b); }
    Inline vi16 v_min16(vi16 a, vi16 b) { return _mm256_min_epi16(a, b); }
    Inline vi16 v_max16(vi16 a, vi16 b) { return _mm256_max_epi16(a, b); }
    Inline vi16 v_set1_16(int16_t x) { return _mm256_set1_epi16(x); }
    Inline vi16 v_mullo16(vi16 a, vi16 b) { return _mm256_mullo_epi16(a, b); }
    Inline vi32 v_madd16(vi16 a, vi16 b) { return _mm256_madd_epi16(a, b); }
    Inline vi32 v_add32(vi32 a, vi32 b) { return _mm256_add_epi32(a, b); }
    Inline vi32 v_zero32() { return _mm256_setzero_si256(); }
    Inline int32_t v_reduce32(vi32 v) {
        __m128i lo = _mm256_castsi256_si128(v);
        __m128i hi = _mm256_extracti128_si256(v, 1);
        __m128i s = _mm_add_epi32(lo, hi);
        s = _mm_hadd_epi32(s, s);
        s = _mm_hadd_epi32(s, s);
        return _mm_cvtsi128_si32(s);
    }
#else
#define NNUE_SIMD 0
    using vi16 = int16_t;
    using vi32 = int32_t;
    static constexpr int W16 = 1;
    Inline vi16 v_load16(const int16_t* p) { return *p; }
    Inline void v_store16(int16_t* p, vi16 x) { *p = x; }
    Inline vi16 v_add16(vi16 a, vi16 b) { return (int16_t)(a + b); }
    Inline vi16 v_sub16(vi16 a, vi16 b) { return (int16_t)(a - b); }
    Inline vi16 v_min16(vi16 a, vi16 b) { return a < b ? a : b; }
    Inline vi16 v_max16(vi16 a, vi16 b) { return a > b ? a : b; }
    Inline vi16 v_set1_16(int16_t x) { return x; }
    Inline vi16 v_mullo16(vi16 a, vi16 b) { return (int16_t)(a * b); }
    Inline vi32 v_madd16(vi16 a, vi16 b) { return (int32_t)a * (int32_t)b; }
    Inline vi32 v_add32(vi32 a, vi32 b) { return a + b; }
    Inline vi32 v_zero32() { return 0; }
    Inline int32_t v_reduce32(vi32 v) { return v; }
#endif

    constexpr int L1 = 1024;

    // Feature HalfKA
    constexpr int KING_BUCKETS = 32;
    constexpr int COLORS = 2;
    constexpr int PIECE_TYPES = 6;
    constexpr int SQUARES = 64;
    constexpr int FT_IN = KING_BUCKETS * COLORS * PIECE_TYPES * SQUARES;

    constexpr int32_t QA = 255;
    constexpr int32_t QB = 64;
    constexpr int32_t SCALE = 400;

    struct Network {
        alignas(64) int16_t ftW[FT_IN][L1];
        alignas(64) int16_t ftBias[L1];

        alignas(64) int16_t outW[2 * L1];
        int32_t             outBias;
    };

    inline Network* net = nullptr;

    struct FeatureId {
        uint16_t id[2];

        constexpr uint16_t& operator[](int i) { return id[i]; }
        constexpr const uint16_t& operator[](int i) const { return id[i]; }
    };

    struct DirtyPiece {
        bool   refresh[2] = { false, false };

        FeatureId adds[2];
        FeatureId subs[2];
        int8_t nAdd = 0;
        int8_t nSub = 0;

        inline void add(FeatureId fId) {
            adds[nAdd++] = fId;
        }

        inline void sub(FeatureId fId) {
            for (int i = 0; i < nAdd; i++) {
                if (adds[i][0] == fId[0]) {
                    adds[i] = adds[--nAdd];
                    return;
                }
            }
            subs[nSub++] = fId;
        }

        inline void clear() { nAdd = nSub = 0; refresh[white] = refresh[black] = false; }
    };

    struct Accumulator {
        alignas(64) int16_t v[2][L1];
        DirtyPiece dirty;
        bool computed[2] = { false, false };
    };

    inline Accumulator accumulators[MAX_PLY + 1];

    template <bool persp, class Board>
    Inline int kingSquare(const Board& board) {
        if constexpr (persp == white) return (board.side == white) ? board.kMS : board.kES;
        else                          return (board.side == white) ? board.kES : board.kMS;
    }

    template <Piece piece, bool persp, bool color>
    Inline FeatureId halfKAIndex(int sq, int kMS, int kES) {
        FeatureId f;
        const int wK = (persp == white ? kMS : kES);
        const int bK = (persp == black ? kMS : kES) ^ 56;

        const int wMir = MIRROR[wK & 7];
        const int bMir = MIRROR[bK & 7];

        const int wKb = KING_BUCKET[wK ^ wMir];
        const int bKb = KING_BUCKET[bK ^ bMir];

        f[white] = ((wKb * COLORS + (color == white ? 0 : 1)) * PIECE_TYPES + int(piece)) * SQUARES + (sq ^ wMir);
        f[black] = ((bKb * COLORS + (color == black ? 0 : 1)) * PIECE_TYPES + int(piece)) * SQUARES + ((sq ^ 56) ^ bMir);

        return f;
    }

    template <bool persp>
    Inline void applyDirty(Accumulator& dst, const Accumulator& src, const DirtyPiece& d) {
        const int16_t* s = src.v[persp];
        int16_t* out = dst.v[persp];

        for (int i = 0; i < L1; i += 4 * W16) {
            vi16 acc0 = v_load16(s + i + 0 * W16);
            vi16 acc1 = v_load16(s + i + 1 * W16);
            vi16 acc2 = v_load16(s + i + 2 * W16);
            vi16 acc3 = v_load16(s + i + 3 * W16);

            for (int a = 0; a < d.nAdd; ++a) {
                const int16_t* w = net->ftW[d.adds[a][persp]];
                acc0 = v_add16(acc0, v_load16(w + i + 0 * W16));
                acc1 = v_add16(acc1, v_load16(w + i + 1 * W16));
                acc2 = v_add16(acc2, v_load16(w + i + 2 * W16));
                acc3 = v_add16(acc3, v_load16(w + i + 3 * W16));
            }

            for (int b = 0; b < d.nSub; ++b) {
                const int16_t* w = net->ftW[d.subs[b][persp]];
                acc0 = v_sub16(acc0, v_load16(w + i + 0 * W16));
                acc1 = v_sub16(acc1, v_load16(w + i + 1 * W16));
                acc2 = v_sub16(acc2, v_load16(w + i + 2 * W16));
                acc3 = v_sub16(acc3, v_load16(w + i + 3 * W16));
            }

            v_store16(out + i + 0 * W16, acc0);
            v_store16(out + i + 1 * W16, acc1);
            v_store16(out + i + 2 * W16, acc2);
            v_store16(out + i + 3 * W16, acc3);
        }
    }

    template <bool persp, class Board>
    Inline void refreshFromBoard(Accumulator& acc, const Board& board) {
        const int kMS = kingSquare<persp>(board);
        const int kES = kingSquare<!persp>(board);

        const bool stmW = (board.side == white);
        U64 wP = stmW ? board.pM : board.pE, bP = stmW ? board.pE : board.pM;
        U64 wN = stmW ? board.nM : board.nE, bN = stmW ? board.nE : board.nM;
        U64 wB = stmW ? board.bM : board.bE, bB = stmW ? board.bE : board.bM;
        U64 wR = stmW ? board.rM : board.rE, bR = stmW ? board.rE : board.rM;
        U64 wQ = stmW ? board.qM : board.qE, bQ = stmW ? board.qE : board.qM;
        U64 wK = stmW ? board.kM : board.kE, bK = stmW ? board.kE : board.kM;

        int idx[32];
        int cnt = 0;
        Bitloop(wP) { idx[cnt++] = halfKAIndex<Piece::Pawn, persp, white>(SquareOf(wP), kMS, kES)[persp]; }
        Bitloop(bP) { idx[cnt++] = halfKAIndex<Piece::Pawn, persp, black>(SquareOf(bP), kMS, kES)[persp]; }
        Bitloop(wN) { idx[cnt++] = halfKAIndex<Piece::Knight, persp, white>(SquareOf(wN), kMS, kES)[persp]; }
        Bitloop(bN) { idx[cnt++] = halfKAIndex<Piece::Knight, persp, black>(SquareOf(bN), kMS, kES)[persp]; }
        Bitloop(wB) { idx[cnt++] = halfKAIndex<Piece::Bishop, persp, white>(SquareOf(wB), kMS, kES)[persp]; }
        Bitloop(bB) { idx[cnt++] = halfKAIndex<Piece::Bishop, persp, black>(SquareOf(bB), kMS, kES)[persp]; }
        Bitloop(wR) { idx[cnt++] = halfKAIndex<Piece::Rook, persp, white>(SquareOf(wR), kMS, kES)[persp]; }
        Bitloop(bR) { idx[cnt++] = halfKAIndex<Piece::Rook, persp, black>(SquareOf(bR), kMS, kES)[persp]; }
        Bitloop(wQ) { idx[cnt++] = halfKAIndex<Piece::Queen, persp, white>(SquareOf(wQ), kMS, kES)[persp]; }
        Bitloop(bQ) { idx[cnt++] = halfKAIndex<Piece::Queen, persp, black>(SquareOf(bQ), kMS, kES)[persp]; }
        idx[cnt++] = halfKAIndex<Piece::King, persp, white>(SquareOf(wK), kMS, kES)[persp];
        idx[cnt++] = halfKAIndex<Piece::King, persp, black>(SquareOf(bK), kMS, kES)[persp];

        int16_t* a = acc.v[persp];
        for (int i = 0; i < L1; i += 4 * W16) {
            vi16 v0 = v_load16(net->ftBias + i + 0 * W16);
            vi16 v1 = v_load16(net->ftBias + i + 1 * W16);
            vi16 v2 = v_load16(net->ftBias + i + 2 * W16);
            vi16 v3 = v_load16(net->ftBias + i + 3 * W16);
            for (int k = 0; k < cnt; ++k) {
                const int16_t* w = net->ftW[idx[k]];
                v0 = v_add16(v0, v_load16(w + i + 0 * W16));
                v1 = v_add16(v1, v_load16(w + i + 1 * W16));
                v2 = v_add16(v2, v_load16(w + i + 2 * W16));
                v3 = v_add16(v3, v_load16(w + i + 3 * W16));
            }
            v_store16(a + i + 0 * W16, v0);
            v_store16(a + i + 1 * W16, v1);
            v_store16(a + i + 2 * W16, v2);
            v_store16(a + i + 3 * W16, v3);
        }
    }

    template <bool persp, class Board>
    Inline void ensureComputed(int ply, const Board& board) {
        if (accumulators[ply].computed[persp]) return;

        int base = ply;
        while (base > 0 && !accumulators[base - 1].computed[persp]) {
            if (accumulators[base].dirty.refresh[persp]) {
                refreshFromBoard<persp>(accumulators[ply], board);
                accumulators[ply].computed[persp] = true;
                return;
            }
            --base;
        }

        if (base == 0) return;

        if (accumulators[base].dirty.refresh[persp]) {
            refreshFromBoard<persp>(accumulators[ply], board);
            accumulators[ply].computed[persp] = true;
            return;
        }

        for (int k = base; k <= ply; ++k) {
            applyDirty<persp>(accumulators[k], accumulators[k - 1], accumulators[k].dirty);
            accumulators[k].computed[persp] = true;
        }
    }

    template <class Board>
    Inline void initRoot(const Board& board) {
        refreshFromBoard<white>(accumulators[0], board);
        refreshFromBoard<black>(accumulators[0], board);
        accumulators[0].computed[white] = true;
        accumulators[0].computed[black] = true;
        accumulators[0].dirty.clear();
    }

    template <bool side, class Board>
    Inline int evaluate(int ply, const Board& board) {
        ensureComputed<white>(ply, board);
        ensureComputed<black>(ply, board);

        const int16_t* accM = accumulators[ply].v[side];
        const int16_t* accE = accumulators[ply].v[!side];

        const vi16 zero = v_set1_16(0);
        const vi16 qa = v_set1_16((int16_t)QA);

        vi32 accS = v_zero32();
        vi32 accN = v_zero32();

        for (int i = 0; i < L1; i += W16) {
            vi16 cM = v_min16(v_max16(v_load16(accM + i), zero), qa);
            accS = v_add32(accS, v_madd16(v_mullo16(cM, v_load16(net->outW + i)), cM));

            vi16 cE = v_min16(v_max16(v_load16(accE + i), zero), qa);
            accN = v_add32(accN, v_madd16(v_mullo16(cE, v_load16(net->outW + L1 + i)), cE));
        }

        int64_t out = (int64_t)net->outBias + v_reduce32(v_add32(accS, accN));

        return (int)(out * SCALE / ((int64_t)QA * QA * QB));
    }

    Inline bool loadWeights(const char* path) {
        if (!net) net = new Network();

        std::ifstream f(path, std::ios::binary);
        if (!f) {
            std::printf("NNUE: cannot open %s\n", path);
            return false;
        }

        auto readBlock = [&](void* dst, size_t size) {
            f.read(reinterpret_cast<char*>(dst), size);
            return (bool)f;
            };

        if (!readBlock(net->ftW, sizeof(net->ftW)))          return false;
        if (!readBlock(net->ftBias, sizeof(net->ftBias)))    return false;
        if (!readBlock(net->outW, sizeof(net->outW)))        return false;
        if (!readBlock(&net->outBias, sizeof(net->outBias))) return false;

        return true;
    }
}