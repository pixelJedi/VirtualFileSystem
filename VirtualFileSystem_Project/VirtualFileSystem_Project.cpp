#include <iostream>
#include <string>
#include "IVFS.h"

#include <time.h>	// temp

using namespace std;

int main()
{
	string testname = "zeros.tfs";
	size_t size = 1024 * 1024 * 1;	// 1 Mb
	cout << "Testing file " << testname << endl;
	
	//FillWithZeros(fs, size);

	// Testing VDisk
	VDisk* vd = new VDisk(testname, size);
	cout << vd -> PrintSpaceLeft();
	delete vd;

	// Testing VFS
	//VFS* testVFS = new VFS();
	//testVFS->MountOrCreate(testname);
	//testVFS->Open(filename.c_str());
	//delete testVFS;

	// Testing filestreams:
	//fstream fs;
	//fs.open(testname, ios_base::in | ios_base::out | ios_base::binary);
	//fs.close();

	// Measuring time:
	//clock_t tStart = clock();
	//printf("Time taken: %.2fs\n", (double)(clock() - tStart) / CLOCKS_PER_SEC);

	cout << "Done\n";
}