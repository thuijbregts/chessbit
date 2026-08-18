#include "Test.hpp"
#include "BestMoveTest.h"
#include "Cui.h"
#include "Uci.h"
#include "Perft.h"
#include "Engine.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <future>
#include <vector>
#include <utility>
#include <cstdio>
#include <filesystem>

using namespace std::chrono;
using namespace movegen;
using namespace game;
using namespace tt;
using namespace perft;

Cui::Cui()
{
	start();
}

Cui::~Cui()
{

}

void Cui::start() {
	string input;
	vector<string> cmd;
	initMoves();
	cout << "+---+---+---+---+---+---+---+---+---+---+" << endl;
	cout << "|     chessbit by Thomas Huijbregts     |" << endl;
	cout << "+---+---+---+---+---+---+---+---+---+---+" << endl;

	help();
	do {
		getline(cin, input);
		cmd = utils::split(input, ' ');
		execute(cmd);

	} while (true);
}

void Cui::initMoves() {
	batch::batch.reset();
	generateMoves(1, &batch::batch);
}

bool Cui::isCommand(vector<string>& cmd) {
	int size = cmd.size();
	if (size == 0) {
		return false;
	}

	if (cmd[0].length() == 0) {
		return false;
	}

	string first = cmd[0];
	for (Command CMD : cui::COMMANDS) {
		if (CMD.command == first) {
			return true;
		}
	}

	if (utils::validMove(first)) {
		if (executeMove(first)) {
			initMoves();
		}
		else {
			cout << "Move does not exist for this position" << endl;
		}
		return true;
	}

	cout << "Invalid command... type 'help' for command list" << endl;
	return false;
}

void Cui::execute(vector<string>& cmd) {

	if (!isCommand(cmd)) {
		return;
	}

	string first = cmd[0];

	if (first == cui::PLAY) {
		play(cmd);
		return;
	}

	if (first == cui::UCI) {
		uci::loop();
		return;
	}

	if (first == cui::UNDO) {
		undo();
		return;
	}

	if (first == cui::MOVES) {
		showMoves();
		return;
	}

	if (first == cui::RESET) {
		reset();
		return;
	}

	if (first == cui::PRINT_BOARD) {
		printBoard();
		return;
	}

	if (first == cui::GET_FEN) {
		getFen();
		return;
	}

	if (first == cui::SET_FEN) {
		setBoard(cmd, cmd.size());
		return;
	}

	if (first == cui::PERFT) {
		perft(cmd);
		return;
	}

	if (first == cui::TT) {
		toggleTT(cmd);
		return;
	}

	if (first == cui::TEST) {
		test();
		return;
	}

	if (first == cui::PERFT_SUITE) {
		perftsuite();
		return;
	}

	if (first == cui::BEST_MOVE) {
		bestmove(cmd);
		return;
	}

	if (first == cui::BENCHMARK) {
		string param1, param2;
		if (cmd.size() > 1) param1 = cmd[1];
		if (cmd.size() > 2) param2 = cmd[2];
		benchmark(param1, param2);
		return;
	}

	if (first == cui::COMPARE) {
		compare();
		return;
	}

	if (first == cui::COMPARE_SEARCH) {
		compareSearch();
		return;
	}

	if (first == cui::PIECES) {
		pieces();
		return;
	}

	if (first == cui::HELP) {
		help();
		return;
	}

	if (first == cui::EXIT) {
		exit(0);
	}
}

bool Cui::executeMove(string& move) {
	BoardState* mv = batch::batch.moves;

	for (int i = 0; i < batch::batch.size; ++i) {
		if (move == utils::getMoveSimple(mv[i])) {
			game::makeMove(mv[i]);
			return true;
		}
	}
	return false;
}

void Cui::play(vector<string>& cmd) {
	int depth = 10;
	long long budget = 0;
	for (size_t i = 1; i < cmd.size(); i++) {
		string& tok = cmd[i];
		if (tok == cui::ARG_T) {
			if (i + 1 < cmd.size() && utils::isPositiveDigits(cmd[i + 1]) && stoi(cmd[i + 1]) > 0) {
				budget = stoi(cmd[++i]);
			}
			else {
				cout << "Incorrect time value" << endl;
				return;
			}
		}
		else if (utils::isPositiveDigits(tok) && stoi(tok) > 0) {
			depth = stoi(tok);
		}
		else {
			cout << "Invalid argument" << endl;
			return;
		}
	}

	if (budget > 0) depth = MAX_PLY;

	high_resolution_clock::time_point start, end;

	start = high_resolution_clock::now();
	BoardState bestMove;
	if (game::board.side == white) engine::start<white>(depth, game::board, &bestMove, budget);
	else						   engine::start<black>(depth, game::board, &bestMove, budget);
	end = high_resolution_clock::now();

	long long total = duration_cast<microseconds>(end - start).count();

	cout << "Time:\t\t" << total / 1000 << " ms" << endl;

	game::makeMove(bestMove);
}

void Cui::undo() {
	if (game::moveCount > 0) {
		game::unmakeMove();
		initMoves();
	}
	else {
		cout << "No move to undo" << endl;
	}
}

void Cui::showMoves() {
	BoardState* mv = batch::batch.moves;
	for (int i = 0; i < batch::batch.size; ++i) {
		cout << utils::getMoveSimple(mv[i]) << endl;
	}
	cout << "Moves:\t" << batch::batch.size << endl;
}

void Cui::printBoard() {
	game::printBoard(game::board);
}

void Cui::reset() {
	setFen(StartPosition);

	initMoves();
}

void Cui::setBoard(vector<string>& cmd, int size) {
	string fen;
	for (int i = 1; i < size; ++i) {
		fen += cmd[i];
		if (i < size - 1) {
			fen += " ";
		}
	}
	try {
		setFen(fen.c_str());
		initMoves();
	}
	catch (invalid_argument& e) {
		cout << "Error while setting fen: " << e.what() << endl;
	}
}

void Cui::getFen() {
	cout << game::getFen(game::board) << endl;
}

void Cui::toggleTT(vector<string>& cmd) {
	int ttSize = 0;
	if (cmd.size() >= 2) {
		if (utils::isPositiveDigits(cmd[1]) && stoi(cmd[1]) > 0) {
			ttSize = stoi(cmd[1]);
		}
		else {
			cout << "Incorrect size value" << endl;
			return;
		}
	}

	ttEnabled = !ttEnabled;

	if (ttEnabled) {
		if (ttSize > 0) tt::init(ttSize);
		else			tt::init();
	}
	else {
		tt::free();
	}
}

void Cui::perft(vector<string>& cmd) {
	bool divideMode = false;
	int depth = -1;
	int threads = 0;

	for (size_t i = 1; i < cmd.size(); i++) {
		string& tok = cmd[i];
		if (tok == cui::ARG_D) {
			divideMode = true;
		}
		else if (tok == cui::ARG_T) {
			if (i + 1 < cmd.size() && utils::isPositiveDigits(cmd[i + 1]) && stoi(cmd[i + 1]) > 0) {
				threads = stoi(cmd[++i]);
			}
			else {
				cout << "Incorrect thread value" << endl;
				return;
			}
		}
		else if (utils::isPositiveDigits(tok) && stoi(tok) > 0) {
			depth = stoi(tok);
		}
		else {
			cout << "Incorrect perft option" << endl;
			return;
		}
	}

	if (depth <= 0) {
		cout << "incorrect depth value" << endl;
		return;
	}

	if (divideMode) {
		perftDivide(depth, threads);
	}
	else {
		perftFast(depth);
	}
}

void Cui::perftFast(int depth) {
	string fen = game::getFen(game::board);

	high_resolution_clock::time_point start, end;

	start = high_resolution_clock::now();
	U64 nodes = generateMoves(depth);
	end = high_resolution_clock::now();

	long long total = duration_cast<microseconds>(end - start).count();

	cout << "Depth:\t\t" << depth << endl;
	cout << "Nodes:\t\t" << nodes << endl;
	cout << "Time:\t\t" << total / 1000 << " ms" << endl;
	if (total > 0) {
		cout << "Average:\t" << (nodes * 1.0 / total) << " Mn/s" << endl;
	}

	game::setFen(fen.c_str());
}

void Cui::perftDivide(int depth, int threads) {
	string fen = game::getFen(game::board);

	high_resolution_clock::time_point start, end;

	start = high_resolution_clock::now();
	U64 nodes = divide(depth, threads);
	end = high_resolution_clock::now();

	long long total = duration_cast<microseconds>(end - start).count();

	cout << "Depth:\t\t" << depth << endl;
	cout << "Nodes:\t\t" << nodes << endl;
	cout << "Time:\t\t" << total / 1000 << " ms" << endl;
	if (total > 0) {
		cout << "Average:\t" << (nodes * 1.0 / total) << " Mn/s" << endl;
	}

	game::setFen(fen.c_str());

	initMoves();
}

__forceinline U64 Cui::divide(int depth, int threads) {
	U64 totalNodes = 0;
	U64 moveNodes;

	int size = batch::batch.size;
	if (depth == 1)
		return size;

	BoardState* m = batch::batch.moves;

	for (int i = 0; i < size; i++) {
		moveNodes = generateMoves(depth - 1, m[i]);

		printf("%s %llu\n", utils::getMoveSimple(m[i]).c_str(), moveNodes);
		totalNodes += moveNodes;
	}

	return totalNodes;
}

void Cui::test() {
	string fen = game::getFen(game::board);

	high_resolution_clock::time_point start, end;
	U64 nodes;
	int success = 0;
	int tests = 0;

	for (PerftTest test : cui::TESTS) {
		cout << "Position:\t\t" << test.fen << endl;
		cout << "Depth:\t\t\t" << test.depth << endl;
		setFen(test.fen);
		start = high_resolution_clock::now();
		nodes = generateMoves(test.depth);
		end = high_resolution_clock::now();

		long long total = duration_cast<microseconds>(end - start).count();

		cout << "Nodes:\t\t\t" << nodes << endl;
		cout << "Expected nodes:\t\t" << test.result << endl;
		cout << "Time:\t\t\t" << total / 1000 << " ms" << endl;
		if (nodes == test.result) {
			cout << "Test Succeeded!" << endl;
			success++;
		}
		else {
			cout << "Test Failed!" << endl;
		}
		cout << "--------------------------" << endl;

		tests++;
	}

	cout << "Final results:\t\t" << success << "/" << tests << endl;

	game::setFen(fen.c_str());
}

void Cui::perftsuite() {
	string fenS = game::getFen(game::board);

	int success = 0;
	int tests = 0;
	for (auto pos : test::Positions)
	{
		auto v = test::GetElements(pos, ';');
		std::string fen = v[0];
		cout << fen << endl;
		int to = v.size();
		for (int i = 1; i < to; i++) {
			game::setFen(fen.c_str());

			auto perftvals = test::GetElements(v[i], ' ');
			U64 expected = static_cast<U64>(std::strtol(perftvals[1].c_str(), NULL, 10));
			U64 result = generateMoves(i);
			std::string status = expected == result ? "OK" : "ERROR";
			if (expected == result) {
				cout << "   " << i << ": " << result << " " << status << endl;
				success++;
			}
			else  cout << "xxx -> " << i << ": " << result << " vs " << expected << " " << status << endl;

			tests++;
		}
	}
	cout << "Final results:\t\t" << success << "/" << tests << endl;

	game::setFen(fenS.c_str());
}

void Cui::bestmove(vector<string>& cmd) {
	int requested = 4;
	if (cmd.size() >= 2) {
		if (utils::isPositiveDigits(cmd[1]) && stoi(cmd[1]) > 0) requested = stoi(cmd[1]);
		else {
			cout << "Incorrect depth value" << endl;
			return;
		}
	}

	string fen = game::getFen(game::board);

	int passed = 0;
	long long totalUs = 0;
	long long worstUs = 0;
	string worstFen;

	printf("Best-move suite: %d positions, requested depth %d (min per position enforced)\n", bmtest::COUNT, requested);
	printf("-----------------------------------------------------------------------------\n");
	printf("  #   mate  depth  expected  got     time         result\n");
	printf("-----------------------------------------------------------------------------\n");

	int score;
	for (int i = 0; i < bmtest::COUNT; ++i) {
		const bmtest::BestMoveTest& t = bmtest::TESTS[i];
		const int depth = requested < t.depth ? t.depth : requested;

		game::setFen(t.fen);

		BoardState bestMove;
		engine::clearHeuristics();
		tt::GENERATION++;

		const auto s = high_resolution_clock::now();
		if (game::board.side == white) score = engine::search<white>(depth, game::board, 0, -INF, INF, &bestMove);
		else						   score = engine::search<black>(depth, game::board, 0, -INF, INF, &bestMove);
		const auto e = high_resolution_clock::now();

		const long long us = duration_cast<microseconds>(e - s).count();
		totalUs += us;
		if (us > worstUs) { worstUs = us; worstFen = t.fen; }

		const string got = utils::getMoveSimple(bestMove);
		const bool ok = got.substr(0, 4) == string(t.bm).substr(0, 4);
		if (ok) passed++;

		printf("%3d   #%-3d  %5d  %-8s  %-6s  %8.3f ms   %s\n",
			i + 1, t.mateIn, depth, t.bm, got.c_str(), us / 1000.0, ok ? "OK" : "FAIL");

		if (!ok) printf("        -> FEN: %s   (score %d)\n", t.fen, score);
	}

	printf("-----------------------------------------------------------------------------\n");
	printf("Passed:      %d/%d\n", passed, bmtest::COUNT);
	printf("Total time:  %lld ms\n", totalUs / 1000);
	printf("Average:     %.3f ms/pos\n", (totalUs / 1000.0) / bmtest::COUNT);
	printf("Slowest:     %lld ms  (%s)\n", worstUs / 1000, worstFen.c_str());

	game::setFen(fen.c_str());
}

void Cui::benchmark(string& depth, string& amount) {
	int d = 6;
	int a = 25;
	bool print = true;

	if (depth.length() > 0) {
		if (depth == "all") {
			a = 15;
			print = false;
			string fen = game::getFen(game::board);

			setFen(StartPosition); cout << StartPosition << endl; executeBenchmark(6, a, print);
			setFen(KiwiPete); cout << KiwiPete << endl; executeBenchmark(5, a, print);
			setFen(Pos3); cout << Pos3 << endl; executeBenchmark(7, a, print);
			setFen(Pos4); cout << Pos4 << endl; executeBenchmark(6, a, print);
			setFen(Pos5); cout << Pos5 << endl; executeBenchmark(5, a, print);
			setFen(Pos6); cout << Pos6 << endl; executeBenchmark(5, a, print);
			setFen(EndGame); cout << EndGame << endl; executeBenchmark(6, a, print);

			game::setFen(fen.c_str());
			return;
		}
		else if (utils::isPositiveDigits(depth) && stoi(depth) > 0) {
			d = stoi(depth);
		}
		else {
			cout << "Incorrect depth value" << endl;
			return;
		}
	}

	if (amount.length() > 0) {
		if (utils::isPositiveDigits(amount) && stoi(amount) > 0) {
			a = stoi(amount);
		}
		else {
			cout << "Incorrect amount value" << endl;
			return;
		}
	}

	executeBenchmark(d, a, print);
}

void Cui::executeBenchmark(int depth, int amount, bool print) {
	string fen = game::getFen(game::board);

	high_resolution_clock::time_point start, end;
	long long total, best;

	for (int i = 0; i < amount; i++) {
		start = high_resolution_clock::now();
		auto volatile result = generateMoves(depth);
		end = high_resolution_clock::now();

		total = duration_cast<microseconds>(end - start).count();
		if (print) printf("Time:\t\t%lld\t%llu\n", total / 1000, result);
		if (i == 0) {
			best = total;
		}
		else if (total < best) {
			best = total;
		}
	}

	printf("Best time:\t\t%lld\n", best / 1000);

	game::setFen(fen.c_str());
}

void Cui::compare() {
	string fen = game::getFen(game::board);

	high_resolution_clock::time_point start, end;
	U64 result;
	long long total;

	auto ts = std::chrono::steady_clock::now();
	setFen(StartPosition);
	for (int i = 1; i <= 7; i++)
	{
		start = high_resolution_clock::now();
		result = generateMoves(i);
		end = high_resolution_clock::now();
		total = duration_cast<microseconds>(end - start).count();
		std::cout << "Perft Start " << i << ": " << result << " " << total / 1000 << "ms " << result * 1.0 / total << " MNodes/s\n";
	}
	if (result == 3195901860ull) std::cout << "OK\n\n";
	else std::cout << "ERROR!\n\n";

	setFen(KiwiPete);
	for (int i = 1; i <= 6; i++)
	{
		start = high_resolution_clock::now();
		result = generateMoves(i);
		end = high_resolution_clock::now();
		total = duration_cast<microseconds>(end - start).count();
		std::cout << "Perft Kiwi " << i << ": " << result << " " << total / 1000 << "ms " << result * 1.0 / total << " MNodes/s\n";
	}
	if (result == 8031647685ull) std::cout << "OK\n\n";
	else std::cout << "ERROR!\n\n";

	setFen(Pos6);
	for (int i = 1; i <= 6; i++)
	{
		start = high_resolution_clock::now();
		result = generateMoves(i);
		end = high_resolution_clock::now();
		total = duration_cast<microseconds>(end - start).count();
		std::cout << "Perft Midgame " << i << ": " << result << " " << total / 1000 << "ms " << result * 1.0 / total << " MNodes/s\n";
	}
	if (result == 6923051137ull) std::cout << "OK\n\n";
	else std::cout << "ERROR!\n\n";

	setFen(EndGame);
	for (int i = 1; i <= 7; i++)
	{
		start = high_resolution_clock::now();
		result = generateMoves(i);
		end = high_resolution_clock::now();
		total = duration_cast<microseconds>(end - start).count();
		std::cout << "Perft Endgame " << i << ": " << result << " " << total / 1000 << "ms " << result * 1.0 / total << " MNodes/s\n";
	}
	if (result == 24958831314ull) std::cout << "OK\n\n";
	else std::cout << "ERROR!\n\n";

	U64 nodes = (3195901860ull + 8031647685ull + 6923051137ull + 24958831314ull);
	auto te = std::chrono::steady_clock::now();
	total = duration_cast<microseconds>(te - ts).count();
	std::cout << "Perft aggregate: " << nodes
		<< " " << total / 1000 << "ms " << nodes * 1.0 / total << " MNodes/s\n";

	game::setFen(fen.c_str());
}

void Cui::compareSearch() {
	string fen = game::getFen(game::board);

	high_resolution_clock::time_point start, end;

	start = high_resolution_clock::now();
	for (PerftTest test : cui::TESTS) {
		cout << "Position:\t\t" << test.fen << endl;
		cout << "Depth:\t\t\t" << test.searchDepth << endl;

		setFen(test.fen);

		if (ttEnabled) tt::clear();

		BoardState bestMove;
		if (game::board.side == white) engine::start<white>(test.searchDepth, game::board, &bestMove);
		else						   engine::start<black>(test.searchDepth, game::board, &bestMove);
	}
	end = high_resolution_clock::now();
	long long total = duration_cast<microseconds>(end - start).count();

	cout << "Time:\t\t\t" << total / 1000 << " ms" << endl;

	game::setFen(fen.c_str());
}

U64 Cui::generateMoves(int depth, batch::Batch* batch) {
	return generateMoves(depth, game::board, batch);
}

U64 Cui::generateMoves(int depth, const BoardState& board, batch::Batch* batch) {
	const uint8_t kMoved = (!(board.casPerms & (wk | wq)) ? KING_MOVED[white] : 0)
		| (!(board.casPerms & (bk | bq)) ? KING_MOVED[black] : 0);

	if (board.side == white) {
		switch (kMoved) {
		case KING_MOVED[white]: return perft::start<white, KING_MOVED[white]>(depth, board, batch);
		case KING_MOVED[black]: return perft::start<white, KING_MOVED[black]>(depth, board, batch);
		case KING_MOVED[both]:  return perft::start<white, KING_MOVED[both]>(depth, board, batch);
		default:                return perft::start<white, 0>(depth, board, batch);
		}
	}
	else {
		switch (kMoved) {
		case KING_MOVED[white]: return perft::start<black, KING_MOVED[white]>(depth, board, batch);
		case KING_MOVED[black]: return perft::start<black, KING_MOVED[black]>(depth, board, batch);
		case KING_MOVED[both]:  return perft::start<black, KING_MOVED[both]>(depth, board, batch);
		default:                return perft::start<black, 0>(depth, board, batch);
		}
	}
}

void Cui::iteratePieces(U64 p, U64 n, U64 b, U64 r, U64 q) {
	cout << "Type: PAWN\t\tCount: " << Bitcount(p) << endl;
	cout << "Type: KNIGHT\t\tCount: " << Bitcount(n) << endl;
	cout << "Type: BISHOP\t\tCount: " << Bitcount(b) << endl;
	cout << "Type: ROOK\t\tCount: " << Bitcount(r) << endl;
	cout << "Type: QUEEN\t\tCount: " << Bitcount(q) << endl;
}

void Cui::pieces() {
	cout << "WHITE:" << endl;
	if (game::board.side == white) iteratePieces(game::board.pM, game::board.nM, game::board.bM, game::board.rM, game::board.qM);
	else						iteratePieces(game::board.pE, game::board.nE, game::board.bE, game::board.rE, game::board.qE);

	cout << "BLACK:" << endl;
	if (game::board.side == black) iteratePieces(game::board.pM, game::board.nM, game::board.bM, game::board.rM, game::board.qM);
	else						iteratePieces(game::board.pE, game::board.nE, game::board.bE, game::board.rE, game::board.qE);
}

void Cui::help() {
	cout << "\n******Moves******\n" << endl;
	cout << "Make a move by entering the source and destination square" << endl;
	cout << "Example: a2a3\tAdd Q,R,B,N for promotions" << endl;
	cout << "\n******Transposition Table******\n" << endl;
	cout << (ttEnabled ? "ON" : "OFF") << endl;
	cout << "\n******Commands******\n" << endl;
	cout << "play\t\tPlays the best move for a given depth" << endl;
	cout << "undo\t\tCancels the last move made" << endl;
	cout << "moves\t\tPrints the legal moves of the current position" << endl;
	cout << "reset\t\tResets the game to its initial position" << endl;
	cout << "print\t\tPrints the board representation of the current position" << endl;
	cout << "fen\t\tPrints the FEN record of the current position" << endl;
	cout << "setfen\t\tSets the position given by a FEN record" << endl;
	cout << "perft\t\tGenerates all moves down to a given depth" << endl;
	cout << "\t-d\tShows total of moves for each current legal move (multi-thread)" << endl;
	cout << "\t-t\tNumber of threads for -d (default: max available threads)" << endl;
	cout << "tt\t\tToggles the transposition table on/off (tt [size])" << endl;
	cout << "cmp\t\tIterates over popular positions and provides an average" << endl;
	cout << "cmps\t\tSearches popular positions and provides an average" << endl;
	cout << "test\t\tTests popular positions to validate perft results" << endl;
	cout << "perftsuite\tTests full list of positions to validate perft results" << endl;
	cout << "bestmove\tRuns the engine on known forced mates and checks the move (bestmove [depth])" << endl;
	cout << "benchmark\tPerforms a series of perft and returns the best time" << endl;
	cout << "\t\tSpecify depth then amount. Default is 6 and 25" << endl;
	cout << "pieces\t\tPrints the pieces' count for each side" << endl;
	cout << "exit\t\tExit the application" << endl;
}