#include "Test.hpp"
#include "Cui.h"
#include "MoveGenerator.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>

using namespace std::chrono;
using namespace movegen;
using namespace game;
using namespace movarray;

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
	movarray::movesArray.reset();
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
	int size = cmd.size();
	string param1, param2;

	if (size > 1) {
		if (size == 2) {
			param1 = cmd[1];
		}
		else {
			param1 = cmd[1];
			param2 = cmd[2];
		}
	}

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
		setBoard(cmd, size);
		return;
	}

	if (first == cui::PERFT) {
		if (size == 2) {
			string empty;
			perft(empty, param1);
		}
		else {
			perft(param1, param2);
		}

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
	generateMoves();*/
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
	int size = movarray::movesArray.size();
	MoveInfo* mv = movarray::movesArray.moves();
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

void Cui::perft(string& option, string& depth) {
	if (depth.length() > 0 && utils::isPositiveDigits(depth) && stoi(depth) > 0) {
		int d = stoi(depth);
		if (option.length() > 0) {
			if (option == cui::PERFT_D) {
				perftDivide(d);
			}
			else if (option == cui::PERFT_F) {
				perftFull(d);
			}
			else {
				cout << "incorrect perft option" << endl;
			}
		}
		else {
			perftFast(d);
		}
	}
	else {
		cout << "incorrect depth value" << endl;
	}
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

void Cui::perftDivide(int depth) {
	string fen = game::getFen();

	high_resolution_clock::time_point start, end;

	start = high_resolution_clock::now();
	U64 nodes = divide(depth);
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

__forceinline U64 Cui::divide(int depth) {
	U64 nodes = 0;
	U64 current = 0;

	initMoves();
	int size = movarray::movesArray.size();
	if (depth == 1) {
		return size;
	}
	MoveInfo* m = movarray::movesArray.moves();
	for (int i = 0; i < size; i++) {
		game::makeMove(m[i]);
		current = generateMoves(depth - 1);
		nodes += current;
		printf("%s %llu\n", utils::getMoveSimple(m[i]).c_str(), current);
		game::unmakeMove();
	}

	return nodes;
}

void Cui::perftFull(int depth) {
	string fen = game::getFen();

	int caps = 0;
	int eP = 0;
	int cstl = 0;
	int prom = 0;
	int chk = 0;
	int dischck = 0;
	int dblchk = 0;
	int chkm = 0;

	high_resolution_clock::time_point start, end;

	start = high_resolution_clock::now();
	U64 nodes = full(depth, caps, eP, cstl, prom, chk, dischck, dblchk, chkm);
	end = high_resolution_clock::now();

	long long total = duration_cast<microseconds>(end - start).count();

	printf("Captures:\t%d\n", caps);
	printf("En passant:\t%d\n", eP);
	printf("Castles:\t%d\n", cstl); 
	printf("Promotions:\t%d\n", prom);
	printf("Checks:\t\t%d\n", chk); 
	printf("Disc. checks:\t%d\n", dischck);
	printf("Double checks:\t%d\n", dblchk);
	printf("Checkmates:\t%d\n", chkm);
	printf("\n");
	printf("Depth:\t\t%d\n", depth);
	printf("Nodes:\t\t%llu\n", nodes);
	printf("Time:\t\t%llu ms\n", (total / 1000));
	if (total > 0) {
		cout << "Average:\t" << (nodes * 1.0 / total) << " Mn/s" << endl;
	}

	game::setFen(fen.c_str());

	movarray::movesArray = movarray::movesArrayPool[0];
	initMoves();
}

__forceinline U64 Cui::full(int depth, int& caps, int& eP, int& cstl, int& prom, int& chk, int& dischck, int& dblchk, int& chkm) {
	if (depth == 0) {
		return 1;
	}
	U64 nodes = 0;

	movarray::movesArray = movarray::movesArrayPool[depth];
	initMoves();
	int size = movarray::movesArray.size();
	MoveInfo* m = movarray::movesArray.moves();
	for (int i = 0; i < size; i++) {
		game::makeMove(m[i]);

		if (depth == 1) {
			if (m[i].capture) caps++;
			if (m[i].type == Promotion) prom++;
			else if (m[i].type == EnPassant) eP++;
			else if (m[i].type == Castle) cstl++;
			U64 checks = m[i].board.checks;
			if (checks) {
				chk++;
				if (!generateMoves(1)) chkm++;
				else {
					U64 to = (1ULL << m[i].to);
					if (checks & to) {
						checks ^= to;
						if (checks) dblchk++;
					}
					else dischck++;
				}
			}
		}

		nodes += full(depth - 1, caps, eP, cstl, prom, chk, dischck, dblchk, chkm);
		game::unmakeMove();
	}

	return nodes;
}

void Cui::test() {
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
}

void Cui::perftsuite() {
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
}

void Cui::benchmark(string& depth, string& amount) {
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
}

U64 Cui::generateMoves(int depth) {
	if (game::board.side == white) {
		switch (game::board.casPerms) {
		case 0b0000: return generateMoves<white, KING_MOVED[both]>(depth);
		case 0b0001: return generateMoves<white, KING_MOVED[black]>(depth);
		case 0b0010: return generateMoves<white, KING_MOVED[black]>(depth);
		case 0b0011: return generateMoves<white, KING_MOVED[black]>(depth);
		case 0b0100: return generateMoves<white, KING_MOVED[white]>(depth);
		case 0b0101: return generateMoves<white, 0>(depth);
		case 0b0110: return generateMoves<white, 0>(depth);
		case 0b0111: return generateMoves<white, 0>(depth);
		case 0b1000: return generateMoves<white, KING_MOVED[white]>(depth);
		case 0b1001: return generateMoves<white, 0>(depth);
		case 0b1010: return generateMoves<white, 0>(depth);
		case 0b1011: return generateMoves<white, 0>(depth);
		case 0b1100: return generateMoves<white, KING_MOVED[white]>(depth);
		case 0b1101: return generateMoves<white, 0>(depth);
		case 0b1110: return generateMoves<white, 0>(depth);
		default: return generateMoves<white, 0>(depth);
		}

	}
	else {
		switch (game::board.casPerms) {
		case 0b0000: return generateMoves<black, KING_MOVED[both]>(depth);
		case 0b0001: return generateMoves<black, KING_MOVED[black]>(depth);
		case 0b0010: return generateMoves<black, KING_MOVED[black]>(depth);
		case 0b0011: return generateMoves<black, KING_MOVED[black]>(depth);
		case 0b0100: return generateMoves<black, KING_MOVED[white]>(depth);
		case 0b0101: return generateMoves<black, 0>(depth);
		case 0b0110: return generateMoves<black, 0>(depth);
		case 0b0111: return generateMoves<black, 0>(depth);
		case 0b1000: return generateMoves<black, KING_MOVED[white]>(depth);
		case 0b1001: return generateMoves<black, 0>(depth);
		case 0b1010: return generateMoves<black, 0>(depth);
		case 0b1011: return generateMoves<black, 0>(depth);
		case 0b1100: return generateMoves<black, KING_MOVED[white]>(depth);
		case 0b1101: return generateMoves<black, 0>(depth);
		case 0b1110: return generateMoves<black, 0>(depth);
		default: return generateMoves<black, 0>(depth);
		}
	}
}

template <bool side, uint8_t kMoved>
U64 Cui::generateMoves(int depth) {
	switch (depth) {
	/*case 18: return PerftGenerator<18, side, kMoved>::generateMoves(game::board);
	case 17: return PerftGenerator<17, side, kMoved>::generateMoves(game::board);
	case 16: return PerftGenerator<16, side, kMoved>::generateMoves(game::board);
	case 15: return PerftGenerator<15, side, kMoved>::generateMoves(game::board);
	case 14: return PerftGenerator<14, side, kMoved>::generateMoves(game::board);
	case 13: return PerftGenerator<13, side, kMoved>::generateMoves(game::board);
	case 12: return PerftGenerator<12, side, kMoved>::generateMoves(game::board);
	case 11: return PerftGenerator<11, side, kMoved>::generateMoves(game::board);
	case 10: return PerftGenerator<10, side, kMoved>::generateMoves(game::board);
	case 9: return PerftGenerator<9, side, kMoved>::generateMoves(game::board);
	case 8: return PerftGenerator<8, side, kMoved>::generateMoves(game::board);*/
	case 7: return PerftGenerator<7, side, kMoved>::generateMoves(game::board);
	case 6: return PerftGenerator<6, side, kMoved>::generateMoves(game::board);
	case 5: return PerftGenerator<5, side, kMoved>::generateMoves(game::board);
	case 4: return PerftGenerator<4, side, kMoved>::generateMoves(game::board);
	case 3: return PerftGenerator<3, side, kMoved>::generateMoves(game::board);
	case 2: return PerftGenerator<2, side, kMoved>::generateMoves(game::board);
	case 1: return PerftGenerator<1, side, kMoved>::generateMoves(game::board);
	default: return PerftGenerator<0, side, kMoved>::generateMoves(game::board);
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
	cout << "\n******Commands******\n" << endl;
	cout << "undo\t\tCancels the last move made" << endl;
	cout << "moves\t\tPrints the legal moves of the current position" << endl;
	cout << "reset\t\tResets the game to its initial position" << endl;
	cout << "print\t\tPrints the board representation of the current position" << endl;
	cout << "fen\t\tPrints the FEN record of the current position" << endl;
	cout << "setfen\t\tSets the position given by a FEN record" << endl;
	cout << "perft\t\tGenerates all moves down to a given depth" << endl;
	cout << "\t-d\tShows total of moves for each current legal move" << endl;
	cout << "\t-f\tGives statistics about the position (not optimized)" << endl;
	cout << "cmp\t\tIterates over popular positions and provides an average" << endl;
	cout << "test\t\tTests popular positions to validate perft results" << endl;
	cout << "perftsuite\tTests full list of positions to validate perft results" << endl;
	cout << "benchmark\tPerforms a series of perft and returns the best time" << endl;
	cout << "\t\tSpecify depth then amount. Default is 6 and 25" << endl;
	cout << "pieces\t\tPrints the pieces' count for each side" << endl;
	cout << "exit\t\tExit the application" << endl;
}