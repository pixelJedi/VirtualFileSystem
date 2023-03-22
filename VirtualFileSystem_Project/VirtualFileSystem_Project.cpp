﻿#include <iostream>
#include <string>
#include <bitset>
#include "IVFS.h"
#include "Vertice.h"

// #include <time.h>	

using namespace std;

void run_test1(VFS* vfs);
void measure_time(void(*test)(VFS*), VFS* v);

int main()
{
	VFS* vfs = new VFS();
	string diskname = "test.tfs";
	std::cout << "Testing file: " << diskname << endl;

	try	
	{ 
		std::cout << "---Mount-----------------------------------------\n";
		vfs->MountOrCreate(diskname);
	} catch (std::invalid_argument& e)
	{
		std::cout << e.what();
	}

	try	
	{
		measure_time(run_test1, vfs);
	} catch (runtime_error& e)
	{
		std::cout << e.what() << endl;
	}

	std::cout << "---Unmount---------------------------------------\n";
	vfs->Unmount(diskname);

	delete vfs;
}

void run_test1(VFS* vfs)
{
	/* (One thread)
	/ 1. Открыть 2 файла на запись, 1 на чтение (заранее предсоздать).
	/ 2. Писать по очереди несколько раз в 2 файла данные размером больше чем Block
	/ 3. Закрыть все 3 файла.
	/ 4. Убедиться, что записалось все правильно.
	*/

	// Each name within the path should be less than NODENAME - validation not yet implemented
	char f1[]{ "bin\\file" };
	char f2[]{ "bin\\goal\\abel" };
	char f3[]{ "alpha" };
	char test1[]{	// 2000 bytes
"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla semper tristique mauris, at tristique\
 urna accumsan sed. Curabitur ac facilisis justo, non hendrerit urna. Mauris porttitor ex eget rhonc\
us ultricies. Suspendisse et ornare diam. Integer convallis ex vitae elementum scelerisque. Pellente\
sque aliquet nunc vel mollis elementum. Morbi pulvinar scelerisque dolor. Pellentesque pharetra quam\
 ut eleifend porttitor. Phasellus sollicitudin eu dolor a mollis.\
Donec imperdiet ultricies orci a aliquam. Sed et scelerisque ex. Aenean in nisi nulla. Aliquam portt\
itor elit non augue dapibus consequat sit amet sed nisl. In ipsum leo, posuere ac scelerisque non, v\
olutpat eu metus. Integer non elit ac ante lacinia luctus vitae et lorem. Nulla scelerisque, nibh eg\
et ullamcorper imperdiet, tellus leo tempus orci, sed pellentesque sapien diam non augue. Cras viver\
ra orci ac turpis sodales, eu porttitor elit accumsan. Fusce nec gravida nunc. Phasellus ut risus eu\
 tellus ultricies iaculis.\
Donec egestas pellentesque nisl, posuere dictum elit feugiat a. Nunc lobortis tortor nec mauris iacu\
lis, quis auctor dolor volutpat. Vivamus ornare, lectus et feugiat maximus, lorem lorem interdum vel\
it, ac malesuada elit felis sed diam. Mauris eu blandit justo. Fusce sit amet nisi in tortor pharetr\
a aliquam sit amet nec nisi. Aenean nec ante vitae sem gravida fermentum vel ut eros. Duis a rhoncus\
 quam. Vivamus gravida facilisis congue. Quisque ut aliquam dui. Pellentesque vel porttitor nibh, in\
 tempor magna. In hac habitasse platea dictumst. Praesent porttitor neque eget auctor consequat. Nul\
la id nisl vitae velit mollis elementum. Aenean vestibulum ullamcorper eros, nec aliquet orci congue\
 non. Quisque auctor ligula congue consequat euismod.\
Ut vel accumsan turpis. Phasellus varius fringilla justo, euismod malesuada sem sagittis et. Nulla l\
igula erat, finibus ac lorem ac, porttitor commodo sapien. Nunc aliquam odio vel eros maximus, non m\
ollis tellus ultrices. Sed congue finibus pretium proin." };
	char test2[]{	// 2139 bytes
"What was he supposed to do here? Never get angry? He wasn't sure he could have done anything without\
 being angry and who knows what would have happened to Neville and his books then. Besides, Harry ha\
d read enough fantasy books to know how this one went. He would try to suppress the anger and he wou\
ld fail and it would keep coming out again. And after this whole long journey of self-discovery he w\
ould learn at the end that his anger was a part of himself and that only by accepting it could he le\
arn to use it wisely. Star Wars was the only universe in which the answer actually was that you were\
 supposed to cut yourself off completely from negative emotions, and something about Yoda had always\
 made Harry hate the little green moron.\
So the obvious time - saving plan was to skip the journey of self - discovery and go straight to the\
 part where he realised that only by accepting his anger as a part of himself could he stay in contr\
ol of it.\
The problem was that he didn't feel out of control when he was angry. The cold rage made him feel li\
ke he was in control. It was only when he looked back that events as a whole seemed to have... blown\
 up out of control, somehow.\
He wondered how much the Game Controller cared about that sort of thing, and whether he'd won or los\
t points for it. Harry himself felt like he'd lost quite a few points, and he was sure the old lady \
in the picture would have told him that his was the only opinion that mattered.\
And Harry was also wondering whether the Game Controller had sent Professor Sprout. It was the logic\
al thought: the note had threatened to notify the Game Authorities, and then there Professor Sprout \
was. Maybe Professor Sprout was the Game Controller - the Head of House Hufflepuff would be the last\
 person anyone would suspect, which ought to put her near the top of Harry's list. He'd read one or \
two mystery novels, too.\
\"So how am I doing in the game?\" Harry said out loud.\
A sheet of paper flew over his head, as if someone had thrown it from behind him - Harry turned arou\
nd, but there was no one there - and when Harry turned forwards again, the note was settling to the \
floor." };
	char test3[]{	// 16300 bytes
"0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101\
0101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"
	};
	std::cout << "---Create----------------------------------------\n";
	File* file_w1 = vfs->Create(f1);
	File* file_w2 = vfs->Create(f2);
	File* file_r = vfs->Open(f3);
	std::cout << "---Print-Tree------------------------------------\n";
	vfs->PrintAll();
	std::cout << "---Write-----------------------------------------\n";
	cout << vfs->Write(file_w2, test3, strlen(test3)) << "/" << strlen(test3) << " bytes wrote" << endl;
	cout << vfs->Write(file_w1, test1, strlen(test1)) << "/" << strlen(test1) << " bytes wrote" << endl;
	cout << vfs->Write(file_w2, test2, strlen(test2)) << "/" << strlen(test2) << " bytes wrote" << endl;
	cout << vfs->Write(file_w1, test2, strlen(test2)) << "/" << strlen(test2) << " bytes wrote" << endl;
	cout << vfs->Write(file_w2, test1, strlen(test1)) << "/" << strlen(test1) << " bytes wrote" << endl;
	std::cout << "---Read------------------------------------------\n";
	int size = 3000;
	char* buff = new char[size];
	cout << vfs->Read(file_r, buff, size) << "/" << size << " bytes read" << endl;/**/
	std::cout << "---Close-----------------------------------------\n";
	vfs->Close(file_w1);
	vfs->Close(file_w2);
	vfs->Close(file_r);
}

void measure_time(void(*test)(VFS*), VFS* v)
{
	clock_t tStart, tFin;
	printf(">> Time measuring started!\n");
	tStart = clock();
	test(v);
	tFin = clock();
	printf(">> Time taken: %.2fs\n", (double)(tFin - tStart) / CLOCKS_PER_SEC);
}