# chessbit - The fastest Perft engine (c++)

This project was inspired by [Gigantua](https://github.com/Gigantua/Gigantua), and a desire to push the limits. Although most of the logic is my own, I had no idea about bmi instructions and templates before starting the project, so Gigantua's source code was of immense help to discover these concepts. Credit where credit is due! You will find some code that I took from there.

Here are some numbers on an AMD Ryzen 7 9800x3d. Chessbit is able to calculate some positions at over 4BNodes/s on this CPU (~25-30% increase from Gigantua)

![](https://i.imgur.com/NQqTkCE.png)

## A little about the implementation...
### Overall
The engine is written in c++, using bitboards as the board representation, as well as making use of the **Parallel Bits Extract** (PEXT) bmi2 instruction for the bishop and rook attack tables.
The code is compiled with the Intel c++ compiler 2025 (incorporated into Visual Studio) with the following options:

/GS /GA /W3 /Gy /Zc:wchar_t /Zi /O3 /Ob2 /D "NDEBUG" /D "_CONSOLE" /D "__INTEL_LLVM_COMPILER=20250201" /D "_UNICODE" /D "UNICODE" /Qipo /Zc:forScope /std:c17 /Oi /MD /std:c++20 /Fa"x64\Release\" /EHsc /nologo /Fo"x64\Release\" /Ot /Fp"x64\Release\chessbit.pch" /bigobj -mbmi2 -mbmi -D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH /QxAVX2 

The Intel compiler is doing a great job at optimizing the code. I recommend.

### Structure
After much optimization, the code is actually quite compact, with a main recursive function doing the heavy lifting.
It is structured as follows:

1. **King moves**  
   - Because no matter how many checks, the only piece that can always move is the King.
2. **Checks evaluation**  - Return if there are 2 checks, as only the King can move.

   a. **Find pins**  
      - Split between Bishop and Rook pins.  
      - The pinned pieces will only be able to move into their respective pin masks generated here.
        
   b. **Check check piece**  
      - In case of pawn or knight, only captures are possible, so we have a specific branch for that.  
      - In other cases, generate normal moves (minus en passant, because if a single check occurs that is not a pawn, then en passant is not possible), into the pin masks.
4. **If there are no checks**  
   a. **Find pins**
   
   b. **Generate all moves**  
      - The pieces are efficiently pruned based on the pins (for example, a pinned knight cannot move, so we can easily ignore it).
        
   c. **Castling**

### Pins
The idea of pin masks was an old idea I had, which I improved upon in this project.
The idea is simple: 
1. Iterate over enemy Bishops and Rooks, that are not checking the King, and that are in the same ray as the King.
2. For each piece, check with a precalculated table of pin masks, if the squares between the piece and the King have exactly one bit set. If yes, then we add it to our pins (we don't care if it's an enemy, useful for performance)
3. The result is a uint64 for both Bishops and Rooks, with at most 4 pin masks starting from the King square (4 directions).
   Note: only the queen can target two different pin masks, but this is trivially solved with point 4
4. I also use a global table with a "depth" index, to store the pin masks for each piece, which I then use as my pinned piece attacks (instead of the costly PEXT lookup)
   
The precalculated table is a matrix of 64*64 squares (slider square & king square), where the bits are only set between 2 squares if they are in the same ray.

<pre> ```
template <int depth>
ForceInline U64 iteratePieces(U64 pieces, U64 occB, int kMS) {
    U64 pins = 0ULL;

    Bitloop(pieces)
    {
        int sliderSquare = SquareOf(pieces);

        U64 pinMask = PIN_MASKS[sliderSquare][kMS];
        U64 pinnedPieces = pinMask & occB;

        if (Bitcount(pinnedPieces) == 1) {
            U64 attacks = pinMask | SQUARE_BITS[sliderSquare];
            validAttacksMasks[depth][SquareOf(pinnedPieces)] = attacks ^ pinnedPieces;
            pins |= attacks;
        }
    }

    return pins;
}

template <int depth>
ForceInline U64 findBishopPins(const BoardState& board) {
    return iteratePieces<depth>((board.bE | board.qE) & BISHOP_XRAYS[board.kMS] & ~board.checks, board.occB, board.kMS);
}

template <int depth>
ForceInline U64 findRookPins(const BoardState& board) {
    return iteratePieces<depth>((board.rE | board.qE) & ROOK_XRAYS[board.kMS] & ~board.checks, board.occB, board.kMS);
}
``` </pre>

### Pawn moves
The initial idea was to create an attack table which would allow to iterate over pawns without checking left/right/forward individually. This proved to be much faster than the naive implementation. However, I recently thought about bitshifts, as for some reason I thought they were acting on a single bit at a time. This is great, because we don't have to iterate over pawns anymore. A single shift left, forward, and right is enough to handle all attacks at once (after pruning illegal moves). This was the last huge improvement, as I hadn't realized I was bottlenecked with the initial implementation (~20% boost).

<pre> ```
const U64 pawnsAtk = board.pM & ~rPins;
const U64 pawnsPush = board.pM & ~bPins;

const U64 pawnsLeftAll = pawnsAtkLeft<side>(pawnsAtk & ~bPins & ~FIRST_COL) | (pawnsAtkLeft<side>(pawnsAtk & bPins & ~FIRST_COL) & bPins);
U64 pawnsLeft = pawnsLeftAll & board.occE;
const U64 pawnsRightAll = pawnsAtkRight<side>(pawnsAtk & ~bPins & ~LAST_COL) | (pawnsAtkRight<side>(pawnsAtk & bPins & ~LAST_COL) & bPins);
U64 pawnsRight = pawnsRightAll & board.occE;
U64 pawnsFwd = (pawnsAtkForward<side>(pawnsPush & ~rPins) & ~board.occB) | (pawnsAtkForward<side>(pawnsPush & rPins) & ~board.occB & rPins);
U64 pawnsDouble = pawnsAtkForward<side>(pawnsFwd & FIRST_PUSH_RANK[side]) & ~board.occB;
``` </pre>

This is all you need to have legal pawn moves, ready to be counted (or executed for depth > 1)

### Filter King attacks
King moves are annoying, because you need to verify whether a square is not attacked by an enemy piece. I have improved this function many times to the point I no longer see how to improve it, unless I would find a brand new way to solve this problem.
Because Castling also requires to check attacks on the squares the King goes through, I added it in the same function. Because why calculate twice the enemy moves? This was a big improvement on performance.
Note: I also tried this approach for pins, but it turned out to be slower.

An idea I had here was to add precalculated tables of piece "zones", using PEXT and the King attacks as a mask, for Knights, Bishops and Rooks (Pawns are slightly faster with simple bit shifts).
The idea is to build a mask, based on the ally occupancy around the king. If a square is taken, then there is no need to calculate attacks on it. What remains is a mask of only the possible squares that could attack the free squares, while the ally occupancy blocks the rays.

Here is an example, with red dots for the Bishop mask, and red for the Rook. As you can see, both rooks here are ignored because they cannot possibly attack the King squares, which saves a lot in calculations.
![](https://i.imgur.com/RjqXfCB.png)

### Castling
I tried several approaches, with 2 good candidates. 
The one that is currently implemented is like this:
1. 2 template boolean parameters that indicate whether the kings have already moved or not - these are trivially set based on King moves (+ Castling)
2. Use these to determine, "for free", if we can castle or not (obviously, if a King has moved, it is not possible to castle anymore).
3. We still keep track of castling permissions, but this is easily done for each move. We remove permissions when a Rook or King moves, and also when the initial square of Rooks are taken. No IFs needed.
4. The Castling function is just a combination of: check castling permissions + squares between King and Rook empty + attacks on King squares (calculated in Filter King attacks)

The other approach would be to use the castling permissions as a template paramter, but this costs in terms of size (16 values vs 4), and I found that bigger code size sometimes means less performance, even though "constexpr" conditions are free at runtime. The compilation time also increases a lot, up to 12 minutes, so I discarded this approach as it was giving very similar results (TBConfirmed).

<pre> ```
template <int castlingSide>
ForceInline bool castle(const BoardState& board, U64 attacks) {
    return !(!(board.casPerms & CASTLING[castlingSide]) | (CASTLING_OCCUPIED_SQUARES[castlingSide] & board.occB) | (attacks & CASTLING_PASSING_SQUARES[castlingSide]));
}
``` </pre>

### En passant pin mask
Just a note on this. I haven't found a better way to do this, but I'm not entirely convinced this is the best approach.
En passant moves have a special case, where they can be effectively pinned without being the only piece in the ray between the King and a Rook/Queen (on the EP rank). That extra piece being the en passant pawn that is to be taken. So when iterating over en passant moves, I also make sure that it is not pinned, as the generic Pins function cannot detect that.

<pre> ```
template <bool side>
ForceInline U64 passantPinMask(const BoardState& board, int from) {
    if (!(EN_PASSANT_RANK[side] & board.kM)) {
        return FULL_BOARD;
    }

    U64 occB = board.occB;
    int enemyPawn = board.eP + PAWN_PUSH[!side];
    PopBit(occB, enemyPawn);
    PopBit(occB, from);

    return PASSANT_PIN_RESULT[SquareOf(getRookAttacks(board.kMS, occB) & (board.rE | board.qE))];
}
``` </pre>

This just returns a mask (either 0 or all 1s if not pinned) that is then used to validate the EP bit.

### Other improvements

1. The initial implementation was relying on global variables. I came to understand in this project that accessing global variables/arrays has a big cost on performance. I had a massive performance increase when I refactored the code to pass all the game information in the parameters of the recursive function
2. On the same note, I had first passed every variable as a parameter, which was a lot (12 for each piece, 3 for occupancies, etc), and was a big mess in the code to debug as well. This is when I decided to use a struct to store the elements. As a Java developer, I had no idea instantiating a struct was basically free in c++. This gave a small but significant improvement over the "full params", and made the code much cleaner as well.
3. __forceinline everywhere! At least as much as possible. The performance gain cannot be overstated, this is a must do.
4. BMI instructions, as stated earlier, are also a must. This basically made my engine 2 to 3 times faster, just by changing naive loops to _blsr_ instructions, for example.

### Conclusion
That's basically it. The implementation is in the end not that complex. However the journey to reach this result was! I came from A LOT of code, down to something more concise but this took a lot of trying, doing, undoing and redoing before I could get to a satisfying result.
This is probably my favorite project ever, and I'm even sad that I don't really see what else to do with it... perhaps an actual chess engine based on this is next!

Thanks for reading!
