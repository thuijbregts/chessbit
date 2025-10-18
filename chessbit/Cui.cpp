#include "Test.hpp"
#include "Cui.h"
#include "MoveGenerator.h"
#include "TranspositionTable.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <future>
#include <vector>
#include <utility>
#include <cstdio>
#include <filesystem>

using namespace chrono;
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
	printf("\n");
	help();
	do {
		getline(cin, input);
		cmd = utils::split(input, ' ');
		execute(cmd);

	} while (true);
}

void Cui::initMoves() {
	movesArray.reset();
	generateMoves(0, game::moves[game::count]->board, movesArray);
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
	if (game::count > 0) {
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
	game::printBoard(game::moves[game::count]->board);
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
			else if (option == "-w") {
				perftWrite(d);
			}
			else if (option == "-r") {
				perftRead(d);
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
	U64 nodes = generateMoves(depth, game::moves[game::count]->board, movesArray);
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

	int size = movesArray.size();
	if (depth == 1) {
		return size;
	}
	vector<future<U64>> futures;
	MoveInfo* m = movesArray.moves();
	MoveArray* arr = new MoveArray[size];
	for (int i = 0; i < size; i++) {
		generateMoves(0, m[i].board, arr[i]);
		MoveInfo* m1 = arr[i].moves();
		MoveArray* arr1 = new MoveArray[arr[i].size()];
		for (int j = 0; j < arr[i].size(); j++) {
			generateMoves(0, m1[j].board, arr1[j]);
			MoveInfo* m2 = arr1[j].moves();
			for (int k = 0; k < arr1[j].size(); k++) {
				futures.push_back(async(launch::async, [this, depth, move = m2[k]]() {
					MoveArray dummy;
					U64 current = generateMoves(depth - 3, move.board, dummy);
					//printf("%s %llu\n", utils::getMoveSimple(move).c_str(), current);
					return current;
					}));
			}
		}
	}

	for (auto& f : futures) {
		nodes += f.get();
	}

	return nodes;
}

namespace std {
	template<>
	struct hash<Zobrist> {
		size_t operator()(const Zobrist& z) const noexcept {
			return static_cast<size_t>(z.high ^ (z.low * 0x9E3779B97F4A7C15ULL));
		}
	};
}

void Cui::perftWrite(int depth) {
	string fen = game::getFen();

	auto start = high_resolution_clock::now();

	// Vérifie si store_temp.dat existe déjà
	bool tempExists = std::filesystem::exists("store_temp.dat");

	if (!tempExists) {
		cout << "[INFO] store_temp.dat non trouvé, génération brute..." << endl;

		ofstream raw("store_temp.dat", ios::binary | ios::trunc);
		if (!raw) {
			cerr << "Erreur: impossible de créer store_temp.dat" << endl;
			return;
		}

		std::function<void(int, MoveArray&)> dump;
		dump = [&](int d, MoveArray& movesArray) {
			int size = movesArray.size();
			MoveInfo* m = movesArray.moves();
			if (d > 1) {
				for (int i = 0; i < size; i++) {
					MoveArray arr;
					generateMoves(0, m[i].board, arr);
					dump(d - 1, arr);
				}
			}
			else {
				for (int i = 0; i < size; i++) {
					bstate::Entry e(m[i].board, 1);
					raw.write(reinterpret_cast<const char*>(&e), sizeof(bstate::Entry));
				}
			}
			};

		dump(depth, movesArray);
		raw.close();

		auto mid = high_resolution_clock::now();
		cout << "Brute dump terminé en "
			<< duration_cast<milliseconds>(mid - start).count() << " ms" << endl;
	}
	else {
		cout << "[INFO] store_temp.dat déjà présent, skip génération brute." << endl;
	}

	// Phase de tri/merge externe
	const size_t CHUNK = 500'000;
	vector<bstate::Entry> buffer(CHUNK);

	ifstream in("store_temp.dat", ios::binary);
	if (!in) {
		cerr << "Erreur: store_temp.dat introuvable" << endl;
		return;
	}

	vector<string> chunkFiles;
	size_t part = 0;

	while (in) {
		in.read(reinterpret_cast<char*>(buffer.data()), CHUNK * sizeof(bstate::Entry));
		size_t count = in.gcount() / sizeof(bstate::Entry);
		if (count == 0) break;

		sort(buffer.begin(), buffer.begin() + count, [](const auto& a, const auto& b) {
			if (a.board.zobrist.high != b.board.zobrist.high)
				return a.board.zobrist.high < b.board.zobrist.high;
			return a.board.zobrist.low < b.board.zobrist.low;
			});

		string chunkName = "chunk_" + to_string(part++) + ".dat";
		ofstream chunk(chunkName, ios::binary | ios::trunc);
		chunk.write(reinterpret_cast<const char*>(buffer.data()), count * sizeof(bstate::Entry));
		chunk.close();
		chunkFiles.push_back(chunkName);
	}
	in.close();

	ofstream out("store.dat", ios::binary | ios::trunc);
	vector<ifstream> parts;
	for (auto& f : chunkFiles) parts.emplace_back(f, ios::binary);

	vector<bstate::Entry> heads(parts.size());
	vector<bool> valid(parts.size(), false);

	auto refill = [&](size_t i) {
		if (parts[i].read(reinterpret_cast<char*>(&heads[i]), sizeof(bstate::Entry)))
			valid[i] = true;
		else
			valid[i] = false;
		};

	for (size_t i = 0; i < parts.size(); i++) refill(i);

	bstate::Entry current;
	bool hasCurrent = false;

	while (true) {
		int idx = -1;
		Zobrist minKey{};
		for (size_t i = 0; i < parts.size(); i++) {
			if (!valid[i]) continue;
			Zobrist k = heads[i].board.zobrist;
			if (idx == -1 || k.high < minKey.high || (k.high == minKey.high && k.low < minKey.low)) {
				idx = (int)i;
				minKey = k;
			}
		}
		if (idx == -1) break;

		bstate::Entry e = heads[idx];
		refill(idx);

		if (!hasCurrent) {
			current = e;
			hasCurrent = true;
		}
		else if (e.board.zobrist.high == current.board.zobrist.high &&
			e.board.zobrist.low == current.board.zobrist.low) {
			current.qty += e.qty;
		}
		else {
			out.write(reinterpret_cast<const char*>(&current), sizeof(bstate::Entry));
			current = e;
		}
	}
	if (hasCurrent)
		out.write(reinterpret_cast<const char*>(&current), sizeof(bstate::Entry));

	out.close();

	for (auto& f : chunkFiles)
		std::filesystem::remove(f);

	auto end = high_resolution_clock::now();
	cout << "Fusion externe terminée en "
		<< duration_cast<milliseconds>(end - start).count() << " ms" << endl;

	game::setFen(fen.c_str());
}


void Cui::perftRead(int depth) {
	ifstream in("store.dat", ios::binary);
	if (!in) {
		cerr << "File store.dat not found" << endl;
		return;
	}

	string fen = game::getFen();
	high_resolution_clock::time_point start, end;

	// lire nombre d'entrées
	uint64_t size = 0;
	in.read(reinterpret_cast<char*>(&size), sizeof(size));

	const size_t CHUNK_SIZE = 1'000'000;
	vector<bstate::Entry> buffer(CHUNK_SIZE);

	U64 nodes = 0;
	start = high_resolution_clock::now();

	while (size > 0 && in) {
		size_t readCount = min(size, (uint64_t)CHUNK_SIZE);
		in.read(reinterpret_cast<char*>(buffer.data()), readCount * sizeof(bstate::Entry));

		vector<future<U64>> futures;
		for (size_t i = 0; i < readCount; i++) {
			bstate::Entry& entry = buffer[i];
			futures.push_back(async(launch::async, [this, depth, &entry]() {
				return generateMoves(depth, entry.board, movesArray) * entry.qty;
				}));
		}

		for (auto& f : futures) nodes += f.get();
		size -= readCount;
	}
	in.close();

	end = high_resolution_clock::now();

	cout << "Depth:\t\t" << depth << endl;
	cout << "Nodes:\t\t" << nodes << endl;
	cout << "Time:\t\t"
		<< duration_cast<milliseconds>(end - start).count() << " ms" << endl;
	game::setFen(fen.c_str());
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
		nodes = generateMoves(test.depth, game::moves[game::count]->board, movesArray);
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
		string fen = v[0];
		cout << fen << endl;
		int to = v.size();
		for (int i = 1; i < to; i++) {
			game::setFen(fen.c_str());

			auto perftvals = test::GetElements(v[i], ' ');
			U64 expected = static_cast<U64>(strtol(perftvals[1].c_str(), NULL, 10));
			U64 result = generateMoves(i, game::moves[game::count]->board, movesArray);
			string status = expected == result ? "OK" : "ERROR";
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
		auto volatile result = generateMoves(depth, game::moves[game::count]->board, movesArray);
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

	auto ts = chrono::steady_clock::now();
	setFen(StartPosition);
	for (int i = 1; i <= 7; i++)
	{
		start = high_resolution_clock::now();
		result = generateMoves(i, game::moves[game::count]->board, movesArray);
		end = high_resolution_clock::now();
		total = duration_cast<microseconds>(end - start).count();
		cout << "Perft Start " << i << ": " << result << " " << total / 1000 << "ms " << result * 1.0 / total << " MNodes/s\n";
	}
	if (result == 3195901860ull) cout << "OK\n\n";
	else cout << "ERROR!\n\n";

	setFen(KiwiPete);
	for (int i = 1; i <= 6; i++)
	{
		start = high_resolution_clock::now();
		result = generateMoves(i, game::moves[game::count]->board, movesArray);
		end = high_resolution_clock::now();
		total = duration_cast<microseconds>(end - start).count();
		cout << "Perft Kiwi " << i << ": " << result << " " << total / 1000 << "ms " << result * 1.0 / total << " MNodes/s\n";
	}
	if (result == 8031647685ull) cout << "OK\n\n";
	else cout << "ERROR!\n\n";

	setFen(Pos6);
	for (int i = 1; i <= 6; i++)
	{
		start = high_resolution_clock::now();
		result = generateMoves(i, game::moves[game::count]->board, movesArray);
		end = high_resolution_clock::now();
		total = duration_cast<microseconds>(end - start).count();
		cout << "Perft Midgame " << i << ": " << result << " " << total / 1000 << "ms " << result * 1.0 / total << " MNodes/s\n";
	}
	if (result == 6923051137ull) cout << "OK\n\n";
	else cout << "ERROR!\n\n";

	setFen(EndGame);
	for (int i = 1; i <= 7; i++)
	{
		start = high_resolution_clock::now();
		result = generateMoves(i, game::moves[game::count]->board, movesArray);
		end = high_resolution_clock::now();
		total = duration_cast<microseconds>(end - start).count();
		cout << "Perft Endgame " << i << ": " << result << " " << total / 1000 << "ms " << result * 1.0 / total << " MNodes/s\n";
	}
	if (result == 24958831314ull) cout << "OK\n\n";
	else cout << "ERROR!\n\n";

	U64 nodes = (3195901860ull + 8031647685ull + 6923051137ull + 24958831314ull);
	auto te = chrono::steady_clock::now();
	total = duration_cast<microseconds>(te - ts).count();
	cout << "Perft aggregate: " << nodes
		<< " " << total / 1000 << "ms " << nodes * 1.0 / total << " MNodes/s\n";

	game::setFen(fen.c_str());
}

U64 Cui::generateMoves(int depth, const BoardState& board, MoveArray& movesArray) {
	if (board.side == white) {
		switch (board.casPerms) {
		case 0b0000: return generateMoves<white, true, true>(depth, board, movesArray);
		case 0b0001: return generateMoves<white, false, true>(depth, board, movesArray);
		case 0b0010: return generateMoves<white, false, true>(depth, board, movesArray);
		case 0b0011: return generateMoves<white, false, true>(depth, board, movesArray);
		case 0b0100: return generateMoves<white, true, false>(depth, board, movesArray);
		case 0b0101: return generateMoves<white, false, false>(depth, board, movesArray);
		case 0b0110: return generateMoves<white, false, false>(depth, board, movesArray);
		case 0b0111: return generateMoves<white, false, false>(depth, board, movesArray);
		case 0b1000: return generateMoves<white, true, false>(depth, board, movesArray);
		case 0b1001: return generateMoves<white, false, false>(depth, board, movesArray);
		case 0b1010: return generateMoves<white, false, false>(depth, board, movesArray);
		case 0b1011: return generateMoves<white, false, false>(depth, board, movesArray);
		case 0b1100: return generateMoves<white, true, false>(depth, board, movesArray);
		case 0b1101: return generateMoves<white, false, false>(depth, board, movesArray);
		case 0b1110: return generateMoves<white, false, false>(depth, board, movesArray);
		default: return generateMoves<white, false, false>(depth, board, movesArray);
		}

	}
	else {
		switch (board.casPerms) {
		case 0b0000: return generateMoves<black, true, true>(depth, board, movesArray);
		case 0b0001: return generateMoves<black, true, false>(depth, board, movesArray);
		case 0b0010: return generateMoves<black, true, false>(depth, board, movesArray);
		case 0b0011: return generateMoves<black, true, false>(depth, board, movesArray);
		case 0b0100: return generateMoves<black, false, true>(depth, board, movesArray);
		case 0b0101: return generateMoves<black, false, false>(depth, board, movesArray);
		case 0b0110: return generateMoves<black, false, false>(depth, board, movesArray);
		case 0b0111: return generateMoves<black, false, false>(depth, board, movesArray);
		case 0b1000: return generateMoves<black, false, true>(depth, board, movesArray);
		case 0b1001: return generateMoves<black, false, false>(depth, board, movesArray);
		case 0b1010: return generateMoves<black, false, false>(depth, board, movesArray);
		case 0b1011: return generateMoves<black, false, false>(depth, board, movesArray);
		case 0b1100: return generateMoves<black, false, true>(depth, board, movesArray);
		case 0b1101: return generateMoves<black, false, false>(depth, board, movesArray);
		case 0b1110: return generateMoves<black, false, false>(depth, board, movesArray);
		default: return generateMoves<black, false, false>(depth, board, movesArray);
		}
	}
}

template <bool side, bool kMMoved, bool kEMoved>
U64 Cui::generateMoves(int depth, const BoardState& board, MoveArray& movesArray) {
	switch (depth) {
	case 18: return PerftGenerator<18, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 17: return PerftGenerator<17, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 16: return PerftGenerator<16, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 15: return PerftGenerator<15, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 14: return PerftGenerator<14, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 13: return PerftGenerator<13, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 12: return PerftGenerator<12, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 11: return PerftGenerator<11, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 10: return PerftGenerator<10, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 9: return PerftGenerator<9, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 8: return PerftGenerator<8, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 7: return PerftGenerator<7, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 6: return PerftGenerator<6, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 5: return PerftGenerator<5, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 4: return PerftGenerator<4, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 3: return PerftGenerator<3, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 2: return PerftGenerator<2, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	case 1: return PerftGenerator<1, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
	default: return PerftGenerator<0, side, kMMoved, kEMoved>::generateMoves(board, movesArray);
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
	if (game::moves[game::count]->board.side == white) iteratePieces(game::moves[game::count]->board.pM, game::moves[game::count]->board.nM, game::moves[game::count]->board.bM, game::moves[game::count]->board.rM, game::moves[game::count]->board.qM);
	else						iteratePieces(game::moves[game::count]->board.pE, game::moves[game::count]->board.nE, game::moves[game::count]->board.bE, game::moves[game::count]->board.rE, game::moves[game::count]->board.qE);

	cout << "BLACK:" << endl;
	if (game::moves[game::count]->board.side == black) iteratePieces(game::moves[game::count]->board.pM, game::moves[game::count]->board.nM, game::moves[game::count]->board.bM, game::moves[game::count]->board.rM, game::moves[game::count]->board.qM);
	else						iteratePieces(game::moves[game::count]->board.pE, game::moves[game::count]->board.nE, game::moves[game::count]->board.bE, game::moves[game::count]->board.rE, game::moves[game::count]->board.qE);
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
	cout << "\t-f\tGives statistics about the position" << endl;
	cout << "cmp\t\tIterates over popular positions and provides an average" << endl;
	cout << "test\t\tTests popular positions to validate perft results" << endl;
	cout << "perftsuite\tTests full list of positions to validate perft results" << endl;
	cout << "benchmark\tPerforms a series of perft and returns the best time" << endl;
	cout << "\t\tSpecify depth then amount. Default is 6 and 25" << endl;
	cout << "pieces\t\tPrints the pieces' count for each side" << endl;
	cout << "exit\t\tExit the application" << endl;
}