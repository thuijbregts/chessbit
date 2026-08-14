#ifndef CUI_H
#define CUI_H

#include "Game.h"
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;
using namespace game;

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
	void play(vector<string>& cmd);
	void undo();
	void showMoves();
	void reset();
	void printBoard();
	void setBoard(vector<string>& cmd, int size);
	void getFen();
	void perft(vector<string>& cmd);
	void perftFast(int depth);
	void perftDivide(int depth, int threads);
	__forceinline U64 divide(int depth, int threads);
	void test();
	void perftsuite();
	void bestmove(vector<string>& cmd);
	void benchmark(string& depth, string& amount);
	void executeBenchmark(int depth, int amount, bool print);
	void compare();
	void compareSearch();
	void toggleTT(vector<string>& cmd);

	U64 generateMoves(int depth);
	U64 generateMoves(int depth, const BoardState& board);
	template <bool side, uint8_t kMoved>
	U64 generateMoves(int depth, const BoardState& board);

	void iteratePieces(U64 p, U64 n, U64 b, U64 r, U64 q);
	void pieces();
	void help();
};

struct Command {
	string command;
	string options;
};

struct PerftTest {
	const char* fen;
	int depth;
	int searchDepth;
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
	const string BEST_MOVE = "bestmove";
	const string BENCHMARK = "benchmark";
	const string COMPARE = "cmp";
	const string COMPARE_SEARCH = "cmps";
	const string TT = "tt";

	const string PERFT_D = "-d";
	const string PERFT_F = "-f";
	const string PERFT_T = "-t";

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
		{ BEST_MOVE,{} },
		{ BENCHMARK,{} },
		{ COMPARE,{} },
		{ COMPARE_SEARCH,{} },
		{ TT,{} }
	};

	const PerftTest TESTS[]{
		{ StartPosition, 7, 10, 3195901860 },
		{ KiwiPete, 6, 10, 8031647685 },
		{ EndGame, 6, 10, 849167880 },
		{ Pos3, 8, 12, 3009794393 },
		{ Pos4, 6, 10, 706045033 },
		{ Pos5, 5, 11, 89941194 },
		{ Pos6, 6, 10, 6923051137 }
	};
}

#endif