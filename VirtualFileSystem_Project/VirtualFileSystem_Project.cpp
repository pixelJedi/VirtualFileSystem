#include <iostream>
#include <string>
#include <bitset>
#include "IVFS.h"
#include "Vertice.h"

// #include <time.h>	

using namespace std;

int main()
{	
	// Testing VFS

	string str = "sdfs";
	str.resize(50);
	
	string disk = "test.tfs";
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
		std::cout << "---Mount-----------------------------------------\n";
		try	{ 
			vfs->MountOrCreate(disk);	
		} catch (std::invalid_argument& e)
		{
			// todo: rewrite without exeptions, may overflow the stack
			std::cout << e.what();
			vfs->MountOrCreate(disk);
		}
		std::cout << "---Create----------------------------------------\n";
		File* f = vfs->Create(file);
		std::cout << "---Write-----------------------------------------\n";
		cout << vfs->Write(f, text, strlen(text)) << " bytes wrote into file: " << f->GetName() << endl;
		std::cout << "---Read-error------------------------------------\n";
		int pos = 1500;
		char* buff = new char[pos];
		cout << vfs->Read(f, buff, pos) << " bytes read from file: " << f->GetName() << endl;
		std::cout << "---Close-----------------------------------------\n";
		vfs->Close(f);
		std::cout << "---Open-----------------------------------------\n";
		vfs->Open(file);
		std::cout << "---Read-ok---------------------------------------\n";
		cout << vfs->Read(f, buff, pos) << " bytes read from file: " << f->GetName() << endl;
		cout << buff << endl;
		std::cout << "---Unmount---------------------------------------\n";
		vfs->Unmount(disk);
	}
	catch (runtime_error& e)
	{
		cout << e.what() << endl;
	}
	delete[] file;
	delete vfs;/**/

	// Measuring time:
	//clock_t tStart = clock();
	//printf("Time taken: %.2fs\n", (double)(clock() - tStart) / CLOCKS_PER_SEC);
}