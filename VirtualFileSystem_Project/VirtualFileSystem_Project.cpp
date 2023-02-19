#include <iostream>
#include <string>
#include "IVFS.h"

#include <time.h>	// temp

using namespace std;

int main()
{
	string testname = "zeros.tfs";
	size_t size = 1024 * 1024;	// 1 Mb
	//cout << "Testing file " << testname << endl;
	
	//FillWithZeros(fs, size);


	// Testing VFS
	/**/
	VFS* vfs = new VFS();
	string str = "exisas";
	vfs->MountOrCreate(str);

	// Testing VDisk
	/*
	VDisk* wd = new VDisk(testname, size);
	cout << wd -> PrintSpaceLeft();
	delete wd;

	VDisk* rd = new VDisk(testname);
	cout << rd -> PrintSpaceLeft();
	delete rd;*/
	
	// Testing reading from files
	/*
	fstream fs;
	fs.open(testname, std::fstream::in | std::ios::out | std::fstream::binary);
	char* str = new char[ADDR];
	fs.seekg(8,ios_base::beg);
	fs.read(str, ADDR);
	cout << CharToInt32(str);*/

	// Testing data conversion
	/*
	uint32_t test = 1234567;
	char* ptr = DataToChar(test);
	test = CharToInt32(ptr);
	cout << test;*/

	// Testing VFS 
	/*
	VFS* testVFS = new VFS();
	testVFS->MountOrCreate(testname);
	delete testVFS;*/

	// Testing filestreams:
	/*
	fstream fs;
	fs.open(testname, ios_base::in | ios_base::out | ios_base::binary);
	fs.seekp(0, std::ios_base::beg);
	int s = 123;
	fs.write(to_string(s).c_str(), ADDR);
	fs.seekg(0, std::ios_base::beg);
	int str;
	fs.read((char*)&str, ADDR);
	cout << str << endl;
	fs.close();*/

	// Measuring time:
	//clock_t tStart = clock();
	//printf("Time taken: %.2fs\n", (double)(clock() - tStart) / CLOCKS_PER_SEC);

	cout << "\nDone\n";
}