#include <iostream>
#include <string>
#include <bitset>
#include "IVFS.h"
#include "Node.h"

// #include <time.h>	

using namespace std;

int main()
{
	Node* root = new Node();
	root->Add("mike\\jake\\cook\\lochness", 123);
	root->Add("mike\\jake\\cook\\loch", 123);
	root->Add("mike\\boob\\cook\\loch", 123);
	root->Print();

	/*
	// Testing VFS
	string disk = "test.tfs";
	size_t size = 1024 * 1024;	// 1 Mb
	cout << "Testing file " << disk << ": size " << size << endl;

	VFS* vfs = new VFS();
	char* file = new char[5] { 'f', 'i', 'l', 'e', '\0'};

	vfs->MountOrCreate(disk);
	vfs->Create(file);		// <-- currently working on
	vfs->Unmount(disk);

	delete[] file;
	delete vfs;*/

	// Measuring time:
	//clock_t tStart = clock();
	//printf("Time taken: %.2fs\n", (double)(clock() - tStart) / CLOCKS_PER_SEC);
}