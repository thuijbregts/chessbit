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
    ForceInline vi16 v_load16(const int16_t* p) { return _mm256_load_si256((const __m256i*)p); }
    ForceInline void v_store16(int16_t* p, vi16 x) { _mm256_store_si256((__m256i*)p, x); }
    ForceInline vi16 v_add16(vi16 a, vi16 b) { return _mm256_add_epi16(a, b); }
    ForceInline vi16 v_sub16(vi16 a, vi16 b) { return _mm256_sub_epi16(a, b); }
    ForceInline vi16 v_min16(vi16 a, vi16 b) { return _mm256_min_epi16(a, b); }
    ForceInline vi16 v_max16(vi16 a, vi16 b) { return _mm256_max_epi16(a, b); }
    ForceInline vi16 v_set1_16(int16_t x) { return _mm256_set1_epi16(x); }
    ForceInline vi16 v_mullo16(vi16 a, vi16 b) { return _mm256_mullo_epi16(a, b); }
    ForceInline vi32 v_madd16(vi16 a, vi16 b) { return _mm256_madd_epi16(a, b); }
    ForceInline vi32 v_add32(vi32 a, vi32 b) { return _mm256_add_epi32(a, b); }
    ForceInline vi32 v_zero32() { return _mm256_setzero_si256(); }
    ForceInline int32_t v_reduce32(vi32 v) {
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
    ForceInline vi16 v_load16(const int16_t* p) { return *p; }
    ForceInline void v_store16(int16_t* p, vi16 x) { *p = x; }
    ForceInline vi16 v_add16(vi16 a, vi16 b) { return (int16_t)(a + b); }
    ForceInline vi16 v_sub16(vi16 a, vi16 b) { return (int16_t)(a - b); }
    ForceInline vi16 v_min16(vi16 a, vi16 b) { return a < b ? a : b; }
    ForceInline vi16 v_max16(vi16 a, vi16 b) { return a > b ? a : b; }
    ForceInline vi16 v_set1_16(int16_t x) { return x; }
    ForceInline vi16 v_mullo16(vi16 a, vi16 b) { return (int16_t)(a * b); }
    ForceInline vi32 v_madd16(vi16 a, vi16 b) { return (int32_t)a * (int32_t)b; }
    ForceInline vi32 v_add32(vi32 a, vi32 b) { return a + b; }
    ForceInline vi32 v_zero32() { return 0; }
    ForceInline int32_t v_reduce32(vi32 v) { return v; }
#endif

    constexpr int L1 = 1024;
    constexpr int L2 = 32;
    constexpr int L3 = 32;

    // Feature HalfKA
    constexpr int KING_BUCKETS = 64;
    constexpr int COLORS = 2;
    constexpr int PIECE_TYPES = 6;
    constexpr int SQUARES = 64;
    constexpr int FT_IN = KING_BUCKETS * COLORS * PIECE_TYPES * SQUARES;

    constexpr int32_t QA = 255;
    constexpr int32_t QB = 64;
    constexpr int32_t SCALE = 400;

    ForceInline int32_t SCReLUHidden(int64_t pre) {
        //formule corrigée
        int32_t x = (int32_t)std::clamp<int64_t>(pre / QB, 0, QA);
        return x * x;
    }

    struct Network {
        alignas(64) int16_t ftW[FT_IN][L1];
        alignas(64) int16_t ftBias[L1];

        alignas(64) int16_t l1W[2][L2][L1];
        alignas(64) int32_t l1Bias[2][L2];

        alignas(64) int16_t l2W[L3][2 * L2];
        alignas(64) int32_t l2Bias[L3];

        alignas(64) int16_t outW[L3];
        int32_t             outBias;
    };

    inline Network* net = nullptr;

    struct FeatureId {
        int id[2];
        constexpr int& operator[](int i) { return id[i]; }
        constexpr const int& operator[](int i) const { return id[i]; }
    };

    struct FeatureChange {
        bool add;
        FeatureId fId;
    };

    struct DirtyPiece {
        int8_t n = 0;
        bool   refresh[2] = { false, false };
        FeatureChange ch[4];

        inline void push(bool add, FeatureId fId) { ch[n++] = { add, fId }; }
        inline void clear() { n = 0; refresh[white] = refresh[black] = false; }
    };

    struct Accumulator {
        alignas(64) int16_t v[2][L1];
        DirtyPiece dirty;
        bool computed[2] = { false, false };
    };

    inline Accumulator accumulators[MAX_PLY + 1];

    template <bool persp>
    ForceInline void applyDirty(Accumulator& dst, const Accumulator& src, const DirtyPiece& d) {
        std::memcpy(dst.v[persp], src.v[persp], sizeof(dst.v[persp]));
        int16_t* a = dst.v[persp];
        for (int c = 0; c < d.n; ++c) {
            const int16_t* w = net->ftW[d.ch[c].fId[persp]];
            const bool add = d.ch[c].add;
            for (int i = 0; i < L1; i += W16) {
                vi16 v = v_load16(a + i);
                v = add ? v_add16(v, v_load16(w + i)) : v_sub16(v, v_load16(w + i));
                v_store16(a + i, v);
            }
        }
    }

    template <bool persp, class Board>
    ForceInline int kingSquare(const Board& board) {
        if constexpr (persp == white) return (board.side == white) ? board.kMS : board.kES;
        else                          return (board.side == white) ? board.kES : board.kMS;
    }

    template <Piece piece, bool persp, bool color>
    ForceInline FeatureId halfKAIndex(int sq, int kMS, int kES) {
        FeatureId f;
        const int wK = (persp == white ? kMS : kES);
        const int bK = (persp == black ? kMS : kES) ^ 56;
        f[white] = ((wK * COLORS + (color == white ? 0 : 1)) * PIECE_TYPES + int(piece)) * SQUARES + sq;
        f[black] = ((bK * COLORS + (color == black ? 0 : 1)) * PIECE_TYPES + int(piece)) * SQUARES + (sq ^ 56);

        //printf("sq: %d, kms: %d, kes: %d, piece: %d, persp: %d, feat white: %d, feat black: %d\n", sq, kMS, kES, int(piece), persp, f[white], f[black]);
        return f;
    }

    template <bool persp, class Board>
    ForceInline void refreshFromBoard(Accumulator& acc, const Board& board) {
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

        /*for (int i = 0; i < cnt; i++) {
            printf("%d, ", idx[i]);
        }*/

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
    ForceInline void ensureComputed(int ply, const Board& board) {
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
        if (base == 0 || accumulators[base].dirty.refresh[persp]) {
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
    inline void initRoot(const Board& board) {
        refreshFromBoard<white>(accumulators[0], board);
        refreshFromBoard<black>(accumulators[0], board);
        accumulators[0].computed[white] = true;
        accumulators[0].computed[black] = true;
        accumulators[0].dirty.clear();
    }

    template <bool side, class Board>
    ForceInline int evaluate(int ply, const Board& board) {
        ensureComputed<white>(ply, board);
        ensureComputed<black>(ply, board);

        const int16_t* accuM = accumulators[ply].v[side];
        const int16_t* accuE = accumulators[ply].v[!side];

        const vi16 zero = v_set1_16(0);
        const vi16 qa = v_set1_16((int16_t)QA);

        alignas(64) int16_t inputM[L1];
        alignas(64) int16_t inputE[L1];
        for (int i = 0; i < L1; i += W16) {
            v_store16(inputM + i, v_min16(v_max16(v_load16(accuM + i), zero), qa));
            v_store16(inputE + i, v_min16(v_max16(v_load16(accuE + i), zero), qa));
        }

        int32_t h[2 * L2];
        for (int j = 0; j < L2; ++j) {
            const int16_t* w0 = net->l1W[0][j];
            const int16_t* w1 = net->l1W[1][j];
            vi32 a0a = v_zero32(), a0b = v_zero32(), a1a = v_zero32(), a1b = v_zero32();
            for (int i = 0; i < L1; i += 2 * W16) {
                vi16 x0a = v_load16(inputM + i), x0b = v_load16(inputM + i + W16);
                vi16 x1a = v_load16(inputE + i), x1b = v_load16(inputE + i + W16);
                /*a0a = v_add32(a0a, v_madd16(v_mullo16(x0a, v_load16(w0 + i)), x0a));
                a0b = v_add32(a0b, v_madd16(v_mullo16(x0b, v_load16(w0 + i + W16)), x0b));
                a1a = v_add32(a1a, v_madd16(v_mullo16(x1a, v_load16(w1 + i)), x1a));
                a1b = v_add32(a1b, v_madd16(v_mullo16(x1b, v_load16(w1 + i + W16)), x1b));*/

                //formule corrigée
                a0a = v_add32(a0a, v_madd16(x0a, v_load16(w0 + i)));
                a0b = v_add32(a0b, v_madd16(x0b, v_load16(w0 + i + W16)));
                a1a = v_add32(a1a, v_madd16(x1a, v_load16(w1 + i)));
                a1b = v_add32(a1b, v_madd16(x1b, v_load16(w1 + i + W16)));
            }
            h[j] = SCReLUHidden((int64_t)net->l1Bias[0][j] + v_reduce32(v_add32(a0a, a0b)));
            h[L2 + j] = SCReLUHidden((int64_t)net->l1Bias[1][j] + v_reduce32(v_add32(a1a, a1b)));
        }

        int32_t h2[L3];
        for (int j = 0; j < L3; ++j) {
            int64_t s = net->l2Bias[j];
            const int16_t* w = net->l2W[j];
            for (int i = 0; i < 2 * L2; ++i)
                s += (int64_t)h[i] * w[i];
            h2[j] = SCReLUHidden(s);
        }

        int64_t out = net->outBias;
        for (int i = 0; i < L3; ++i)
            out += (int64_t)h2[i] * net->outW[i];

        //formule corrigée
        out /= QA;
        out += net->outBias;
        out *= SCALE;
        out /= (QA * QB);

        return (int)out;
    }

    template <class Board>
    inline bool debugCheck(int ply, const Board& board) {
        ensureComputed<white>(ply, board);
        ensureComputed<black>(ply, board);
        Accumulator ref;
        refreshFromBoard<white>(ref, board);
        refreshFromBoard<black>(ref, board);
        return std::memcmp(ref.v[white], accumulators[ply].v[white], sizeof(ref.v[white])) == 0
            && std::memcmp(ref.v[black], accumulators[ply].v[black], sizeof(ref.v[black])) == 0;
    }

    inline bool loadWeights(const char* path) {
        if (!net)
            net = new Network();

        std::ifstream f(path, std::ios::binary | std::ios::ate);

        if (!f) {
            std::printf("NNUE: cannot open %s\n", path);
            return false;
        }

        const std::streamsize fileSize = f.tellg();

        constexpr size_t ftWSize = sizeof(net->ftW);
        constexpr size_t ftBiasSize = sizeof(net->ftBias);
        constexpr size_t l1WSize = sizeof(net->l1W);
        constexpr size_t l1BiasSize = sizeof(net->l1Bias);
        constexpr size_t l2WSize = sizeof(net->l2W);
        constexpr size_t l2BiasSize = sizeof(net->l2Bias);
        constexpr size_t outWSize = sizeof(net->outW);
        constexpr size_t outBiasSize = sizeof(net->outBias);

        const size_t expectedSize =
            ftWSize
            + ftBiasSize
            + l1WSize
            + l1BiasSize
            + l2WSize
            + l2BiasSize
            + outWSize
            + outBiasSize;

        std::printf("\n=== NNUE FILE ===\n");
        std::printf("file size:     %lld\n", (long long)fileSize);
        std::printf("expected data: %zu\n", expectedSize);
        std::printf("remaining:     %lld\n",
            (long long)fileSize - (long long)expectedSize);

        f.seekg(0, std::ios::beg);

        auto readBlock = [&](const char* name, void* dst, size_t size) {
            const std::streamoff pos = f.tellg();

            std::printf(
                "%-12s offset=%lld size=%zu\n",
                name,
                (long long)pos,
                size
            );

            f.read(reinterpret_cast<char*>(dst), size);

            if (!f) {
                std::printf("ERROR reading %s\n", name);
                return false;
            }

            return true;
            };

        if (!readBlock("ftW", net->ftW, sizeof(net->ftW)))      return false;
        if (!readBlock("ftBias", net->ftBias, sizeof(net->ftBias)))   return false;

        if (!readBlock("l1W", net->l1W, sizeof(net->l1W)))      return false;
        if (!readBlock("l1Bias", net->l1Bias, sizeof(net->l1Bias)))   return false;

        if (!readBlock("l2W", net->l2W, sizeof(net->l2W)))      return false;
        if (!readBlock("l2Bias", net->l2Bias, sizeof(net->l2Bias)))   return false;

        if (!readBlock("outW", net->outW, sizeof(net->outW)))     return false;
        if (!readBlock("outBias", &net->outBias, sizeof(net->outBias))) return false;

        std::printf("\n=== FIRST VALUES ===\n");

        std::printf("ftW[0][0..15] = ");
        for (int i = 0; i < 16; ++i)
            std::printf("%d ", (int)net->ftW[0][i]);
        std::printf("\n");

        std::printf("ftBias[0..15] = ");
        for (int i = 0; i < 16; ++i)
            std::printf("%d ", (int)net->ftBias[i]);
        std::printf("\n");

        std::printf("l1W[0][0][0..15] = ");
        for (int i = 0; i < 16; ++i)
            std::printf("%d ", (int)net->l1W[0][0][i]);
        std::printf("\n");

        std::printf("l1W[1][0][0..15] = ");
        for (int i = 0; i < 16; ++i)
            std::printf("%d ", (int)net->l1W[1][0][i]);
        std::printf("\n");

        std::printf("l1Bias[0][0..15] = ");
        for (int i = 0; i < 16; ++i)
            std::printf("%d ", (int)net->l1Bias[0][i]);
        std::printf("\n");

        std::printf("l1Bias[1][0..15] = ");
        for (int i = 0; i < 16; ++i)
            std::printf("%d ", (int)net->l1Bias[1][i]);
        std::printf("\n");

        std::printf("l2W[0][0..15] = ");
        for (int i = 0; i < 16; ++i)
            std::printf("%d ", (int)net->l2W[0][i]);
        std::printf("\n");

        std::printf("l2Bias[0..15] = ");
        for (int i = 0; i < 16; ++i)
            std::printf("%d ", (int)net->l2Bias[i]);
        std::printf("\n");

        std::printf("outW[0..15] = ");
        for (int i = 0; i < 16; ++i)
            std::printf("%d ", (int)net->outW[i]);
        std::printf("\n");

        std::printf("outBias = %d\n", net->outBias);

        auto range16 = [](const char* name, const int16_t* p, size_t n) {
            int16_t mn = INT16_MAX;
            int16_t mx = INT16_MIN;

            for (size_t i = 0; i < n; ++i) {
                mn = std::min(mn, p[i]);
                mx = std::max(mx, p[i]);
            }

            std::printf("%-12s min=%d max=%d\n", name, (int)mn, (int)mx);
            };

        auto range32 = [](const char* name, const int32_t* p, size_t n) {
            int32_t mn = INT32_MAX;
            int32_t mx = INT32_MIN;

            for (size_t i = 0; i < n; ++i) {
                mn = std::min(mn, p[i]);
                mx = std::max(mx, p[i]);
            }

            std::printf("%-12s min=%d max=%d\n", name, mn, mx);
            };

        range16("ftW", &net->ftW[0][0], FT_IN * L1);
        range16("ftBias", net->ftBias, L1);

        range16("l1W", &net->l1W[0][0][0], 2 * L2 * L1);
        range32("l1Bias", &net->l1Bias[0][0], 2 * L2);

        range16("l2W", &net->l2W[0][0], L3 * 2 * L2);
        range32("l2Bias", net->l2Bias, L3);

        range16("outW", net->outW, L3);
        range32("outBias", &net->outBias, 1);

        return true;
    }
}