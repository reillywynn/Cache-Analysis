#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <tuple>
#include <ranges>
#include <bit>
#include <sstream>
#include <bitset>
#include <span>
#include <stdlib.h>

using namespace std;

class Cache {
private:
	unsigned int blockSize;
	unsigned int associativity;
	unsigned long long int capacity;
	bool isLRU;


	unsigned int set;
	unsigned int tag;
	unsigned int findSet;

	vector<unsigned long long int> tags;
	vector<char> noGo;
	vector<char> Go;
	vector<int> highPriority;


	int dataRequests = 0;
	int numberOfMisses = 0;
	unsigned long long int noGoWB = 0;

	ifstream inFile;

public:
	Cache(bool isLRU, string file, unsigned int blockSize, unsigned int associativity, unsigned long long int capacity);
	~Cache();
	
	void runCache();
	tuple<bool, unsigned long long int> readLine(string currLine);
		
	int GetSet(unsigned long long int address);
	unsigned long long int GetTag(unsigned long long int address);
	bool hitCheck(bool type, unsigned long long int address);
		
	void updateStats(bool hit);
	void printStats();
		
};

Cache::Cache(bool isLRU, string file, unsigned int blockSize, unsigned int associativity, unsigned long long int capacity) {
	inFile.open(file);

	this->blockSize = blockSize;
	this->associativity = associativity;
	this->capacity = capacity;
	this->isLRU = isLRU;

	unsigned int blocks = capacity / blockSize;

	tags.resize(blocks);
	noGo.resize(blocks);
	Go.resize(blocks);
	highPriority.resize(blocks);

	//
	int numBitsInBlock = popcount(blockSize - 1);

	this->set = numBitsInBlock;
	auto sets = capacity / (blockSize * associativity);
	findSet = sets - 1;
	int numBitsInSet = popcount(findSet);

	tag = numBitsInBlock + numBitsInSet;
}

Cache::~Cache() {
	inFile.close();
	printStats();
}

void Cache::runCache() {
	string line;

	while (getline(inFile, line)) {
		auto [tp, address] = readLine(line);
		bool hit = hitCheck(tp, address);
		updateStats(hit);
	}
}

tuple<bool, unsigned long long int> Cache::readLine(string curr) {
	string in;
	unsigned long long int address;
	bool tp;

	string temp;
	string other;

	stringstream trace(curr);
	getline(trace, in, ' ');
	trace << hex << curr;
	trace >> address;
	getline(trace, temp, ' ');
	getline(trace, other, ' ');

	if (in == "l") {
		tp = 0;
	}

	else {
		tp = 1;
	}

	return {tp, address};
}

int Cache::GetSet(unsigned long long int address) {
	auto shift = address >> set;
	return shift & findSet;
}

unsigned long long int Cache::GetTag(unsigned long long int address) {
	return address >> tag;
}

bool Cache::hitCheck(bool type, unsigned long long int address) {
	auto set = GetSet(address);
	auto tag = GetTag(address);
	auto base = set * associativity;

	if (noGo.size() > 0) {
		span localityTags{ tags.data() + base, associativity };
		span localitynoGo{ noGo.data() + base, associativity };
		span localityGo{ Go.data() + base, associativity };
		span localityHighPriority{ highPriority.data() + base, associativity };


		bool hit = false;
		int noGoLine = -1;
		int index;
		for (int i = 0u; i < localityGo.size(); i++) { 
			if (!localityGo[i]) {
				noGoLine = i;
				continue;
			}

			if (tag != localityTags[i])
				continue;

			hit = true;
			index = i;

			localitynoGo[index] |= type;

			break;
		}

		auto noGoWb = false;
		if (!hit) {
			if (noGoLine >= 0) {
				index = noGoLine;
				localityGo[index] = 1;
			}

			else if (isLRU == true) {
				auto maxElement = ranges::max_element(localityHighPriority);
				index = distance(begin(localityHighPriority), maxElement);
				noGoWb = localitynoGo[index];
			}

			else {
				//FIFO Replacement
			}

			localityTags[index] = tag;
			localitynoGo[index] = type;
		}

		transform(begin(localityHighPriority), end(localityHighPriority), begin(localityHighPriority), [&](int x) {
			if (x <= localityHighPriority[index] && x < associativity)
				return x + 1;
			else
				return x;
			});

		localityHighPriority[index] = 0;

		return { hit };
	}
}

void Cache::updateStats(bool hit) {
	dataRequests++;
	numberOfMisses += !hit;
}

void Cache::printStats() {
	cout << "Cache Properties:" << endl;
	double numBlocks = capacity / blockSize;
	double numSets = numBlocks / (blockSize * associativity);
	cout << "Number of sets: " << numSets << endl;

	cout << "Number of blocks in each set: " << associativity << endl;
	cout << "Block size (Bytes): " << blockSize << endl;

	if (associativity == 8) {
		cout << "Associativity: Fully Associative" << endl;
	}

	else if (associativity == 1) {
		cout << "Associativity: Direct-Mapped " << endl;
	}

	else {
		cout << "Associativiy: " << associativity << "-Way Set Associative" << endl;
	}

	cout << "Cache Size (Bytes): " << capacity << endl;


	cout << "\nCache Hit Rate Data: " << endl;
	unsigned int hits = dataRequests - numberOfMisses;
	double hitRate = (double)hits / (double)dataRequests * 100.0;
	cout << "Hit Rate: " << hitRate << "%" << endl;
	cout << "Miss Rate: " << 100 - hitRate << "%" << endl;
	cout << "Hits: " << hits << endl;
	cout << "Misses: " << numberOfMisses << endl;
}

int main() {
	int userInput;
	
	cout << "Enter the block size in powers of 2 (2^x, for 64 input 6): " << endl;
	cin >> userInput;
	unsigned int blockSize = pow(2, userInput);

	cout << "Enter the power of the associativity (0, 1, 2, 3): " << endl;
	cin >> userInput;
	unsigned int associativity = pow(2, userInput);

	cout << "Enter the capacity of the cache (2^x, for 512 enter 9): " << endl;
	cin >> userInput;
	unsigned long long int capacity = pow(2, userInput);

	cout << "Choose replacement policy: 1 for LRU or 0 for FIFO:" << endl;
	cin >> userInput;
	bool LRU = userInput;

	for (int i = 0; i < 2; i++) {
		string traceFile;
		cout << "\nInput the trace file to run (enter the whole name with extension):" << endl;
		cin >> traceFile;

		Cache simulation(LRU, traceFile, blockSize, associativity, capacity);

		simulation.runCache();

	}

	return 0;
}


