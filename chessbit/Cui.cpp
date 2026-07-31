#include "Test.hpp"
#include "Cui.h"
#include "MoveGenerator.h"
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
using namespace movarray;
using namespace tt;

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
	movesArray.reset();
	generateMoves(0);
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
		play();
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
	int size = movesArray.size();
	MoveInfo* mv = movesArray.moves();

	for (int i = 0; i < size; ++i) {
		if (move == utils::getMoveSimple(mv[i])) {
			game::makeMove(mv[i]);
			return true;
		}
	}
	return false;
}

void Cui::play() {
	/*game::makeMove(search::searchMove());
	generateMoves<false>();*/
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
	int size = movesArray.size();
	MoveInfo* mv = movesArray.moves();
	for (int i = 0; i < size; ++i) {
		cout << utils::getMoveSimple(mv[i]) << endl;
	}
	cout << "Moves:\t" << size << endl;
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
	cout << game::getFen() << endl;
}

void Cui::toggleTT(vector<string>& cmd) {
	if (cmd.size() >= 2) {
		if (cmd[1] == "on")       ttEnabled = true;
		else if (cmd[1] == "off") ttEnabled = false;
		else {
			cout << "usage: tt [on|off]" << endl;
			return;
		}
	}
	else {
		ttEnabled = !ttEnabled;
	}

	cout << "Transposition table: " << (ttEnabled ? "ON" : "OFF") << endl;
}

void Cui::perft(vector<string>& cmd) {
	bool divideMode = false;
	int depth = -1;
	int threads = 0;
	int ttSize = 0;

	for (size_t i = 1; i < cmd.size(); i++) {
		string& tok = cmd[i];
		if (tok == cui::PERFT_D) {
			divideMode = true;
		}
		else if (tok == cui::PERFT_T) {
			if (i + 1 < cmd.size() && utils::isPositiveDigits(cmd[i + 1]) && stoi(cmd[i + 1]) > 0) {
				threads = stoi(cmd[++i]);
			}
			else {
				cout << "Incorrect thread value" << endl;
				return;
			}
		}
		else if (tok == cui::PERFT_H) {
			if (i + 1 < cmd.size() && utils::isPositiveDigits(cmd[i + 1]) && stoi(cmd[i + 1]) > 0) {
				ttSize = stoi(cmd[++i]);
			}
			else {
				cout << "Incorrect TT size value" << endl;
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

	if (ttEnabled) tt::init(ttSize);

	if (divideMode) {
		perftDivide(depth, threads);
	}
	else {
		perftFast(depth);
	}

	if (ttEnabled) tt::free();
}

void Cui::perftFast(int depth) {
	string fen = game::getFen();

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
	string fen = game::getFen();

	high_resolution_clock::time_point start, end;

	start = high_resolution_clock::now();
	U64 nodes = ttEnabled ? divide<true>(depth, threads) : divide<false>(depth, threads);
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

template <bool useTT>
__forceinline U64 Cui::divide(int depth, int threads) {
	U64 totalNodes = 0;
	U64 moveNodes;

	int size = movesArray.size();
	if (depth == 1)
		return size;

	if (depth < 4) {
		MoveInfo* m = movesArray.moves();

		for (int i = 0; i < size; i++) {
			moveNodes = generateMoves<useTT>(depth - 1, m[i].board);

			printf("%s %llu\n", utils::getMoveSimple(m[i]).c_str(), moveNodes);
			totalNodes += moveNodes;
		}

		return totalNodes;
	}

	struct Task { int root; BoardState board; };

	MoveArray roots = movesArray;
	MoveInfo* rm = roots.moves();
	const int rootCount = roots.size();

	std::vector<Task> tasks;
	tasks.reserve(8192);

	std::vector<std::atomic<int>> remaining(rootCount);
	std::vector<std::atomic<U64>> rootNodes(rootCount);

	const int plies = 2;

	auto expand = [&](auto&& self, int root, const BoardState& b, int pliesLeft) -> void {
		if (pliesLeft == 0) {
			tasks.push_back({ root, b });
			remaining[root].fetch_add(1, std::memory_order_relaxed);
			return;
		}
		movesArray.reset();
		generateMoves<useTT>(0, b);
		const int c = movesArray.size();
		MoveInfo* m = movesArray.moves();

		BoardState kids[256];
		for (int j = 0; j < c; ++j) kids[j] = m[j].board;
		for (int j = 0; j < c; ++j) self(self, root, kids[j], pliesLeft - 1);
	};

	for (int i = 0; i < rootCount; i++)
		expand(expand, i, rm[i].board, plies);

	for (int i = 0; i < rootCount; i++)
		if (remaining[i].load(std::memory_order_relaxed) == 0)
			printf("%s 0\n", utils::getMoveSimple(rm[i]).c_str());

	std::atomic<size_t> next{ 0 };

	auto worker = [&] {
		size_t t;
		while ((t = next.fetch_add(1, std::memory_order_relaxed)) < tasks.size()) {
			const Task& task = tasks[t];

			U64 nodes = generateMoves<useTT>(depth - (plies + 1), task.board);

			rootNodes[task.root].fetch_add(nodes, std::memory_order_relaxed);

			if (remaining[task.root].fetch_sub(1, std::memory_order_acq_rel) == 1) {
				U64 mv = rootNodes[task.root].load(std::memory_order_relaxed);
				printf("%s %llu\n", utils::getMoveSimple(rm[task.root]).c_str(), mv);
			}
		}
	};

	int maxThreads = std::thread::hardware_concurrency();
	int threadCount = (threads > 0 && threads < maxThreads) ? threads : maxThreads;

	if (tasks.size() > 0 && threadCount > tasks.size())
		threadCount = tasks.size();

	std::vector<std::thread> pool;
	pool.reserve(threadCount);

	for (int t = 0; t < threadCount; ++t)
		pool.emplace_back(worker);

	for (auto& th : pool)
		th.join();

	for (int i = 0; i < rootCount; i++)
		totalNodes += rootNodes[i].load(std::memory_order_relaxed);

	return totalNodes;
}

void Cui::test() {
	bool tt = ttEnabled;
	ttEnabled = false;

	string fen = game::getFen();

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

	ttEnabled = tt;
}

void Cui::perftsuite() {
	bool tt = ttEnabled;
	ttEnabled = false;

	string fenS = game::getFen();

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

	ttEnabled = tt;
}

void Cui::benchmark(string& depth, string& amount) {
	bool tt = ttEnabled;
	ttEnabled = false;

	int d = 6;
	int a = 25;
	bool print = true;

	if (depth.length() > 0) {
		if (depth == "all") {
			a = 15;
			print = false;
			string fen = game::getFen();

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

	ttEnabled = tt;
}

void Cui::executeBenchmark(int depth, int amount, bool print) {
	string fen = game::getFen();

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
	bool tt = ttEnabled;
	ttEnabled = false;

	string fen = game::getFen();

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

	ttEnabled = tt;
}

U64 Cui::generateMoves(int depth) {
	if (ttEnabled)	return generateMoves<true>(depth, game::board);
	else			return generateMoves<false>(depth, game::board);
	
}

template <bool useTT>
U64 Cui::generateMoves(int depth, const BoardState& board) {
	const uint8_t kMoved = (!(board.casPerms & (wk | wq)) ? KING_MOVED[white] : 0)
		| (!(board.casPerms & (bk | bq)) ? KING_MOVED[black] : 0);

	if (board.side == white) {
		switch (kMoved) {
		case KING_MOVED[white]: return generateMoves<white, KING_MOVED[white], useTT>(depth, board);
		case KING_MOVED[black]: return generateMoves<white, KING_MOVED[black], useTT>(depth, board);
		case KING_MOVED[both]:  return generateMoves<white, KING_MOVED[both], useTT>(depth, board);
		default:                return generateMoves<white, 0, useTT>(depth, board);
		}
	}
	else {
		switch (kMoved) {
		case KING_MOVED[white]: return generateMoves<black, KING_MOVED[white], useTT>(depth, board);
		case KING_MOVED[black]: return generateMoves<black, KING_MOVED[black], useTT>(depth, board);
		case KING_MOVED[both]:  return generateMoves<black, KING_MOVED[both], useTT>(depth, board);
		default:                return generateMoves<black, 0, useTT>(depth, board);
		}
	}
}

template <bool side, uint8_t kMoved, bool useTT>
U64 Cui::generateMoves(int depth, const BoardState& board) {
	switch (depth) {
		/*case 18: return PerftGenerator<18, side, kMoved, useTT>::generateMoves(board);
		case 17: return PerftGenerator<17, side, kMoved, useTT>::generateMoves(board);
		case 16: return PerftGenerator<16, side, kMoved, useTT>::generateMoves(board);
		case 15: return PerftGenerator<15, side, kMoved, useTT>::generateMoves(board);
		case 14: return PerftGenerator<14, side, kMoved, useTT>::generateMoves(board);
		case 13: return PerftGenerator<13, side, kMoved, useTT>::generateMoves(board);
	case 12: return PerftGenerator<12, side, kMoved, useTT>::generateMoves(board);
	case 11: return PerftGenerator<11, side, kMoved, useTT>::generateMoves(board);*/
	case 10: return PerftGenerator<10, side, kMoved, useTT>::generateMoves(board);
	case 9: return PerftGenerator<9, side, kMoved, useTT>::generateMoves(board);
	case 8: return PerftGenerator<8, side, kMoved, useTT>::generateMoves(board);
	case 7: return PerftGenerator<7, side, kMoved, useTT>::generateMoves(board);
	case 6: return PerftGenerator<6, side, kMoved, useTT>::generateMoves(board);
	case 5: return PerftGenerator<5, side, kMoved, useTT>::generateMoves(board);
	case 4: return PerftGenerator<4, side, kMoved, useTT>::generateMoves(board);
	case 3: return PerftGenerator<3, side, kMoved, useTT>::generateMoves(board);
	case 2: return PerftGenerator<2, side, kMoved, useTT>::generateMoves(board);
	case 1: return PerftGenerator<1, side, kMoved, useTT>::generateMoves(board);
	default: return PerftGenerator<0, side, kMoved, useTT>::generateMoves(board);
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
	cout << "undo\t\tCancels the last move made" << endl;
	cout << "moves\t\tPrints the legal moves of the current position" << endl;
	cout << "reset\t\tResets the game to its initial position" << endl;
	cout << "print\t\tPrints the board representation of the current position" << endl;
	cout << "fen\t\tPrints the FEN record of the current position" << endl;
	cout << "setfen\t\tSets the position given by a FEN record" << endl;
	cout << "perft\t\tGenerates all moves down to a given depth" << endl;
	cout << "\t-d\tShows total of moves for each current legal move (multi-thread)" << endl;
	cout << "\t-t\tNumber of threads for -d (default: max available threads)" << endl;
	cout << "\t-h\tSize of the transposition table in MB (default: max available memory)" << endl;
	cout << "tt\t\tToggles the transposition table on/off (tt [on|off])" << endl;
	cout << "cmp\t\tIterates over popular positions and provides an average" << endl;
	cout << "test\t\tTests popular positions to validate perft results" << endl;
	cout << "perftsuite\tTests full list of positions to validate perft results" << endl;
	cout << "benchmark\tPerforms a series of perft and returns the best time" << endl;
	cout << "\t\tSpecify depth then amount. Default is 6 and 25" << endl;
	cout << "pieces\t\tPrints the pieces' count for each side" << endl;
	cout << "exit\t\tExit the application" << endl;
}