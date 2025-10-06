#ifndef CUI_H
#define CUI_H

#include "Game.h"
#include "MoveArray.h"
#include <string>
#include <vector>

using namespace std;
using namespace game;
using namespace moveinfo;
using namespace movarray;

class Cui {

public:
	Cui();
	~Cui();

private:
	void start();

	void initMoves();

	void execute(vector<string>& command);

	bool isCommand(vector<string>& command);
	bool executeMove(string& move);
	void play();
	void undo();
	void showMoves();
	void reset();
	void printBoard();
	void setBoard(vector<string>& cmd, int size);
	void getFen();
	void perft(string& option, string& depth);
	void perftFast(int depth);
	void perftDivide(int depth);	
	__forceinline U64 divide(int depth);
	void test();
	void perftsuite();
	void benchmark(string& depth, string& amount);
	void executeBenchmark(int depth, int amount, bool print);
	void compare();

	U64 generateMoves(int depth, const BoardState& board, MoveArray& movesArray);
	template <bool side, bool kMMoved, bool kEMoved>
	U64 generateMoves(int depth, const BoardState& board, MoveArray& movesArray);

	void iteratePieces(U64 p, U64 n, U64 b, U64 r, U64 q);
	void pieces();
	void help();

	MoveArray movesArray;
};

struct Command {
	string command;
	string options;
};

struct PerftTest {
	const char* fen;
	int depth;
	U64 result;
};

namespace cui {
	const string EXIT = "exit";
	const string HELP = "help";
	const string SET_FEN = "setfen";
	const string GET_FEN = "fen";
	const string PERFT = "perft";
	const string DIVIDE = "divide";
	const string FULL = "full";
	const string PRINT_BOARD = "print";
	const string MOVES = "moves";
	const string RESET = "reset";
	const string PIECES = "pieces";
	const string UNDO = "undo";
	const string PLAY = "play";
	const string TEST = "test";
	const string PERFT_SUITE = "perftsuite";
	const string BENCHMARK = "benchmark";
	const string COMPARE = "cmp";

	const string PERFT_D = "-d";
	const string PERFT_F = "-f";

	const Command COMMANDS[]{
		{ EXIT, {} },
		{ HELP, {} },
		{ SET_FEN, {} },
		{ GET_FEN, {} },
		{ PERFT, {}  },
		{ DIVIDE, {} },
		{ PRINT_BOARD, {} },
		{ MOVES, {} },
		{ RESET, {} },
		{ PIECES, {} },
		{ UNDO, {} },
		{ PLAY,{} },
		{ TEST,{} },
		{ PERFT_SUITE,{} },
		{ BENCHMARK,{} },
		{ COMPARE,{} }
	};

	const PerftTest TESTS[]{
		{ StartPosition, 7, 3195901860 },
		{ KiwiPete, 6, 8031647685 },
		{ EndGame, 6, 849167880 },
		{ Pos3, 8, 3009794393 },
		{ Pos4, 6, 706045033 },
		{ Pos5, 5, 89941194 },
		{ Pos6, 6, 6923051137 }
	};
}

#endif