#include "Definitions.h"
#include <string>

using namespace defs;
using std::string;

struct MoveInfo;

namespace game {
    extern MoveInfo PAWN_MOVES[7][64][64];
    extern MoveInfo KNIGHT_MOVES[7][64][64];
    extern MoveInfo BISHOP_MOVES[7][64][64];
    extern MoveInfo ROOK_MOVES[7][64][64];
    extern MoveInfo QUEEN_MOVES[7][64][64];
    extern MoveInfo KING_MOVES[7][64][64];
    extern MoveInfo PROMO_KNIGHT_MOVES[7][64][64];
    extern MoveInfo PROMO_BISHOP_MOVES[7][64][64];
    extern MoveInfo PROMO_ROOK_MOVES[7][64][64];
    extern MoveInfo PROMO_QUEEN_MOVES[7][64][64];

    extern MoveInfo EN_PASSANT_MOVES[64][64];
    extern MoveInfo CASTLING_MOVES[64];

    /*extern U64 bishopPins[5248];
    extern U64 rookPins[102400];
    extern U64 queenAttacks[6946816];*/

    /*extern U64 castleAttacksBishop[2][2][16384];
    extern U64 castleAttacksRook[2][2][16384];*/

    extern int moveCount;
    extern MoveInfo movesPlayed[5949];

    extern U64 pieces[2][7];
    extern U64 occupancies[3];
    extern int boardPieces[65];
    extern bool side;
    extern int enPassant;
    extern int castlingPermissions;
    extern U64 checks;

    
    //all the possible pin masks, where first index is the slider square and second index is the enemy king square
    //both squares are set to 0 in the mask
    //65 for the pinMask in case of check, when there is only one check piece
    extern U64 pinMasks[65][64];
    extern U64 pinRays[64][64];
    //returns all the bits before/after a possible pinned piece, on the same line/diagonal as the enemy king
    //the mask is then used with the occupancy to get the least/most significant bit, which is the potential pinner
    //first index is the pinned piece, second index is the enemy king
    extern U64 potentialPinnerMasks[64][64];
    //used as a mask to ignore pieces that are not in the zone
    //the zone is for each square around the king, the possible squares (for a given piece) that could attack the king square
    /*extern U64 pawnAttackZones[2][64][8192];
    extern U64 pawnZones[2][64];*/
    extern U64 knightAttackZones[64];
    extern U64 bishopAttackZones[2][4096];
    extern U64 rookAttackZones[2][1024];
    //for check checks
    //first 64 is for move.to, second is for ennemy king position, returns move.to if the pawn can attack the king, otherwise noSquare
    extern int pawnChecks[2][64][64];
    extern int knightChecks[64][64];

    extern U64 pawnAttacks[2][4096];

    void init();
    void printBoard(U64 bitboard);
    void printBoard();
    void printPieces();

    U64 setOccupancy(int index, int bits_in_mask, U64 attack_mask);

    void initMovesInfo();
    void initMoveInfo(MoveInfo& move, int type, int from, int to, int movedPiece, int enPassant, int deadPiece, int promotedPiece, int sourceRook, int targetRook, int mvvLva);

    void initCastleAttacks();
    U64 castleBishopWithOccupancy(int square, U64 occupancy);
    U64 castleRookWithOccupancy(int square, U64 occupancy);
    void initCastleAttacksBishop(int side, int castlingSide);
    void initCastleAttacksRook(int side, int castlingSide);

    void initSliderAttacks();
    void initPawnAttacks(bool side);
    U64 pawnAttacksWithOccupancy(bool side, U64 occupancy);
    void initBishopPins(int square);
    void initRookPins(int square);
    void initQueenAttacks(int square);
    U64 bishopPinsWithOccupancy(int square, U64 occupancy);
    U64 rookPinsWithOccupancy(int square, U64 occupancy);
    U64 queenAttacksWithOccupancy(int square, U64 occupancy);

    void initPinMasks();
    void initPotentielPinnerMasks();
    void initAttackZones();
    void initBishopZone(int square, bool side, U64 mask);
    void initRookZone(int square, bool side, U64 mask);
    U64 bishopZoneWithOccupancy(int square, bool side, U64 occupancy);
    U64 rookZoneWithOccupancy(int square, bool side, U64 occupancy);
    void initChecks();

    void setFen(const char* fen);
    string getFen();

    template <bool side>
    ForceInline U64 getPawnAttacks(int square, U64 occB, U64 occE) {
        return PAWN_ATTACKS_LOOKUP[side][square][occB];
        //return PAWN_ATTACKS[PAWN_OFFSETS1[side][square] + _pext_u64(occB, PAWN_MASKS1[side][square])];
    }

    template <bool side>
    ForceInline int getPawnAttacksCount(int square, U64 occB, U64 occM) {
        //U64 occupancy = occupancyEnemy | (occupancyBoth & PAWN_FRONT_MASKS[side][square]);
        //slightly faster?
        U64 occupancy = occB ^ (occM & PAWN_CAPTURES[side][square]);
        return PAWN_ATTACKS_COUNT_LOOKUP[side][square][occupancy];
    }

    template <bool side>
    ForceInline int getPawnAttacksCountCheckLeaper(int square, U64 check) {
        U64 occupancy = check | PAWN_FRONT_MASKS[side][square];
        return PAWN_ATTACKS_COUNT_LOOKUP[side][square][occupancy];
    }

    template <bool side>
    ForceInline int getPawnAttacksCountPinned(int square, U64 occB, U64 occE, U64 pinMask) {
        U64 occupancy = (occE & pinMask) | (PAWN_FRONT_MASKS[side][square] & (occB | ~pinMask));
        return PAWN_ATTACKS_COUNT_LOOKUP[side][square][occupancy];
    }

    template <bool side>
    ForceInline U64 getPawnKingAttacks(int kingSquare, U64 occupancy) {
        return PAWN_KING_ATTACKS_LOOKUP[side][kingSquare][occupancy];
    }

    template <bool side>
    ForceInline U64 getPawnKingAttacksCastle(U64 occupancy) {
        return PAWN_KING_ATTACKS_CASTLE[side][_pext_u64(occupancy, PAWN_KING_MASK_CASTLE[side])];
    }

    ForceInline U64 getBishopAttacks(int square, U64 occupancy) {
        return BISHOP_ATTACKS_LOOKUP[square][occupancy];
        //return BISHOP_ATTACKS[BISHOP_OFFSETS[square] + _pext_u64(occupancy, BISHOP_MASKS[square])];
    }

    ForceInline U64 getRookAttacks(int square, U64 occupancy) {
        return ROOK_ATTACKS_LOOKUP[square][occupancy];
        //return ROOK_ATTACKS[ROOK_OFFSETS[square] + _pext_u64(occupancy, ROOK_MASKS[square])];
    }

    ForceInline U64 getQueenAttacks(int square, U64 occupancy) {
        return getBishopAttacks(square, occupancy) | getRookAttacks(square, occupancy);
        //return queenAttacks[QUEEN_OFFSETS[square] + _pext_u64(occupancy, QUEEN_MASKS[square])];
        //return QUEEN_ATTACKS_LOOKUP[square][occupancy];
    }

    ForceInline U64 getKingAttacks(int square) {
        return KING_ATTACKS[square];
    }

    ForceInline U64 getKnightAttacks(int square) {
        return KNIGHT_ATTACKS[square];
    }

    template <int castlingSide>
    ForceInline U64 getRookCastleAttacks(U64 occupancy) {
        return CASTLE_ATTACKS_ROOK[castlingSide][_pext_u64(occupancy, CASTLE_MASKS_ROOK[castlingSide])];
    }

    template <int castlingSide>
    ForceInline U64 getBishopCastleAttacks(U64 occupancy) {
        return CASTLE_ATTACKS_BISHOP[castlingSide][_pext_u64(occupancy, CASTLE_MASKS_BISHOP[castlingSide])];
    }

    ForceInline U64 getBishopAttackZone(int square, U64 occupancy, U64 mask) {
        //return BISHOP_ATTACK_ZONES_LOOKUP[square][occupancy];
        return BISHOP_ATTACK_ZONES[square][_pext_u64(occupancy, mask)];
    }

    ForceInline U64 getRookAttackZone(int square, U64 occupancy, U64 mask) {
        //return ROOK_ATTACK_ZONES_LOOKUP[square][occupancy];
        return ROOK_ATTACK_ZONES[square][_pext_u64(occupancy, mask)];
    }

    template <bool side>
    ForceInline U64 getBishopAttackZoneCastle(U64 occupancy) {
        return BISHOP_ATTACK_ZONE_CASTLE[side][_pext_u64(occupancy, BISHOP_ATTACK_ZONE_CASTLE_MASK[side])];
    }

    template <bool side>
    ForceInline U64 getRookAttackZoneCastle(U64 occupancy) {
        return ROOK_ATTACK_ZONE_CASTLE[side][_pext_u64(occupancy, ROOK_ATTACK_ZONE_CASTLE_MASK[side])];
    }

    ForceInline U64 getBishopPins(int square, U64 occupancy) {
        return BISHOP_PINS_LOOKUP[square][occupancy];
    }

    ForceInline U64 getRookPins(int square, U64 occupancy) {
        return ROOK_PINS_LOOKUP[square][occupancy];
    }

    ForceInline U64 getBishopPinMasks(int square, U64 occupancy) {
        return BISHOP_PIN_MASKS_LOOKUP[square][occupancy];
    }

    ForceInline U64 getRookPinMasks(int square, U64 occupancy) {
        return ROOK_PIN_MASKS_LOOKUP[square][occupancy];
    }
}