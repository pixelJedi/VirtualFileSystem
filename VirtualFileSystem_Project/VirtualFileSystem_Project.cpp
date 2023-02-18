#include <iostream>
#include <string>
#include "IVFS.h"

#include <time.h>	// temp

using namespace std;

int main()
{
	string testname = "zeros.tfs";
	size_t size = 1024 * 1024 * 2;	// 1 Mb
	//cout << "Testing file " << testname << endl;
	
	//FillWithZeros(fs, size);

	// Testing VDisk
	/*VDisk* vd = new VDisk(testname, size);
	cout << vd -> PrintSpaceLeft();
	delete vd;

	vd = new VDisk(testname);
	cout << vd -> PrintSpaceLeft();
	delete vd;*/

	uint32_t test = 1234567;
	char* ptr = DataToChar(test);
	test = CharToInt32(ptr);
	cout << test;

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