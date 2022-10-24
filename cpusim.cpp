#include "CPU.h"

#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
#include<fstream>
#include <sstream>
using namespace std;

/* 
Add all the required standard and developed libraries here. 
Remember to include all those files when you are submitting the project. 
*/
#include <cstdint>
#include <vector>
#include <algorithm>
#include <iterator>

/*
Put/Define any helper function/definitions you need here
*/


int main (int argc, char* argv[]) // your project should be executed like this: ./cpusim filename.txt and should print (a0,a1) 
{
	// Checks for correct number of arguments
	if (argc < 2) {
		//cout << "No file name entered. Exiting...";
		return -1;
	}

	ifstream ifs(argv[1]); //open the file
	if (!(ifs.is_open() && ifs.good())) {
		cout << "error opening file\n";
		return 0;
	}

	// This creates a vector for the instruction memory bytes and then uses std iterators to copy each line of the file.
	// I read them in as uint16_t and then copy them as uint8_t. The reason this is necessary is because uint8_t would
	// be read as a char and only grab one digit of each numbers at a time.
	vector<uint8_t> instMem;
    istream_iterator<uint16_t> input(ifs);
    copy(input, istream_iterator<uint16_t>(), back_inserter(instMem));

    // This vector is for the data memory. Size is set for 4096 bytes.
    vector<uint8_t> dataMem (4096, 0x0);

	// Instantiate CPU object
    auto myCPU = CPU(move(instMem), move(dataMem));

	while (true) // processor's main loop. Each iteration is equal to one clock cycle.
	{
		//fetch
		myCPU.fetch();

		// decode
		myCPU.decode();

		// execute
        myCPU.execute();

		// memory
        myCPU.memory();

		// writeback
        myCPU.writeback();

		// _next values should be written to _current values here:
        myCPU.clockTick();

		// Break the loop if ALL instructions in the pipeline has opcode==0 instruction
		if (myCPU.isFinished()) { break; }
	}

	// print the stats
    myCPU.printStats();

	exit(0);
}



