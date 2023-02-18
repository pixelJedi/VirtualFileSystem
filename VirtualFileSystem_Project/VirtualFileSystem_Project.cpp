#include <iostream>
#include <string>
#include "IVFS.h"

#include <time.h>	// temp

using namespace std;

int main()
{
	string testname = "zeros.tfs";
	cout << "Filling file " << testname << endl;

	//string filename = "booboo";
	//VFS* testVFS = new VFS();

	//testVFS->MountOrCreate(testname);
	//testVFS->Open(filename.c_str());

	fstream fs;
	fs.open(testname, ios_base::in | ios_base::out | ios_base::binary);

	clock_t tStart = clock();
	FillWithZeros(fs, 1024 * 1024 * 10);
	printf("Time taken: %.2fs\n", (double)(clock() - tStart) / CLOCKS_PER_SEC);

	fs.close();
	cout << "Done\n";

	//delete testVFS;
}