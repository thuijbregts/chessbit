#pragma once

#include "Definitions.h"
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <random>
#include <fstream>

namespace nnue {

    using namespace defs;

    constexpr int L1 = 1024;
    constexpr int L2 = 32;
    constexpr int L3 = 32;

    // Feature HalfKA : king_bucket * couleur * type * case
    constexpr int KING_BUCKETS = 64;   // 1 bucket par case de roi (pas de mirror H)
    constexpr int COLORS = 2;
    constexpr int PIECE_TYPES = 6;
    constexpr int SQUARES = 64;
    constexpr int FT_IN = KING_BUCKETS * COLORS * PIECE_TYPES * SQUARES;

    constexpr int32_t QA = 255;
    constexpr int32_t QB = 64;
    constexpr int32_t SCALE = 400;


    inline int32_t screlu(int32_t x) {
        x = std::clamp(x, int32_t(0), QA);
        return x * x;
    }

    inline int32_t screluHidden(int64_t pre) {
        int32_t x = (int32_t)std::clamp<int64_t>(pre / (QA * QB), 0, QA);
        return x * x;
    }

    struct Network {
        alignas(64) int16_t ftW[FT_IN][L1];
        alignas(64) int16_t ftBias[L1];

        alignas(64) int16_t l1W[2][L1][L2];
        alignas(64) int32_t l1Bias[2][L2];

        alignas(64) int16_t l2W[2 * L2][L3];
        alignas(64) int32_t l2Bias[L3];

        alignas(64) int16_t outW[L3];
        int32_t             outBias;
    };

    inline Network* net = nullptr;

    struct Accumulator {
        alignas(64) int16_t v[2][L1];

        void setBias() {
            for (int i = 0; i < L1; ++i) {
                v[white][i] = net->ftBias[i];
                v[black][i] = net->ftBias[i];
            }
        }
    };

    inline Accumulator accumulators[MAX_PLY];

    inline void initRandom(uint64_t seed = 0xC0FFEEu) {
        if (!net) net = new Network();

        std::mt19937 rng((uint32_t)seed);
        auto rnd = [&](int lo, int hi) {
            return (int16_t)(std::uniform_int_distribution<int>(lo, hi)(rng));
            };

        for (int f = 0; f < FT_IN; ++f)
            for (int i = 0; i < L1; ++i)
                net->ftW[f][i] = rnd(-63, 63);
        for (int i = 0; i < L1; ++i) net->ftBias[i] = rnd(-QA, QA);

        for (int p = 0; p < 2; ++p) {
            for (int i = 0; i < L1; ++i)
                for (int j = 0; j < L2; ++j)
                    net->l1W[p][i][j] = rnd(-QB, QB);
            for (int j = 0; j < L2; ++j) net->l1Bias[p][j] = rnd(-QB, QB);
        }

        for (int i = 0; i < 2 * L2; ++i)
            for (int j = 0; j < L3; ++j)
                net->l2W[i][j] = rnd(-QB, QB);
        for (int j = 0; j < L3; ++j) net->l2Bias[j] = rnd(-QB, QB);

        for (int i = 0; i < L3; ++i) net->outW[i] = rnd(-QB, QB);
        net->outBias = rnd(-QB, QB);
    }

    inline bool loadWeights(const char* path) {
        if (!net) net = new Network();
        std::ifstream f(path, std::ios::binary);
        if (!f) return false;
        f.read(reinterpret_cast<char*>(net->ftW), sizeof(net->ftW));
        f.read(reinterpret_cast<char*>(net->ftBias), sizeof(net->ftBias));
        f.read(reinterpret_cast<char*>(net->l1W), sizeof(net->l1W));
        f.read(reinterpret_cast<char*>(net->l1Bias), sizeof(net->l1Bias));
        f.read(reinterpret_cast<char*>(net->l2W), sizeof(net->l2W));
        f.read(reinterpret_cast<char*>(net->l2Bias), sizeof(net->l2Bias));
        f.read(reinterpret_cast<char*>(net->outW), sizeof(net->outW));
        f.read(reinterpret_cast<char*>(&net->outBias), sizeof(net->outBias));
        return (bool)f;
    }

    template <Piece piece, bool side, bool color>
    inline int featureIndex(int sq, int kingSq) {
        constexpr int relColor = (side == color) ? 0 : 1;
        if constexpr (color == black) { sq ^= 56; kingSq ^= 56; }

        return ((kingSq * COLORS + relColor) * PIECE_TYPES + static_cast<int>(piece)) * SQUARES + sq;
    }

    template <Piece piece, bool side>
    inline void addPiece(Accumulator& a, int sq, int kMS) {
        const int iw = featureIndex<piece, side, white>(sq, kMS);
        const int ib = featureIndex<piece, side, black>(sq, kMS);
        const int16_t* ww = net->ftW[iw];
        const int16_t* wb = net->ftW[ib];
        for (int i = 0; i < L1; ++i) {
            a.v[white][i] += ww[i];
            a.v[black][i] += wb[i];
        }
    }

    template <Piece piece, bool side>
    inline void removePiece(Accumulator& a, int sq, int kMS) {
        const int iw = featureIndex<piece, side, white>(sq, kMS);
        const int ib = featureIndex<piece, side, black>(sq, kMS);
        const int16_t* ww = net->ftW[iw];
        const int16_t* wb = net->ftW[ib];
        for (int i = 0; i < L1; ++i) {
            a.v[white][i] -= ww[i];
            a.v[black][i] -= wb[i];
        }
    }

    template <Piece piece, bool side>
    inline void movePiece(Accumulator& a, int from, int to, int kMS) {
        removePiece<piece, side>(a, from, kMS);
        addPiece<piece, side>(a, to, kMS);
    }

    template <bool side>
    inline void refresh(Accumulator& a, U64 pM, U64 nM, U64 bM, U64 rM, U64 qM, int kMS) {
        std::memset(a.v[side], 0, sizeof(a.v[side]));

        U64 bb = pM;
        Bitloop(bb) { addPiece<Piece::Pawn, side>(a, SquareOf(bb), kMS); }
        bb = nM;
        Bitloop(bb) { addPiece<Piece::Knight, side>(a, SquareOf(bb), kMS); }
        bb = bM;
        Bitloop(bb) { addPiece<Piece::Bishop, side>(a, SquareOf(bb), kMS); }
        bb = rM;
        Bitloop(bb) { addPiece<Piece::Rook, side>(a, SquareOf(bb), kMS); }
        bb = qM;
        Bitloop(bb) { addPiece<Piece::Queen, side>(a, SquareOf(bb), kMS); }
    }

    template <bool side>
    inline void init(Accumulator& a, U64 pM, U64 nM, U64 bM, U64 rM, U64 qM, int kMS, U64 pE, U64 nE, U64 bE, U64 rE, U64 qE, int kES) {
        std::memset(a.v[white], 0, sizeof(a.v[white]));
        std::memset(a.v[black], 0, sizeof(a.v[black]));

        a.setBias();

        U64 bb = pM;
        Bitloop(bb) { addPiece<Piece::Pawn, side>(a, SquareOf(bb), kMS); }
        bb = nM;
        Bitloop(bb) { addPiece<Piece::Knight, side>(a, SquareOf(bb), kMS); }
        bb = bM;
        Bitloop(bb) { addPiece<Piece::Bishop, side>(a, SquareOf(bb), kMS); }
        bb = rM;
        Bitloop(bb) { addPiece<Piece::Rook, side>(a, SquareOf(bb), kMS); }
        bb = qM;
        Bitloop(bb) { addPiece<Piece::Queen, side>(a, SquareOf(bb), kMS); }

        bb = pE;
        Bitloop(bb) { addPiece<Piece::Pawn, !side>(a, SquareOf(bb), kES); }
        bb = nE;
        Bitloop(bb) { addPiece<Piece::Knight, !side>(a, SquareOf(bb), kES); }
        bb = bE;
        Bitloop(bb) { addPiece<Piece::Bishop, !side>(a, SquareOf(bb), kES); }
        bb = rE;
        Bitloop(bb) { addPiece<Piece::Rook, !side>(a, SquareOf(bb), kES); }
        bb = qE;
        Bitloop(bb) { addPiece<Piece::Queen, !side>(a, SquareOf(bb), kES); }
    }

    template <bool side>
    inline int evaluate(const Accumulator& acc) {
        const int16_t* accStm = acc.v[side];
        const int16_t* accNstm = acc.v[!side];

        int32_t h[2 * L2];
        for (int j = 0; j < L2; ++j) {
            int64_t s0 = net->l1Bias[0][j];
            int64_t s1 = net->l1Bias[1][j];
            for (int i = 0; i < L1; ++i) {
                s0 += (int64_t)screlu(accStm[i]) * net->l1W[0][i][j];
                s1 += (int64_t)screlu(accNstm[i]) * net->l1W[1][i][j];
            }
            h[j] = screluHidden(s0);
            h[L2 + j] = screluHidden(s1);
        }

        int32_t h2[L3];
        for (int j = 0; j < L3; ++j) {
            int64_t s = net->l2Bias[j];
            for (int i = 0; i < 2 * L2; ++i)
                s += (int64_t)h[i] * net->l2W[i][j];
            h2[j] = screluHidden(s);
        }

        int64_t out = net->outBias;
        for (int i = 0; i < L3; ++i)
            out += (int64_t)h2[i] * net->outW[i];

        return (int)((out / (QA * QB)) * SCALE / QA);
    }
}