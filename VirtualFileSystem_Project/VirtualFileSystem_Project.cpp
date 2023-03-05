#include <iostream>
#include <string>
#include <bitset>
#include "IVFS.h"

// #include <time.h>	

using namespace std;

int main()
{	
	// Testing VFS
	string disk = "test.tfs";
	size_t size = 1024 * 1024;	// 1 Mb
	cout << "Testing file " << disk << ": size " << size << endl;

	VFS* vfs = new VFS();
	char* file = new char[5] { 'f', 'i', 'l', 'e', '\0'};
	std::string test =
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
		d turpis efficitur, ultricies lorem a, suscipit augue.";
	char* text = (char*)&test;
					
	vfs->MountOrCreate(disk);
	File* f = vfs->Create(file);
	vfs->Write(f, text, test.length());
	vfs->Unmount(disk);

	delete[] file;
	delete vfs;/**/

	// Measuring time:
	//clock_t tStart = clock();
	//printf("Time taken: %.2fs\n", (double)(clock() - tStart) / CLOCKS_PER_SEC);
}