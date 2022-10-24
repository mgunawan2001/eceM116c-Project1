#include "CPU.h"

#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
#include<fstream>
#include <sstream>
#include <vector>
using namespace std;



int main (int argc, char* argv[]) 
{
	// Checks for correct number of arguments
	if (argc < 2) {
		cout << "No file name entered. Exiting...";
		return -1;
	}

	// Open file
	ifstream infile(argv[1]); 
	
	// Check file can be opened
	if (!(infile.is_open() && infile.good())) {
		cout << "error opening file\n";
		return 0;
	}

	// Initialize Instruction Memory
	vector<uint8_t> instrMem;

	// Add instructions to Instruction Memory vector
	string line;
	while (infile) {
		infile >> line;
		stringstream line2(line);
		int x;
		line2 >> x;
		instrMem.push_back(bitset<8>(x).to_ulong());
	}

	//Initialize CPU
	auto myCPU = CPU(move(instrMem));

	// Run 'Processor'. 1 iteration = 1 clock cycle
	while (true) 
	{
		// 5 stages
		myCPU.Fetch();
		myCPU.Decode();
        myCPU.Execute();
        myCPU.Memory();
        myCPU.Writeback();

		// End one clock cycle
        myCPU.clockTick();

		// Break the loop if ALL instructions have been executed (all instruction have ZERO opcode)
		if (myCPU.isFinished()) { break; }
	}

	// Print info
    //myCPU.printInfo();

	// Prints (a0,a1)
	myCPU.printResult();

	exit(0);
}



