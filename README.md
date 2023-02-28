# VirtualFileSystem
The **VFS** (Virtual File System) is used for managing thousands of files packed in a few physical ones, which can be useful in systems with read/write limitations. This implementation is being developed mostly for educational purposes.

> :radioactive: The system is currently unstable, use it on your own risk. All recent changes commited for for informational purposes only.<br/>
> The description below describes the general concept. If you want to check known issues, jump to the [FAQ](https://github.com/pixelJedi/VirtualFileSystem#faq).

Currently, the following constants are used:

- `BYTE` = 8 bits;
- `BLOCK` = 1024 bytes: memory for files is allocated in blocks;
- `CLUSTER` = 16 blocks: cluster is reserved per file when created;
- `ADDR` = 4 bytes: 4-byte addresses are used, stored as uint32_t;
- `DISKDATA` = 4*ADDR bytes (see below);
- `NODEDATA` = 64 bytes (see below).


## VFS
![VDisk internals](/VirtualFileSystem_Description/VFS.png)

### Operating
The VFS class inherits IVFS interface which supports the following actions:

- `Open`:	open an existing file for reading;
- `Create`:	open a new file for writing;
- `Read`:	load bytes from the file to the buffer;
- `Write`:	load bytes from the buffer to the file;
- `Close`:	close file.

### VDisk handling
Each VFS can manage multiple VDisks (each is a file that represents a separate file hierarhy), but the user doesn't have to be aware of the details. The VFS manages all VDisk operations.
It's possible to mount and unmount VDisks by providing names of corresponding files:
- `MountOrCreate(string filename)`
- `Unmount(string filename)`
- todo: `ResetAndMount` - to reuse the filename

## VDisk

Each VDisk is assosiated with a physical file in the underlying file system.

### Creating
- When created, the VDisk object checks the physical file provided and loads specific addresses and data, before it's ready for reading/writing files;
- If the file with the name provided doesn't exist yet, the VDisk creates it and sets the node/block capacity based on the size estimation.

### Implementation
![VDisk internals](/VirtualFileSystem_Description/VDisk.png)

The VDisk consists of 3 main data sections:

1. **Disk Data**. Saves the general data about the VDisk:
	- `Free Nodes`: 	the number of new files/directory that can be created on VDisk (each Node represents a file or directory);
	- `Free Blocks`: 	how many data blocks the VDisk is ready to allocate;
	- `Max Node`: 		the limit on nodes is estimated during the initialization (~16 Kbytes per file estimated);
	- `Next Free Block`:	the design assumes that data is written to sequential blocks where possible, so that variable can speed up memory allocation. It equals the relative address of 1st free block (or even cluster).
2. **Node Data**. Saves the file hierarchy, address of title block and manages access. Each node represents a relation:
	- `Node Code`:		a numeric value in address format, represents a "parent";
	- `Metadata` bitset [8 bits]:
		- [1] folder (1) or file (0)
		- [2] ---
		- [1] writeonly flag
		- [4] readonly counter (multiple threads can read the same file)
	- `File Name`:		filename of a child;
	- `File Address`:	if child is a dir, stores the child's node code, else the file title block's address.
3. **Block Data**. 
	- Each BLOCK is a fixed number of bytes;
		- A File can take multiple blocks (sequential if possible, but that's not obligatory);
		- First block of the file is a Title block that stores name and block adresses of the file. Additional title blocks can be provided;
	- Each CLUSTER is a fixed number of blocks. A whole cluster is reserved per file even if less space is actually required.

### Getting and setting data
Two low-lever functions are responsible for the trick:

- `GetBytes(size_t position, const char* data, size_t length)`
- `SetBytes(size_t position, const char* data, size_t length)`

They use `seekg` and `seekp` pointers of the VDisk's fstream to get or owerwrite `data` of `length` starting from `position` in the file.
They're not const as changing pointers changes the fstream obj.

## File

Physically, a File is stored in blocks on VDisk. Nodes are used to address the first block of the File.
A File consists of title and data blocks.
- **Title blocks** store addresses of data blocks. First title block is the main one, it also stores size;
- **Data blocks** store data.

As a struct, the File is intended to provide quick access to the data. When a File is opened, the vector of block addresses is loaded. To prevent collisions, reading/writing flags are set in the file's node.

### Title block
![VDisk internals](/VirtualFileSystem_Description/TitleBlocks.png)

Title blocks [TB] are made this way:
1. File's info area:
	- `Next TB`:		address of the succeeding TB. If there is no one, the TB addresses itself;
	- `Size`:		real size (in bytes) of the data written in the file.
2. File's data addresses area.
	- The address after the last one used addresses TB;
	- If there is no space for new addresses, a new TB is created, its address is written to the `Next TB` byte.
	
### Data block
> **Note.** Currently it's hard to effectively track available blocks. The idea is to use bitmaps and store it in the Disk Data section.

Data block stores binary data.
When a new block is required, the VDisk checks for available blocks and allocates a new cluster to the file.

### FAQ
> **1. Can I read, write, open or create files using your VFS?**

Well, you can't yet. Basically, you can create a file without checking its existence, but I wouldn't recommend it until I finish the debugging.

> **2. Why do you show us such a raw code?**

It's mainly to demonstrate the concept. Anyway, I'll continue working, check the updates.

> **3. What was the main problem with the system, why you didn't finish??**

The main reason is lack of time and skill, as always are. I've overestimated my programming skills; the overall idea of VFS was clear, but I've drowned in low-level syntax debugging. Also I work and study full-time, but I think the skill issue here is the main one.

> **4. Okay, can I do something with VFS?**

Oh yeah! You can mount and unmount VDisks, the VFS will even initialize it. You also can Create a File in a new VFS and see values popping in a hex editor. Pretty inspiring to see it working, y'know.

> **5. What about multithreading?**

I was planning to make this work using mutexes to prevent working with outdated data. I'm rather new to it, so maybe I'll learn a more effective way (I've heard mutex causes system calls which slow down the process).
Nodes of the files store the flags on reading and writing - a mutex should check it before opening a File.
Also the threads may change the DiskData - there should also be mutex, guarding the collisions.

> **6. What was this about learning? Didn't you know what are you dealing with?**

Honestly, I've learned and updated a ton of things. I didn't use C++ since university so I've read a lot about working with pointers, mutexes, their different kinds, streams (expecially file streams), some syntax sugar of C++, etc., but I still have much to learn, and of course I have to train.
I know other languages, but this task is still more complex than I usually do.
Plus, I've refreshed my understanding of file systems, learned working with Github and writing READMEs.

Anyway, thank you for watching! That task was fun. Now I know what I don't know, which is the most important.
