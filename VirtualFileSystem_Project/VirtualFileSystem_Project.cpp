﻿#include <iostream>
#include <string>
#include <bitset>
#include "IVFS.h"
#include "Vertice.h"

// #include <time.h>	

using namespace std;

int main()
{	
	// Testing VFS

	Vertice<int>* v = new Vertice<int>();
	v->Add("0alla\\1gppp\\2mayflower\\3kerning", 4);
	v->Add("0alla\\1kanister", 6);
	try{ v->Add("0alla\\1kanister\\error", 13);	}
	catch (std::invalid_argument& e)
	{
		cout << e.what() << endl;
	}
	v->Add("0alla\\1gppp\\2mayflower\\3bullor", 5);
	v->Add("0koven\\4sagarin\\6atternate", 9);
	v->Add("0koven\\4milka\\5heliotrope", 10);
	v->Add("0m0ooon", 12);
	cout << v->PrintVerticeTree() << endl;
	uint32_t depth = 0;
	cout << TreeToPlain(*v, depth);
	/*for (auto iter = v->begin(); iter != v->end(); ++iter)
	{
		cout << (*iter).first << endl;
		cout << (*iter).second << endl;
	}*/


	/*string disk = "test.tfs";
	size_t size = 1024 * 1024;	// 1 Mb
	cout << "Testing file: " << disk << ", size: " << size << endl;

	VFS* vfs = new VFS();
	char* file = new char[5] { 'f', 'i', 'l', 'e', '\0'};
	char test1[]{
"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mo\
rbi suscipit urna libero, quis elementum velit maximus non. \
Nullam fringilla turpis in tellus accumsan, sed luctus nunc \
sollicitudin. Donec malesuada nibh eget efficitur dapibus. C\
lass aptent taciti sociosqu ad litora torquent per conubia n\
ostra, per inceptos himenaeos. Duis at ipsum quis massa moll\
is dapibus. Nunc pulvinar elementum laoreet. Sed pulvinar ni\
bh nibh, sit amet molestie ex dignissim eget. Vestibulum tri\
stique placerat quam, vel scelerisque eros semper ac. Maecen\
as quis ornare turpis, ut maximus mauris. Suspendisse hendre\
rit varius nunc a tempus. Fusce interdum venenatis sapien, s\
it amet consequat lorem hendrerit porttitor. Suspendisse rho\
ncus lacus nec erat congue convallis vel at sapien. Mauris i\
d turpis efficitur, ultricies lorem a, suscipit augue."};
	char test2[] { "Haruki Murakame" };

	char* text = test1;
					
	try
	{
		vfs->MountOrCreate(disk);
		File* f = vfs->Create(file);
		cout << vfs->Write(f, text, strlen(text)) << " B wrote into file: " << f->GetName();
		vfs->Unmount(disk);
	}
	catch (runtime_error& e)
	{
		cout << e.what() << endl;
	}
	delete[] file;
	delete vfs;*/

	// Measuring time:
	//clock_t tStart = clock();
	//printf("Time taken: %.2fs\n", (double)(clock() - tStart) / CLOCKS_PER_SEC);
}